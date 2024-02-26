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
        Utilities for dealing with a network manager request for comms links
"""

# Python Library Imports
import sys
from typing import Any, Dict, List, Optional

# Local Lib Imports
from race_python_utils import range_config_utils


###
# Network Manager Request Functions
###


def generate_network_manager_request_from_range_config(
    range_config: Dict[str, Any],
    channel_properties_list: List[Dict[str, Any]],
    transmission_type: str,
    link_type: str,
) -> Dict[str, Any]:
    """
    Purpose:
        If no network manager request is passed in, generate one for indirect links.

        Transmission Type:
            - Unicast: one sender/recipient
            - Multicast: multi senders/recipients

        Link Type:
            - Direct: cannot include client<->server links.
            - Indirect: can include client<->server and server<->server links.

        The assumption that if there is no network manager request, then the comms plugin should
        create all possible links given the range config
    Args:
        range_config: range config to generate against
        transmission_type: type of transmission to prepare network manager request for. Changes the
            which links are possible
        link_type: type of link to prepare network manager request for. Changes the structure
            of the resulting links.
    Raises:
        Exception: generation fails
    Returns:
        network_manager_request: requested links based on the range config
    """

    # Verify Params
    valid_link_types = ("direct", "indirect")
    if link_type not in valid_link_types:
        raise Exception(f"{link_type} is not a valid link_type ({valid_link_types})")
    valid_transmission_types = ("unicast", "multicast")
    if transmission_type not in valid_transmission_types:
        raise Exception(
            f"{transmission_type} is not a valid transmission_type "
            f"({valid_transmission_types})"
        )

    network_manager_request = {}

    # Add S2S Requests
    if transmission_type == "unicast":
        network_manager_request = generate_s2s_unicast_network_manager_request_from_range_config(
            range_config, channel_properties_list
        )

        # Add C2S Requests (only when not direct)
        if link_type != "direct":
            c2s_unicast = generate_c2s_unicast_network_manager_request_from_range_config(
                range_config, channel_properties_list
            )

            if not network_manager_request:
                network_manager_request = c2s_unicast
            elif c2s_unicast:
                network_manager_request["links"].extend(c2s_unicast["links"])

    elif transmission_type == "multicast":
        network_manager_request = generate_s2s_multicast_network_manager_request_from_range_config(
            range_config, channel_properties_list
        )

        # Add C2S Requests (only when not direct)
        if link_type != "direct":
            c2s_multicast = generate_c2s_multicast_network_manager_request_from_range_config(
                range_config, channel_properties_list
            )

            if not network_manager_request:
                network_manager_request = c2s_multicast
            elif c2s_multicast:
                network_manager_request["links"].extend(c2s_multicast["links"])

    return network_manager_request


def generate_s2s_unicast_network_manager_request_from_range_config(
    range_config: Dict[str, Any], channel_properties_list: List[Dict]
) -> Dict[str, Any]:
    """
    Purpose:
        Generate all server to server requests from the range config based on
        unicast links
    Args:
        range_config: range config to generate against
    Raises:
        Exception: generation fails
    Returns:
        network_manager_request_s2s: requested links based on the range config for S2S connections
    """
    # TODO RACE2-1890 is just adding all channels to all links requests. Needs to be smarter in RACE2-1893.
    channel_list = []
    for channel in channel_properties_list:
        channel_list.append(channel.get("channelGid"))

    network_manager_request_s2s: Dict[str, Any] = {"links": []}

    # Get all servers from range config
    range_config_servers = range_config_utils.get_server_details_from_range_config(
        range_config
    )

    # Create unicast from all servers to all servers
    for send_server in range_config_servers:
        for recipient_server in range_config_servers:
            # No requests to send to oneself
            if send_server == recipient_server:
                continue

            network_manager_request_s2s["links"].append(
                {
                    "sender": send_server,
                    "recipients": [recipient_server],
                    "details": {},
                    "groupId": None,
                    "channels": channel_list,
                }
            )

    return network_manager_request_s2s


def generate_s2s_multicast_network_manager_request_from_range_config(
    range_config: Dict[str, Any], channel_properties_list: List[Dict]
) -> Dict[str, Any]:
    """
    Purpose:
        Generate all server to server requests from the range config based on
        multicast links
    Args:
        range_config: range config to generate against
    Raises:
        Exception: generation fails
    Returns:
        network_manager_request_s2s: requested links based on the range config for S2S connections
    """
    # TODO RACE2-1890 is just adding all channels to all links requests. Needs to be smarter in RACE2-1893.
    channel_list = []
    for channel in channel_properties_list:
        channel_list.append(channel.get("channelGid"))

    network_manager_request_s2s: Dict[str, Any] = {"links": []}

    # Get all servers from range config
    range_config_servers = range_config_utils.get_server_details_from_range_config(
        range_config
    )

    # Create link request from each server to all servers
    for range_config_server in range_config_servers:

        # Create the recipients and remove the sender from the list
        link_recipients = list(range_config_servers.keys())
        link_recipients.remove(range_config_server)

        network_manager_request_s2s["links"].append(
            {
                "sender": range_config_server,
                "recipients": link_recipients,
                "details": {},
                "groupId": None,
                "channels": channel_list,
            }
        )

    return network_manager_request_s2s


def generate_c2s_unicast_network_manager_request_from_range_config(
    range_config: Dict[str, Any], channel_properties_list: List[Dict]
) -> Dict[str, Any]:
    """
    Purpose:
        Generate all client to server and server to client requests
        from the range config based on unicast links
    Args:
        range_config: range config to generate against
        transmission_type: type of transmission to prepare network manager request for. Changes the
            which links are possible
    Raises:
        Exception: generation fails
    Returns:
        network_manager_request_c2s: requested links based on the range config for
            C2S/S2C connections
    """
    # TODO RACE2-1890 is just adding all channels to all links requests. Needs to be smarter in RACE2-1893.
    channel_list = []
    for channel in channel_properties_list:
        channel_list.append(channel.get("channelGid"))

    network_manager_request_c2s: Dict[str, Any] = {"links": []}

    # Get clients and servers from range config
    range_config_clients = range_config_utils.get_client_details_from_range_config(
        range_config
    )
    range_config_servers = range_config_utils.get_server_details_from_range_config(
        range_config
    )

    # Create unicast from all servers to all clients
    for send_server in range_config_servers:
        for recipient_client in range_config_clients:
            network_manager_request_c2s["links"].append(
                {
                    "sender": send_server,
                    "recipients": [recipient_client],
                    "details": {},
                    "groupId": None,
                    "channels": channel_list,
                }
            )

    # Create unicast from all clients to all servers
    for send_client in range_config_clients:
        for recipient_server in range_config_servers:
            network_manager_request_c2s["links"].append(
                {
                    "sender": send_client,
                    "recipients": [recipient_server],
                    "details": {},
                    "groupId": None,
                    "channels": channel_list,
                }
            )

    return network_manager_request_c2s


def generate_c2s_multicast_network_manager_request_from_range_config(
    range_config: Dict[str, Any], channel_properties_list: List[Dict]
) -> Dict[str, Any]:
    """
    Purpose:
        Generate all client to server and server to client requests
        from the range config based on unicast links
    Args:
        range_config: range config to generate against
        transmission_type: type of transmision to prepare network manager request for. Changes the
            which links are possible
    Raises:
        Exception: generation fails
    Returns:
        network_manager_request_c2s: requested links based on the range config for
            C2S/S2C connections
    """
    # TODO RACE2-1890 is just adding all channels to all links requests. Needs to be smarter in RACE2-1893.
    channel_list = []
    for channel in channel_properties_list:
        channel_list.append(channel.get("channelGid"))

    network_manager_request_c2s: Dict[str, Any] = {"links": []}

    # Get clients and servers from range config
    range_config_clients = range_config_utils.get_client_details_from_range_config(
        range_config
    )
    range_config_servers = range_config_utils.get_server_details_from_range_config(
        range_config
    )

    # Create link request from each client to all servers
    for range_config_client in range_config_clients:
        network_manager_request_c2s["links"].append(
            {
                "sender": range_config_client,
                "recipients": list(range_config_servers.keys()),
                "details": {},
                "groupId": None,
                "channels": channel_list,
            }
        )

    # Create link request from each server to all clients
    for range_config_server in range_config_servers:
        network_manager_request_c2s["links"].append(
            {
                "sender": range_config_server,
                "recipients": list(range_config_clients.keys()),
                "details": {},
                "groupId": None,
                "channels": channel_list,
            }
        )

    return network_manager_request_c2s


def validate_network_manager_request(
    network_manager_request: Dict[str, Any],
    range_config: Dict[str, Any],
) -> None:
    """
    Purpose:
        validate a network manager request of links

        check that the request isn't wildly inaccurate to the
        range config. e.g. network manager should not have requested links
        for nodes that don't exist.
    Args:
        network_manager_request: requested links to validate
        range_config: range config to validate against
    Raises:
        Exception: if network-manager-request is invalid (for whatever reason)
    Returns:
        N/A
    """

    # validate top level structure
    if not network_manager_request.get("links"):
        raise Exception(f"Network Manager Link Request missing links key")

    # Parse out race_node details
    range_config_race_nodes = []
    range_config_clients = range_config_utils.get_client_details_from_range_config(
        range_config
    )
    range_config_race_nodes.extend(list(range_config_clients.keys()))
    range_config_registries = range_config_utils.get_registry_details_from_range_config(
        range_config=range_config,
    )
    range_config_race_nodes.extend(list(range_config_registries.keys()))
    range_config_servers = range_config_utils.get_server_details_from_range_config(
        range_config
    )
    range_config_race_nodes.extend(list(range_config_servers.keys()))

    # Validate the links are for expected clients
    invalid_send_nodes_in_request = set()
    invalid_recipient_nodes_in_request = set()
    for requested_link in network_manager_request["links"]:

        # Every link needs a sender and recipient
        if not requested_link.get("sender"):
            raise Exception(f"Requested Link ({requested_link}) has no senders")
        if not requested_link.get("recipients"):
            raise Exception(f"Requested Link ({requested_link}) has no recipients")

        # Validate sender/recipients in range-config
        if requested_link["sender"] not in range_config_race_nodes:
            invalid_send_nodes_in_request.add(requested_link["sender"])
        for recipient in requested_link["recipients"]:
            if recipient not in range_config_race_nodes:
                invalid_recipient_nodes_in_request.add(recipient)

    if invalid_send_nodes_in_request:
        raise Exception(
            "Invalid nodes found in network manager link request senders: "
            f"{', '.join(invalid_send_nodes_in_request)}"
        )

    if invalid_recipient_nodes_in_request:
        raise Exception(
            "Invalid nodes found in network manager link request recipients: "
            f"{', '.join(invalid_recipient_nodes_in_request)}"
        )
