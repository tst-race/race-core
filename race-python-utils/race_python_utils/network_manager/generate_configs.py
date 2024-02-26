#!/usr/bin/env python3

#
# Copyright 2023 Two Six Technologies
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
Purpose:
    Generate the plugin configs based on a provided range config and save 
    config files to the specified config directory.

    Will take in --range-config arguments to generate 
    configs against.

    Note: if config is not empty, --override will need
    to be run to remove old configs

Steps:
    - Parse CLI args
    - Check for existing configs
        - remove if --override is set and configs exist
        - fail if --override is not set and configs exist
    - Load and Parse Range Config File
    - Generate configs for the plugin
    - Store configs in the specified config directory

usage:
    generate_configs.py \
        [-h] \
        --range RANGE_CONFIG_FILE \
        --channel-props CHANNEL_PROPS_FILE \
        --config-dir CONFIG_DIR \
        [--overwrite]

example call:
    ./generate_plugin_configs.py \
        --range=./2x2.json \
        --channel-props=./channel_props.json \
        --config-dir=./config \
        --overwrite
"""

# Python Library Imports
import argparse
import copy
import json
import logging
import math
import os
import shutil
import sys
from typing import Any, Dict, FrozenSet, List, Optional, Set, Tuple, cast

# Local Lib Imports
from race_python_utils import file_utils
from race_python_utils import range_config_utils
from race_python_utils import network_manager_request_utils
from race_python_utils import network_manager_utils
from race_python_utils import twosix_whiteboard_utils
from race_python_utils.network_manager import Client, Committee, Server

from dataclasses import dataclass


###
# Type Aliases
###

Role = Dict[str, Any]
Channel = Dict[str, Any]
LinkRequest = Dict[str, Any]


LinkSide = str
ChannelId = str
PersonaName = str

Link = Tuple[PersonaName, List[PersonaName]]

###
# Main Execution
###


def run(enable_multicast: bool = False) -> None:
    """
    Purpose:
        Generate configs for Network Manager Two Six Labs Exemplar Plugins
    Args:
        enable_multicast: Enable multicast links from clients to servers
    Returns:
        N/A
    Raises:
        Exception: Config Generation fails
    """
    logging.info("Starting Process To Generate RACE Configs")

    # Parsing Configs
    cli_args = get_cli_arguments(enable_multicast=enable_multicast)

    state = create_state(cli_args)

    if state.prev_attempts == 0:
        handle_initial_attempt(state)
    else:
        handle_final_attempt(state)

    status_file_path = f"{cli_args.config_dir}/network-manager-config-gen-status.json"
    file_utils.write_json(state.status, status_file_path)
    logging.info("Process To Generate RACE Plugin Config Complete")


##
# State functions
##


@dataclass
class ConfigGenState:
    """
    Purpose:
        Hold parsed information about the environment
    """

    prev_attempts: int
    cli_args: argparse.Namespace
    enable_multicast: bool
    range_config: Dict[str, Any]
    committees: Dict[str, Committee]
    personas: List[Dict[str, Any]]
    channels: List[Channel]
    req_gen_c2s: List[Channel]
    req_gen_s2s: List[Channel]
    req_dyn_c2s: List[Channel]
    req_dyn_s2s: List[Channel]
    possible_c2s: List[Channel]
    possible_s2s: List[Channel]
    bootstrap_channels: List[Channel]
    requested_links: List[LinkRequest]
    fulfilled_links: List[LinkRequest]
    status: Dict[str, Any]


def create_state(cli_args: argparse.Namespace) -> ConfigGenState:
    """
    Purpose:
        Parse cli args, read files and create a state object with that information
    """
    range_config = cast(
        Dict[str, Any], file_utils.read_json(cli_args.range_config_file)
    )
    range_config_utils.validate_range_config(
        range_config=range_config, allow_no_clients=cli_args.allow_no_clients
    )
    personas = network_manager_utils.generate_personas_config_from_range_config(range_config)

    if not cli_args.fulfilled_requests_file:
        prev_attempts = 0
        requested_links = []
        fulfilled_links = []
    else:
        status_filename = f"{cli_args.config_dir}/network-manager-config-gen-status.json"
        prev_attempts = file_utils.read_json(status_filename).get("attempt")

        # remove file so that rib doesn't get confused on failure
        os.remove(status_filename)

        request_filename = f"{cli_args.config_dir}/network-manager-request.json"
        requested_links = file_utils.read_json(request_filename).get("links")

        fulfilled_filename = cli_args.fulfilled_requests_file
        fulfilled_links = file_utils.read_json(fulfilled_filename).get("links")

    channels = cast(List[Channel], file_utils.read_json(cli_args.channel_list_file))

    channels_dict = {channel["channelGid"]: channel for channel in channels}

    req_gen_c2s = filter_channel_request(cli_args.genesis_c2s_channels, channels_dict)
    req_gen_s2s = filter_channel_request(cli_args.genesis_s2s_channels, channels_dict)
    req_dyn_c2s = filter_channel_request(cli_args.dynamic_c2s_channels, channels_dict)
    req_dyn_s2s = filter_channel_request(cli_args.dynamic_s2s_channels, channels_dict)

    req_channels = req_gen_c2s + req_gen_s2s + req_dyn_c2s + req_dyn_s2s
    possible_c2s = filter_compatible_channels(channels, req_channels, "CT_INDIRECT")

    # Indirect channels are also allowed to be s2s channels, but should have lower priority than direct channels
    possible_s2s = filter_compatible_channels(channels, req_channels, "CT_DIRECT")
    possible_s2s += possible_c2s

    bootstrap = filter_compatible_channels(channels, req_channels, "CT_LOCAL")

    enable_multicast = getattr(cli_args, "enable_multicast", False)
    # If multicast is enabled, check if any of the genesis channels support multicast. If none
    # support it, disable multicast automatically.
    if enable_multicast:
        genesis_c2s = req_gen_c2s if req_gen_c2s else possible_c2s
        enable_multicast = any(
            [channel["transmissionType"] == "TT_MULTICAST" for channel in genesis_c2s]
        )
        if not enable_multicast:
            logging.info(
                "Multicast has been disabled because none of the genesis channels support multicast"
            )

    return ConfigGenState(
        prev_attempts=prev_attempts,
        cli_args=cli_args,
        enable_multicast=enable_multicast,
        range_config=range_config,
        committees=network_manager_utils.generate_flat_committees_from_reachability(range_config),
        personas=personas,
        req_gen_c2s=req_gen_c2s,
        req_gen_s2s=req_gen_s2s,
        req_dyn_c2s=req_dyn_c2s,
        req_dyn_s2s=req_dyn_s2s,
        possible_c2s=possible_c2s,
        possible_s2s=possible_s2s,
        bootstrap_channels=bootstrap,
        channels=channels,
        requested_links=requested_links,
        fulfilled_links=fulfilled_links,
        status={"attempt": prev_attempts + 1},
    )


def filter_channel_request(
    request_str: str, channels: Dict[ChannelId, Channel]
) -> List[Channel]:
    """
    Purpose:
        Filter requested channels to ensure they are in the channel_list
    Args:
        request_str: comma-separated list of channels
        channels: dictionary of channel names and properties
    Return:
        requested_channels: List of channels after filtering (possibly empty)
    """
    if request_str != "":
        return [channels[cid] for cid in request_str.split(",")]
    return []


def filter_compatible_channels(
    channels: List[Channel],
    required_channels: List[Channel],
    connection_type: str,
) -> List[Channel]:
    """
    Purpose:
        Filter optional channels so they don't conflict with the required channels
    Return:
        List of channels of the specified connection type that do not conflict with a required
        channel. See channels_compatible for a definiton of when channels conflict
    """
    channels = [chan for chan in channels if chan["connectionType"] == connection_type]
    for existing_channel in required_channels:
        channels = [
            channel
            for channel in channels
            if channels_compatible(channel, existing_channel)
        ]

    return channels


def channels_compatible(a: Channel, b: Channel) -> bool:
    """
    Purpose:
        Detect if two channels are compatible with each other.

        A channel 'A' is compatible with another channel 'B' if channel A could still
        create links on a fully connected graph after channel B created as many links
        as possible and vice versa. This does not guarantee that links will be created
        on by channel A for the specified connectivity.

        Examples:
            Channels A and B are the same channel -> Channels conflict
                Channel 'B' creates as many links as possible so Channel A can't
                possibly create any more if they're the same channel
            None of the roles of Channels A and B conflict -> Channels compatible
            One role of Channel A conflicts with all roles of Channel B -> Channels conflict
            One role of Channel A conflicts with one roles of Channel B and the other role of
                Channel A conflicts with the other role of Channel B -> Channels compatible!
            All other examples with channel A and channel B swapped

        This logic is necessary to handle the case where there are two unidirectional
        channels that are only compatible with each other when sending in opposite
        directions

    Return:
        List of channels of the specified connection type that do not conflict with a
        required channel. See channels_compatible for a definiton of when channels conflict
    """

    # Check if they are the same channel
    if a["channelGid"] == b["channelGid"]:
        return False

    # Check that every role of A is compatible with some role of B
    if len(possible_roles(a, b)) != len(a["roles"]):
        return False

    # Check that every role of B is compatible with some role of A
    if len(possible_roles(b, a)) != len(b["roles"]):
        return False

    return True


def possible_roles(a: Channel, b: Channel) -> List[Role]:
    """
    Get a list of roles of channel A that are compatible with some role of channel B
    """
    roles = []
    for a_role in a["roles"]:
        for b_role in b["roles"]:
            if roles_compatible(a_role, b_role):
                roles.append(a_role)
                break
    return roles


def roles_compatible(a: Role, b: Role) -> bool:
    """
    Check if two roles conflict. Roles conflict if they have any mechanical tags in common.
    """
    a_tags = set(a["mechanicalTags"])
    b_tags = set(b["mechanicalTags"])
    return len(a_tags.intersection(b_tags)) == 0


###
# Handle Attempt Functions
###


def handle_initial_attempt(state: ConfigGenState) -> None:
    """
    Logic for the first time rib calls into generate configs
    """
    logging.info(f"Making initial links request.")

    channels = [channel["channelGid"] for channel in state.req_gen_c2s]
    logging.info(f"    required genesis c2s: {channels}")
    channels = [channel["channelGid"] for channel in state.req_gen_s2s]
    logging.info(f"    required genesis s2s: {channels}")
    channels = [channel["channelGid"] for channel in state.req_dyn_c2s]
    logging.info(f"    required dynamic c2s: {channels}")
    channels = [channel["channelGid"] for channel in state.req_dyn_s2s]
    logging.info(f"    required dynamic s2s: {channels}")
    channels = [channel["channelGid"] for channel in state.possible_c2s]
    logging.info(f"    possible c2s: {channels}")
    channels = [channel["channelGid"] for channel in state.possible_s2s]
    logging.info(f"    possible s2s: {channels}")
    channels = [channel["channelGid"] for channel in state.bootstrap_channels]
    logging.info(f"    bootstrap: {channels}")

    # Prepare the dir to store configs in, check for previous values and overwrite
    file_utils.prepare_network_manager_config_dir(
        state.cli_args.config_dir, state.cli_args.overwrite, state.personas
    )

    generate_network_manager_request(state)

    state.status.update({"status": "needs-review", "reason": "first pass"})


def handle_final_attempt(state: ConfigGenState) -> None:
    """
    Logic for the final (second) time rib calls into generate configs
    """
    if get_unfulfilled_links(state) == []:
        logging.info(f"Config generation succeeded")
        # Status = complete tells RiB to stop iterating
        state.status.update({"status": "complete", "reason": "success"})
    else:
        state.status.update({"status": "complete", "reason": "failed"})
        raise Exception(f"Config generation failed: Some links were not fulfilled")


def link_hash_function(link: dict) -> str:
    """
    Purpose:
        Determine unique hash of a link. This function assumes no two links have the exact same
        sender and list of recipients. There can be multiple channels that create their own
        instantiation of a link, but the sender/recipient pairing is what defines the link.
    Args:
        link (dict): Dictionary of link specific fields
    Returns:
        link_has (str): unique id for link
    """

    sender = link.get("sender")
    recipients = link.get("recipients", [])
    recipients.sort()
    return f"sender-{sender}-recipients-{recipients}"


def get_unfulfilled_links(state: ConfigGenState) -> List[dict]:
    """
    Purpose:
        Find links that were requested but not fulfilled
    Args:
        state (ConfigGenState): State object containing information about the environment
    Returns:
        unfulfilled_links (List[dict]): list of links that were requested but not fulfilled
    """

    network_manager_fulfilled_request_map = {}
    for link in state.fulfilled_links:
        network_manager_fulfilled_request_map[link_hash_function(link)] = link

    unfulfilled_links = []
    for link in state.requested_links:
        network_manager_fulfilled_request_map_key = link_hash_function(link)
        if network_manager_fulfilled_request_map_key not in network_manager_fulfilled_request_map.keys():
            unfulfilled_links.append(link)
        elif not network_manager_fulfilled_request_map[network_manager_fulfilled_request_map_key]["channels"]:
            unfulfilled_links.append(link)
    return unfulfilled_links


###
# Generate Config Functions
###


def determine_links_needed(
    state: ConfigGenState,
) -> Tuple[List[Link], List[Link]]:
    """
    Purpose:
        Determine genesis links needed to connect committees and clients. If complete
        connectivity was specified then all clients connect to all servers and all servers
        connect to all other servers. Otherwise, links are made just along the lines
        specified in the committees.

        When not specifying complete connectivity, the order of the returned links is important.
        The returned link order is c2s links, then links within a committee (and specifically in
        order around a ring), and then links between committees. This order is necessary for when
        nodes are being assigned roles later as it avoids most indirect links being used for s2s.
    Args:
        state (ConfigGenState): State object containing information about the environment
    Returns:
        A pair of lists for c2s links and s2s links containing src/dst persona pairs
    """
    if state.cli_args.complete_connectivity:
        client_uuids = {
            persona["raceUuid"]
            for persona in state.personas
            if persona["personaType"] == "client"
        }
        server_uuids = {
            persona["raceUuid"]
            for persona in state.personas
            if persona["personaType"] == "server"
        }
        client_uuids = sorted(client_uuids)
        server_uuids = sorted(server_uuids)
        c2s_links_needed: List[Link] = [
            (client, [server]) for client in client_uuids for server in server_uuids
        ]
        s2s_links_needed: List[Link] = [
            (server1, [server2])
            for server1 in server_uuids
            for server2 in server_uuids
            if server1 != server2
        ]

    else:
        c2s_links_needed = []
        s2s_links_needed = []
        for committee_name, committee_obj in state.committees.items():
            # client-driven links - servers in entrance and exit committees
            for client_obj in committee_obj.clients:
                for server in [
                    server.name for server in client_obj.entrance_committee
                ] + [server.name for server in client_obj.exit_committee]:
                    c2s_links_needed.append((client_obj.name, [server]))
                    logging.info(f"Adding committee link: {c2s_links_needed[-1]}")
                if state.enable_multicast:
                    # Multicast link to entrance committee
                    c2s_links_needed.append(
                        (
                            client_obj.name,
                            [server.name for server in client_obj.entrance_committee],
                        )
                    )
                    logging.info(
                        f"Adding entrance committee link: {c2s_links_needed[-1]}"
                    )

        # server-driven links
        # links for the rings in the committee
        for _, committee_obj in state.committees.items():
            for ring in committee_obj.rings:
                for idx in range(len(ring)):
                    next_idx = (idx + 1) % len(ring)
                    s2s_links_needed.append((ring[idx], [ring[next_idx]]))
                    logging.info(f"Adding ring link: {s2s_links_needed[-1]}")

        # links for the intercommittee connectivity
        # these should be generated after rings to ensure that loader-creator links can
        # be used for links inside the ring
        for _, committee_obj in state.committees.items():
            for server_obj in committee_obj.servers:
                server_json = server_obj.json_config()
                for _, reachable in server_json["reachableCommittees"].items():
                    s2s_links_needed.append((server_obj.name, [reachable[0]]))
                    logging.info(f"Adding intercommittee link: {s2s_links_needed[-1]}")

    # request links in both directions
    s2s_links_needed = [
        (src, dst)
        for node1, node2 in s2s_links_needed
        for src, dst in [(node1, node2), (node2[0], [node1])]
    ]
    c2s_links_needed = [
        (src, dst)
        for node1, node2 in c2s_links_needed
        for src, dst in [(node1, node2), (node2[0], [node1])]
    ]

    # Make links unique in case there are duplicates.
    s2s_links_needed = dedup(s2s_links_needed)
    c2s_links_needed = dedup(c2s_links_needed)

    return c2s_links_needed, s2s_links_needed


def dedup(links: List[Link]) -> List[Link]:
    """Remove duplicate link pairs from the list of sender/recipients tuples"""
    deduped: List[Link] = []
    dsts_by_src: Dict[PersonaName, List[List[PersonaName]]] = {}
    for src, dsts in links:
        if dsts not in dsts_by_src.setdefault(src, []):
            dsts_by_src[src].append(dsts)
            deduped.append((src, dsts))
            logging.info(f"Need link: {deduped[-1]}")
    return deduped


@dataclass
class NodeState:
    """
    Tracks assigned role and links for a node
    """

    persona: str
    channel_roles: Dict[ChannelId, Role]
    mechanical_tags: Set[str]

    # Maps dst node to set of channel ids and link side that we expect links on to that node.
    # Includes dynamic links.
    expected_links: Dict[PersonaName, Dict[ChannelId, LinkSide]]

    expected_multicast_links: List[Dict[str, Any]]


def find_role(
    roles: List[Role], allowed_link_side: List[str], channel_id: ChannelId
) -> Role:
    """
    Find a role that has the one of the specified link side values. Raises an
    exception if a role cannot be found.
    """
    for allowed_role in allowed_link_side:
        for role in roles:
            if role["linkSide"] == allowed_role:
                return role
    raise Exception(
        f"Could not find valid role for channel '{channel_id}'. "
        f"Channel roles: {[role['linkSide'] for role in roles]}, "
        f"Allowed roles: {allowed_link_side}"
    )


def role_compatible_with_node(
    node: NodeState, role: Role, channel_id: ChannelId
) -> bool:
    """
    Check if a role is compatible with the already assigned roles for a node
    """
    # check if node already has a role for this channel
    # if so, check if role is already assigned
    if channel_id in node.channel_roles:
        return node.channel_roles[channel_id]["roleName"] == role["roleName"]

    # check if node has conflicting mechanical tags for this channel
    if len(node.mechanical_tags.intersection(set(role["mechanicalTags"]))) > 0:
        return False

    return True


def add_role_to_node(node: NodeState, role: Role, channel_id: ChannelId) -> None:
    """
    Assign a role to a node for the specified channel
    """
    node.channel_roles[channel_id] = role
    node.mechanical_tags |= set(role["mechanicalTags"])


def node_has_expected_link(
    node: NodeState, dest_persona: str, channel_id: ChannelId, link_side: LinkSide
) -> bool:
    """
    Check if an expected link for the given channel and link side exists for a node
    """
    return node.expected_links.get(dest_persona, {}).get(channel_id) == link_side


def node_has_expected_multicast_link(
    node: NodeState,
    personas: List[NodeState],
    channel_id: ChannelId,
    link_side: LinkSide,
) -> bool:
    """
    Check if an expected multicast link exists for a node
    """
    for link in node.expected_multicast_links:
        if (
            link["personas"] == [node.persona for node in personas]
            and link["channelGid"] == channel_id
            and link["linkSide"] == link_side
        ):
            return True
    return False


def matching_link_side(link_side: str) -> str:
    """
    Get the allowed link side for the other end of a link given the type of one side
    """
    if link_side == "LS_BOTH":
        return "LS_BOTH"
    if link_side == "LS_LOADER":
        return "LS_CREATOR"
    if link_side == "LS_CREATOR":
        return "LS_LOADER"
    raise Exception(f"Invalid string for link side '{link_side}'")


def fulfill_request(
    channel: Channel,
    sender_node: NodeState,
    receiver_nodes: List[NodeState],
    update_roles: bool,
) -> bool:
    """
    Check if a channel can fulfill a link from sender to receiver(s) and optionally
    update the roles of each node.
    """
    channel_id = channel["channelGid"]
    link_direction = channel["linkDirection"]

    if len(receiver_nodes) > 1 and channel["transmissionType"] != "TT_MULTICAST":
        logging.warning(f"Cannot request multicast link from {channel_id}")
        return False

    # Determine the roles each side of the link must have
    loader_roles = ["LS_BOTH", "LS_LOADER"]
    creator_roles = ["LS_BOTH", "LS_CREATOR"]
    if link_direction == "LD_LOADER_TO_CREATOR":
        sender_role = find_role(channel["roles"], loader_roles, channel_id)
        receiver_role = find_role(channel["roles"], creator_roles, channel_id)
    elif link_direction == "LD_CREATOR_TO_LOADER":
        sender_role = find_role(channel["roles"], creator_roles, channel_id)
        receiver_role = find_role(channel["roles"], loader_roles, channel_id)
    elif link_direction == "LD_BIDI":
        maybe_sender_role = sender_node.channel_roles.get(channel_id)
        maybe_receiver_role = receiver_nodes[0].channel_roles.get(channel_id)
        if maybe_sender_role:
            sender_role = maybe_sender_role
            receiver_role = find_role(
                channel["roles"],
                [matching_link_side(sender_role["linkSide"])],
                channel_id,
            )
        elif maybe_receiver_role:
            receiver_role = maybe_receiver_role
            sender_role = find_role(
                channel["roles"],
                [matching_link_side(receiver_role["linkSide"])],
                channel_id,
            )
        else:
            sender_role = find_role(channel["roles"], loader_roles, channel_id)
            receiver_role = find_role(channel["roles"], creator_roles, channel_id)
    else:
        raise Exception(
            f"Found invalid link direction {link_direction} for channel '{channel_id}'"
        )

    # Check if the roles are compatible with the existing roles for each node
    if role_compatible_with_node(sender_node, sender_role, channel_id) and all(
        [
            role_compatible_with_node(receiver_node, receiver_role, channel_id)
            for receiver_node in receiver_nodes
        ]
    ):
        # Don't add a both link-side bidirection link to both nodes, it's only needed on one node in
        # the pair
        if (
            link_direction == "LD_BIDI"
            and sender_role["linkSide"] == "LS_BOTH"
            and receiver_role["linkSide"] == "LS_BOTH"
            and len(receiver_nodes) == 1
            and node_has_expected_link(
                receiver_nodes[0], sender_node.persona, channel_id, "LS_BOTH"
            )
        ):
            update_roles = False

        if update_roles:
            add_role_to_node(sender_node, sender_role, channel_id)
            if len(receiver_nodes) == 1:
                add_role_to_node(receiver_nodes[0], receiver_role, channel_id)
                sender_node.expected_links.setdefault(
                    receiver_nodes[0].persona, dict()
                )[channel_id] = sender_role["linkSide"]
            elif len(receiver_nodes) > 1 and not node_has_expected_multicast_link(
                sender_node, receiver_nodes, channel_id, sender_role["linkSide"]
            ):
                for receiver_node in receiver_nodes:
                    add_role_to_node(receiver_node, receiver_role, channel_id)
                sender_node.expected_multicast_links.append(
                    dict(
                        personas=[node.persona for node in receiver_nodes],
                        channelGid=channel_id,
                        linkSide=sender_role["linkSide"],
                    )
                )

        return True
    return False


def generate_network_manager_request(state: ConfigGenState) -> None:
    """
    Purpose:
        Generate the plugin configs and network manager request
    Args:
        state (ConfigGenState): State object containing information about the environment
    Raises:
        Exception: generation fails
    Returns:
        None
    """

    network_manager_utils.analyze_committees(state.committees)

    nodes = {
        persona["raceUuid"]: NodeState(persona["raceUuid"], {}, set(), {}, [])
        for persona in state.personas
    }
    network_manager_request: List[LinkRequest] = []

    c2s_links_needed, s2s_links_needed = determine_links_needed(state)

    # assign roles for genesis channels and add links to network_manager_request
    genesis_s2s = state.req_gen_s2s if state.req_gen_s2s else state.possible_s2s
    create_links(genesis_s2s, nodes, network_manager_request, s2s_links_needed)

    genesis_c2s = state.req_gen_c2s if state.req_gen_c2s else state.possible_c2s
    create_links(genesis_c2s, nodes, network_manager_request, c2s_links_needed)

    # assign roles for dynamic channels
    dynamic_s2s = state.req_dyn_s2s if state.req_dyn_s2s else state.possible_s2s
    assign_dyn_channel_roles(dynamic_s2s, nodes, s2s_links_needed)

    dynamic_c2s = state.req_dyn_c2s if state.req_dyn_c2s else state.possible_c2s
    assign_dyn_channel_roles(dynamic_c2s, nodes, c2s_links_needed)

    assign_bootstrap_channel_roles(state.bootstrap_channels, nodes)

    # make sure the explicitly requested links are created
    check_req_links(state.req_gen_s2s, nodes, s2s_links_needed)
    check_req_links(state.req_gen_c2s, nodes, c2s_links_needed)
    check_req_links(state.req_dyn_s2s, nodes, s2s_links_needed)
    check_req_links(state.req_dyn_c2s, nodes, c2s_links_needed)

    # write config.json for each node
    for committee_name, committee_obj in state.committees.items():
        for client in committee_obj.clients:
            write_config(client, state, network_manager_request, nodes[client.name])
        for server in committee_obj.servers:
            write_config(server, state, network_manager_request, nodes[server.name])

    # write shared configs
    generate_shared_configs(state)

    filename = f"{state.cli_args.config_dir}/network-manager-request.json"
    file_utils.write_json({"links": network_manager_request}, filename)


def create_links(
    channels: List[Channel],
    nodes: Dict[PersonaName, NodeState],
    network_manager_request: List[LinkRequest],
    links_needed: List[Link],
) -> None:
    """
    Add requests to the network_manager_request list based on channels expected to be able to fulfill them.
    This also assigns roles to each node as links are requested on channels. Channels are
    iterated over in order. Each channel takes as many of the remaining links as possible.
    Each link is only requested on a specific channel.

    Raises an Exception if there are remaining links after trying all channels.
    """
    for channel in channels:
        if not channel.get("enabled", True):
            logging.warning(
                f"Cannot request link from disabled channel {channel['channelGid']}"
            )
            continue

        links_still_needed: List[Link] = []
        for src, dsts in links_needed:
            if fulfill_request(channel, nodes[src], [nodes[dst] for dst in dsts], True):
                channel_id = channel["channelGid"]
                logging.info(
                    f"Requesting {(src, dsts)} genesis link from channel {channel_id}"
                )
                network_manager_request.append(
                    {
                        "sender": src,
                        "recipients": dsts,
                        "details": {},
                        "groupId": None,  # TODO populate this for multicast?
                        "channels": [channel_id],
                    }
                )
            else:
                links_still_needed.append((src, dsts))
        links_needed = links_still_needed

    if len(links_needed) > 0:
        raise Exception("Failed to find channels to satisify all links")


def assign_dyn_channel_roles(
    channels: List[Channel],
    nodes: Dict[PersonaName, NodeState],
    links_needed: List[Link],
) -> None:
    """
    This also assigns roles to each node if a dynamic link can be created between nodes.
    Channels are iterated over in order. Multiple dynamic links can be created for each
    link, but channels can conflict (and therefore not be assigned roles) with previously
    chosen dynamic channels.
    """
    for channel in channels:
        for src, dsts in links_needed:
            # fulfill request will update node with the role and tags of the dynamic channel
            if fulfill_request(channel, nodes[src], [nodes[dst] for dst in dsts], True):
                logging.info(
                    f"Expecting {(src, dsts)} link from channel {channel['channelGid']}"
                )


def assign_bootstrap_channel_roles(
    channels: List[Channel], nodes: Dict[PersonaName, NodeState]
):
    """
    Assigns the role for bootstrap channels. Bootstrap channels are expected to have a single
    role and not conflict with an other channel. If either of those is not the case then this
    function raises an Exception.
    """
    for channel in channels:
        for node in nodes.values():
            channel_id = channel["channelGid"]
            roles = channel["roles"]
            if len(roles) != 1:
                raise Exception(
                    f"Expected 1 role for bootstrap channel. Got {len(roles)}"
                )

            role = roles[0]
            if role_compatible_with_node(node, role, channel_id):
                add_role_to_node(node, role, channel_id)
            else:
                raise Exception(
                    "Bootstrap channel not compatible with channels on node."
                )


def check_req_links(
    channels: List[Channel],
    nodes: Dict[PersonaName, NodeState],
    links: List[Link],
) -> None:
    """
    Check that at least one link is expected to be created for each channel. For genesis links,
    these will be available in link-profiles.json. For dynamic links they will be requested at
    runtime.

    We only have to check expected channels for required links because if a channel is a
    required genesis channel then is can't be selected as a dynamic channel and vice versa for
    dynamic channels
    """
    for channel in channels:
        for src, dsts in links:
            node = nodes[src]
            expected_channels = node.expected_links[dsts[0]].keys()
            if channel["channelGid"] in expected_channels:
                break
        else:
            raise Exception(
                f"{channel['channelGid']} is required but will not have any links. This is "
                "likely caused by some conflict between channels, or because all genesis links "
                "were already fulfilled by another genesis channel."
            )


def write_config(
    node, state: ConfigGenState, network_manager_request: List[LinkRequest], node_state: NodeState
) -> None:
    """
    Purpose:
        Formats informations for and writes to config.json
    """
    node_json = node.json_config()
    node_json["useLinkWizard"] = not state.cli_args.disable_dynamic_links
    node_json["channelRoles"] = {
        channel_id: role["roleName"]
        for channel_id, role in node_state.channel_roles.items()
    }
    node_json["expectedLinks"] = node_state.expected_links
    if isinstance(node, Client):
        node_json["expectedMulticastLinks"] = node_state.expected_multicast_links

    node_json["otherConnections"] = [
        recipient
        for link in network_manager_request
        for recipient in link["recipients"]
        if link["sender"] == node.name
    ]

    filename = f"{state.cli_args.config_dir}/{node.name}/config.json"
    file_utils.write_json(node_json, filename)


def generate_shared_configs(state: ConfigGenState) -> None:
    """
    Purpose:
        Generate configs shared by all nodes. Currently this is the list of personas and keys for each node.
    Args:
        state (ConfigGenState): State object containing information about the environment
    Raises:
        Exception: generation fails
    Returns:
        None
    """

    ###
    # Generate Common Configs
    ###

    # Store personas file
    filepath = f"{state.cli_args.config_dir}/shared/personas/race-personas.json"
    file_utils.write_json(state.personas, filepath)

    # Store AES Keys
    aes_keys = network_manager_utils.generate_aes_keys_from_range_config(state.range_config)
    for race_node, aes_key in aes_keys.items():
        filepath = f"{state.cli_args.config_dir}/shared/personas/{race_node}.aes"
        file_utils.write_bytes(aes_key, filepath)


###
# Helper Functions
###


def get_cli_arguments(enable_multicast: bool = False) -> argparse.Namespace:
    """
    Purpose:
        Parse CLI arguments for script
    Args:
        enable_multicast: Enable multicast arguments
    Return:
        cli_arguments (ArgumentParser Obj): Parsed Arguments Object
    """
    logging.info("Getting and Parsing CLI Arguments")

    parser = argparse.ArgumentParser(description="Generate RACE Config Files")
    required = parser.add_argument_group("Required Arguments")
    optional = parser.add_argument_group("Optional Arguments")

    # Required Arguments
    required.add_argument(
        "--range",
        dest="range_config_file",
        help="Range config of the physical network",
        required=True,
        type=str,
    )
    required.add_argument(
        "--channel-list",
        dest="channel_list_file",
        help="list of channel properties",
        required=True,
        type=str,
    )
    required.add_argument(
        "--config-dir",
        dest="config_dir",
        help="Where should configs be stored?",
        required=True,
        type=str,
    )

    # Optional Arguments
    optional.add_argument(
        "--overwrite",
        dest="overwrite",
        help="Overwrite configs if they exist",
        required=False,
        default=False,
        action="store_true",
    )
    optional.add_argument(
        "--local",
        dest="local_override",
        help=(
            "Ignore range config service connectivity, utilized "
            "local configs (e.g. local hostname/port vs range services fields). "
            "Does nothing for Direct Links at the moment"
        ),
        required=False,
        default=False,
        action="store_true",
    )
    optional.add_argument(
        "--genesis-c2s-channels",
        dest="genesis_c2s_channels",
        help="(Comma-separated list of channels) Only use the channels for genesis client <=> server links",
        required=False,
        default="",
        type=str,
    )
    optional.add_argument(
        "--genesis-s2s-channels",
        dest="genesis_s2s_channels",
        help="(Comma-separated list of channels) Only use the channels for genesis server <=> server links",
        required=False,
        default="",
        type=str,
    )
    optional.add_argument(
        "--dynamic-c2s-channels",
        dest="dynamic_c2s_channels",
        help="(Comma-separated list of channels) Dynamically create client <=> server links for these channels after start (requires at least one available C2S channel not be specified)",
        required=False,
        default="",
        type=str,
    )
    optional.add_argument(
        "--dynamic-s2s-channels",
        dest="dynamic_s2s_channels",
        help="(Comma-separated list of channels) Dynamically create server <=> server links for these channels after start (requires at least one available C2S channel not be specified)",
        required=False,
        default="",
        type=str,
    )
    optional.add_argument(
        "--allow-no-clients",
        help="Allow range configs that only contain server nodes",
        required=False,
        default=False,
        action="store_true",
    )
    optional.add_argument(
        "--complete-connectivity",
        dest="complete_connectivity",
        help="Request genesis links connecting every server-server and server-client pair (a complete graph minus client-client links)",
        required=False,
        default=False,
        action="store_true",
    )
    optional.add_argument(
        "--fulfilled-requests",
        dest="fulfilled_requests_file",
        help="merged fulfilled requests from comms config generators",
        required=False,
        type=str,
    )
    optional.add_argument(
        "--disableDynamicLinks",
        dest="disable_dynamic_links",
        help="Disable dynamic links functionality (LinkWizard)",
        required=False,
        default=False,
        action="store_true",
    )
    if enable_multicast:
        # If enabled, allow disabling of multicast
        optional.add_argument(
            "--no-multicast",
            dest="enable_multicast",
            help="Disable multicast links",
            action="store_false",
        )

    return parser.parse_args()
