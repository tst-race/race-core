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

    Will take in --range-config and --link-request arguments to generate 
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
        [--overwrite] \
        [--config-dir CONFIG_DIR] \
        [--network-manager-request NETWORK_MANAGER_REQUEST_FILE]

example call:
    ./generate_plugin_configs.py \
        --range=./2x2.json \
        --config-dir=./config \
        --overwrite
"""

# Python Library Imports
import argparse
import json
import logging
import math
import os
import sys
from typing import Any, Dict, List, Optional, Tuple

# Local Lib Imports
RACE_UTILS_PATH = f"{os.path.dirname(os.path.realpath(__file__))}/../race-python-utils"
sys.path.insert(0, RACE_UTILS_PATH)
from race_python_utils import file_utils
from race_python_utils import range_config_utils
from race_python_utils import network_manager_request_utils
from race_python_utils import network_manager_utils
from race_python_utils import comms_link_utils
from race_python_utils import twosix_whiteboard_utils


###
# Global
###


CHANNEL_ID = "twoSixDirectGolang"
try:
    with open(
        f"{os.path.dirname(os.path.realpath(__file__))}/channel_properties.json"
    ) as json_file:
        CHANNEL_PROPERTIES = json.load(json_file)
except Exception as err:
    print(f"ERROR: Failed to read channel properties: {repr(err)}")
    CHANNEL_PROPERTIES = {}

# Start port for golang exemplar
START_PORT = 26_000
END_PORT = 27_999


###
# Main Execution
###


def main() -> None:
    """
    Purpose:
        Generate configs for twoSixDirectGolang
    Args:
        N/A
    Returns:
        N/A
    Raises:
        Exception: Config Generation fails
    """
    logging.info("Starting Process To Generate RACE Configs")

    # Parsing Configs
    cli_args = get_cli_arguments()

    # Load and validate Range Config
    range_config = file_utils.read_json(cli_args.range_config_file)
    range_config_utils.validate_range_config(
        range_config=range_config, allow_no_clients=True
    )

    # Load (or create) and validate Network Manager Request
    if cli_args.network_manager_request_file:
        network_manager_request = file_utils.read_json(cli_args.network_manager_request_file)
    else:
        network_manager_request = network_manager_request_utils.generate_network_manager_request_from_range_config(
            range_config, [CHANNEL_PROPERTIES], "unicast", "direct"
        )
    network_manager_request_utils.validate_network_manager_request(network_manager_request, range_config)

    # Prepare the dir to store configs in, check for previous values and overwrite
    file_utils.prepare_comms_config_dir(cli_args.config_dir, cli_args.overwrite)

    # Generate Configs
    generate_configs(range_config, network_manager_request, cli_args)

    logging.info("Process To Generate RACE Plugin Config Complete")


###
# Generate Config Functions
###


def generate_configs(
    range_config: Dict[str, Any],
    network_manager_request: Dict[str, Any],
    cli_args: argparse.Namespace,
) -> None:
    """
    Purpose:
        Generate the plugin configs based on range config and network manager request
    Args:
        range_config: range config to generate against
        network_manager_request: requested links to generate against
        config_dir: where to store configs
        local_override: Ignore range config services. Use Two Six Local Information.
    Raises:
        Exception: generation fails
    Returns:
        None
    """

    # Generate Genesis Link Addresses
    (link_addresses, fulfilled_network_manager_request) = generate_genesis_link_addresses(
        range_config, network_manager_request, cli_args
    )
    file_utils.write_json(
        {CHANNEL_ID: link_addresses},
        f"{cli_args.config_dir}/genesis-link-addresses.json",
    )

    # Store the fufilled links to compare to the request (may need more iterations)
    file_utils.write_json(
        fulfilled_network_manager_request, f"{cli_args.config_dir}/fulfilled-network-manager-request.json"
    )

    user_responses = generate_user_responses(range_config)
    file_utils.write_json(user_responses, f"{cli_args.config_dir}/user-responses.json")


def generate_genesis_link_addresses(
    range_config: Dict[str, Any],
    network_manager_request: Dict[str, Any],
    cli_args: argparse.Namespace,
) -> Tuple[Dict[str, List[Dict[str, Any]]], List[Dict[str, Any]]]:
    """
    Purpose:
        Generate direct comms links utilizing sockets
    Args:
        range_config: range config to generate against
        network_manager_request: requested links to generate against
        local_override: Ignore range config services. Does nothing ATM (may later impact
            enclaves and connectivity between nodes. standardizing between all exemplar
            channels for now to allow easier flexibility)
    Raises:
        Exception: generation fails
    Returns:
        link_addresses: configs for the generated links
        fulfilled_network_manager_request: which network manager links in the request were fufilled
    """

    if cli_args.local_override:
        logging.info("--local does nothing for Two Six Direct Channel at the moment")

    link_addresses = {}
    fulfilled_network_manager_request: Dict[str, Any] = {"links": []}

    # Get possible internode connectivity for building links
    internode_connectivity: Dict[
        str, Any
    ] = range_config_utils.get_full_internode_connectivity(range_config)

    # Get port mapping
    port_mapping = generate_port_mapping(range_config)

    # Build link properties
    (send_properties, receive_properties) = build_link_properties()

    # Parse through requested links to build possible links (mappings
    # need to account for port reusage)
    raw_link_addresses = {}
    for link_idx, requested_link in enumerate(network_manager_request["links"]):
        # Only create links for request for this channel
        if CHANNEL_ID not in requested_link["channels"]:
            continue
        requested_link["channels"] = [CHANNEL_ID]

        # Every link needs a sender and recipient
        if not requested_link["sender"]:
            raise Exception(f"Requested Link ({requested_link}) has no senders")
        if not requested_link["recipients"]:
            raise Exception(f"Requested Link ({requested_link}) has no recipients")

        # Channel cannot support multicast
        if len(requested_link["recipients"]) > 1:
            logging.warning(f"Direct link, cannot have multiple recipients")
            continue

        # Channel should not create links for
        sender_node = requested_link["sender"]
        recipient_node = requested_link["recipients"][0]
        if "client" in sender_node or "client" in recipient_node:
            logging.warning(f"Direct link, cannot connect clients")
            continue

        # Get the port for the socket
        socket_port = port_mapping.get(sender_node, -1)

        # TODO, need to figure out some way to verify that
        # the sender/recipient can communicate through the expected port.
        # also need to determine if we are using the real hostname or the
        # enclave hostname

        # Add the link as raw_link_profile. need to realign with the direct link
        # setup for ports
        raw_link_addresses.setdefault(recipient_node, {})
        raw_link_addresses[recipient_node].setdefault(socket_port, [])
        raw_link_addresses[recipient_node][socket_port].append(sender_node)

        # Append link as fufilled, the links will be created in the next step
        fulfilled_network_manager_request["links"].append(requested_link)

    # Create Links for the mapping. loop through each recipient node
    for recipient_node, sender_port_mapping in raw_link_addresses.items():

        if recipient_node not in link_addresses:
            link_addresses[recipient_node] = []

        # Get each group of senders by the send port
        for sender_port, sender_nodes in sender_port_mapping.items():

            # Create Receive Link. One for the port
            link_addresses[recipient_node].append(
                {
                    "role": "creator",
                    "personas": sender_nodes,
                    "address": f'{{"port" : {sender_port}, "hostname" : "{recipient_node}"}}',
                    "description": "link_type: receive",
                }
            )

            # Create Send Links, one for each sender
            # This ensures each sender doesn't know about the others
            for sender_node in sender_nodes:
                if sender_node not in link_addresses:
                    link_addresses[sender_node] = []
                link_profile = {
                    "role": "loader",
                    "personas": [recipient_node],
                    "address": f'{{"port" : {sender_port}, "hostname" : "{recipient_node}"}}',
                    "description": "link_type: send",
                }
                link_addresses[sender_node].append(link_profile)

    if len(fulfilled_network_manager_request["links"]) < len(network_manager_request["links"]):
        logging.warning(
            f"Not all links fufilled: {len(fulfilled_network_manager_request['links'])} of "
            f"{len(network_manager_request['links'])} fulfilled"
        )
    else:
        logging.info("All requested links fufilled")

    return (link_addresses, fulfilled_network_manager_request)


def build_link_properties() -> Tuple[Dict[str, Any], Dict[str, Any]]:
    """
    Purpose:
        Build the link properties for the link.

        These are hard-code guesses for initial links
    Args:
        N/A
    Returns:
        send_properties: properties for send side users (best, expected, worst)
        receive_properties: properties for recieve side users (best, expected, worst)
    """

    # Build link properties
    link_bandwidth_bps = 25_700_000  # tested with rib local mode against v1.0.0
    link_latency_ms = 16  # tested with rib local mode against v1.0.0
    link_reliability = False
    link_transmission_type = "unicast"
    send_properties = comms_link_utils.generate_link_properties_dict(
        "send",
        link_transmission_type,
        link_reliability,
        link_latency_ms,
        link_bandwidth_bps,
        supported_hints=[],
    )
    receive_properties = comms_link_utils.generate_link_properties_dict(
        "receive",
        link_transmission_type,
        link_reliability,
        link_latency_ms,
        link_bandwidth_bps,
        supported_hints=[],
    )

    return (send_properties, receive_properties)


def generate_port_mapping(
    range_config: Dict[str, Any], num_listening_ports: int = 2
) -> Dict[str, Any]:
    """
    Purpose:
        Generate which port each node will send on. we staticly define a node's send
        port so that each node can receive on multiple ports to balance traffic.
    Args:
        range_config: Dict of the range config. gets nodes and ports
        num_listening_ports: total number of listening ports
    Returns:
        port_mapping: Dict of the mapping between sender/recipient and
            which ports connect them
    """

    port_mapping = {}

    # Set data structures and base/template dicts
    server_socket_port_mapping: Dict[str, Any] = {}

    # Get all servers from range config
    range_config_servers = range_config_utils.get_server_details_from_range_config(
        range_config
    )

    # Create mapping
    for idx, race_server in enumerate(range_config_servers):
        port_mapping[race_server] = START_PORT + math.floor(idx % num_listening_ports)

    return port_mapping


def generate_channel_settings(range_config: Dict[str, Any]) -> Dict[str, Any]:
    """Generate the global settings for the channel.

    Args:
        range_config (Dict[str, Any]): The range config.

    Returns:
        Dict[str, Any]: The settings with string keys and values of various types. Intended to be written to file as json.
    """
    return {"settings": {}}


def generate_user_responses(
    range_config: Dict[str, Any],
) -> Dict[str, Dict[str, str]]:
    """
    Purpose:
        Generate the user input responses based on the range config
    Args:
        range_config: range config to generate against
    Return:
        Mapping of node persona to mapping of prompt to response
    """
    # Set the port range for dynamically created direct links.
    try:
        # Set the port range start so that it does not conflict with existing configured
        # ports.
        port_range_start = max(generate_port_mapping(range_config).values()) + 1
    except:
        # If that fails (e.g. no direct links have been configured) then just use the
        # global port range start.
        port_range_start = START_PORT
    # If there are no more ports available then fail loudly and early instead of having
    # to debug a potentially ambiguous failure in a deployment.
    if port_range_start >= END_PORT:
        raise Exception(
            f"Unable to allocate port range for dynamic direct links. No more ports available"
        )

    responses = {}

    range_config_clients = range_config_utils.get_client_details_from_range_config(
        range_config
    )
    for client_persona in range_config_clients.keys():
        responses[client_persona] = {
            "startPort": str(port_range_start),
            "endPort": str(END_PORT),
        }

    range_config_servers = range_config_utils.get_server_details_from_range_config(
        range_config
    )
    for server_persona in range_config_servers.keys():
        responses[server_persona] = {
            "startPort": str(port_range_start),
            "endPort": str(END_PORT),
        }

    return responses


###
# Helper Functions
###


def get_cli_arguments() -> argparse.Namespace:
    """
    Purpose:
        Parse CLI arguments for script
    Args:
        N/A
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
        "--config-dir",
        dest="config_dir",
        help="Where should configs be stored",
        required=False,
        default="./configs",
        type=str,
    )
    optional.add_argument(
        "--network-manager-request",
        dest="network_manager_request_file",
        help=(
            "Requested links from the network manager. Configs should generate only these"
            " links and as many of the links as possible. RiB local will not have the "
            "same network connectivty as the T&E range."
        ),
        required=False,
        default=False,
        type=str,
    )

    return parser.parse_args()


###
# Entrypoint
###


if __name__ == "__main__":

    LOG_LEVEL = logging.INFO
    logging.getLogger().setLevel(LOG_LEVEL)
    logging.basicConfig(
        stream=sys.stdout,
        level=LOG_LEVEL,
        format="[generate_comms_golang_direct_configs] %(asctime)s.%(msecs)03d %(levelname)s %(message)s",
        datefmt="%a, %d %b %Y %H:%M:%S",
    )

    try:
        main()
    except Exception as err:
        print(f"{os.path.basename(__file__)} failed due to error: {err}")
        raise err
