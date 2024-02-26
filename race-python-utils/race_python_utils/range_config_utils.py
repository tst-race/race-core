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
        Utilities for interacting with a range config
"""

import socket

# Python Library Imports
from typing import Any, Dict, Optional


###
# Range Config Functions
###


def get_client_details_from_range_config(
    range_config: Dict[str, Any],
    genesis: Optional[bool] = None,
) -> Dict[str, Any]:
    """
    Purpose:
        parse clients from a range_config
    Args:
        range_config: range config to parse
        genesis: Genesis value (True or False) on which to filter
            (default is to include all nodes regardless of genesis value)
    Returns:
        clients: clients in the range config
    """

    clients = {}
    for race_node in range_config["range"]["RACE_nodes"]:
        if "client" in race_node.get("type", "").lower():
            if genesis is None or race_node.get("genesis", True) == genesis:
                clients[race_node["name"]] = race_node

    return clients


def get_registry_details_from_range_config(
    range_config: Dict[str, Any],
    genesis: Optional[bool] = None,
) -> Dict[str, Any]:
    """
    Purpose:
        parse registries from a range_config
    Args:
        range_config: range config to parse
        genesis: Genesis value (True or False) on which to filter
            (default is to include all nodes regardless of genesis value)
    Returns:
        registries: registries in the range config
    """
    registries = {}
    for race_node in range_config["range"]["RACE_nodes"]:
        if "registry" in race_node.get("type", "").lower():
            if genesis is None or race_node.get("genesis", True) == genesis:
                registries[race_node["name"]] = race_node

    return registries


def get_server_details_from_range_config(
    range_config: Dict[str, Any],
    genesis: Optional[bool] = None,
) -> Dict[str, Any]:
    """
    Purpose:
        parse servers from a range_config
    Args:
        range_config: range config to parse
        genesis: Genesis value (True or False) on which to filter
            (default is to include all nodes regardless of genesis value)
    Returns:
        servers: servers in the range config
    """

    servers = {}
    for race_node in range_config["range"]["RACE_nodes"]:
        if "server" in race_node.get("type", "").lower():
            if genesis is None or race_node.get("genesis", True) == genesis:
                servers[race_node["name"]] = race_node

    return servers


def get_service_from_range_config(
    range_config: Dict[str, Any],
    service_name: str,
) -> Dict[str, Any]:
    """
    Purpose:
        parse the range config to get the expected service. Parse
        the data into a usable form

        Note, might not be present, should not error
    Args:
        range_config: range config to parse
    Returns:
        service: Two Six Whiteboard details in the range config
    """

    service: Dict[str, Any] = {}
    for race_service in range_config["range"]["services"]:

        # Skip non-needed services
        if race_service.get("type", "") != service_name:
            continue

        # Check if service is authenticated, look for keys with "auth" and not anon
        authenticated_service = False
        for service_key, service_value in race_service.items():
            if "auth" in service_key and service_value != "anonymous":
                authenticated_service = True

        # A service can have multiple ways to access, parse and pick one (https pref)
        for service_access in race_service["access"]:

            service_url = service_access["url"]
            service_protocol = service_access["protocol"]
            if ":" in service_url:
                service_hostname = service_url.split(":")[0]
                service_port = int(service_url.split(":")[-1])
            else:
                service_hostname = service_url
                service_port = guess_port_by_protocol(service_protocol)

            if not service or (  # Take first protocol  # Or Overwrite  http with https
                service["protocol"] == "http" and service_protocol == "https"
            ):
                service = {
                    "name": service_name,
                    "url": service_url,
                    "protocol": service_protocol,
                    "hostname": service_hostname,
                    "port": service_port,
                    "authenticated_service": authenticated_service,
                }

    return service


def validate_range_config(
    range_config: Dict[str, Any],
    allow_no_clients: bool = False,
    allow_non_genesis_servers: bool = False,
) -> None:
    """
    Purpose:
        validate a range_config
    Args:
        range_config: range config to validate
        allow_no_clients: whether or not to allow no clients in range config
        allow_non_genesis_servers: whether or not to allow non-genesis servers in range config
    Raises:
        Exception: if range-config is invalid (for whatever reason)
    Returns:
        N/A
    """

    # validate top level structure
    expected_range_keys = [
        "RACE_nodes",
        "enclaves",
        "services",
    ]
    for range_key in expected_range_keys:
        if range_config["range"].get(range_key, None) is None:
            raise Exception(f"Range Config missing range.{range_key} key")

    # Validate clients/servers exist
    if not get_client_details_from_range_config(range_config) and not allow_no_clients:
        raise Exception("No clients found in range config")
    servers = get_server_details_from_range_config(range_config)
    if not servers:
        raise Exception("No servers found in range config")

    if not allow_non_genesis_servers:
        if any([not node.get("genesis", True) for node in servers.values()]):
            raise Exception("Non-genesis servers found in range config")

    # Validate Enclaves (existance)
    if not range_config["range"].get("enclaves"):
        raise Exception("No enclaves found in range config")

    # TODO, more range config checking


###
# Parse Connectivity Functions
###


def get_full_internode_connectivity(range_config: Dict[str, Any]) -> Dict[str, Any]:
    """
    Purpose:
        Example of how to get interconnectivity of all nodes
    Args:
        range_config: range config (physical network description)
    Raises:
        Exception: TBD
    Returns:
        full_internode_connectivity: Dict of connectivity. key is sender, value
            is all of the possible receivers
    """

    full_internode_connectivity: Dict[str, Any] = {}

    race_nodes = range_config.get("range", {}).get("RACE_nodes", {})
    if not race_nodes:
        raise Exception("No race_nodes found in range_config")

    for sender_node in race_nodes:

        full_internode_connectivity.setdefault(sender_node["name"], {})

        for recipient_node in race_nodes:
            if sender_node == recipient_node:
                continue

            full_internode_connectivity[sender_node["name"]][
                recipient_node["name"]
            ] = get_internode_connectivity(
                range_config, sender_node["name"], recipient_node["name"]
            )

    return full_internode_connectivity


def get_race_node_by_name(
    range_config: Dict[str, Any],
    race_node_name: str,
) -> Dict[str, Any]:
    """
    Purpose:
        Example of how to get a race node by name
    Args:
        range_config: range config (physical network description)
        race_node_name: node to get details for
    Raises:
        Exception: TBD
    Returns:
        race_node: Dict of the race node
    """

    race_node = None

    race_nodes = range_config.get("range", {}).get("RACE_nodes", {})
    if not race_nodes:
        raise Exception("No race_nodes found in range_config")

    for race_node_details in race_nodes:
        if race_node_details["name"] == race_node_name:
            race_node = race_node_details
            break

    if not race_node:
        raise Exception(f"{race_node} not found in range_config")

    return race_node


def get_enclave_by_name(
    range_config: Dict[str, Any],
    enclave_name: str,
) -> Dict[str, Any]:
    """
    Purpose:
        Example of how to get a race node by name
    Args:
        range_config: range config (physical network description)
        enclave_name: enclave to get details for
    Raises:
        Exception: TBD
    Returns:
        enclave: Dict of the enclave
    """

    enclave = None

    enclaves = range_config.get("range", {}).get("enclaves", {})
    if not enclaves:
        raise Exception("No enclaves found in range_config")

    for enclave_details in enclaves:
        if enclave_details["name"] == enclave_name:
            enclave = enclave_details
            break

    if not enclave:
        raise Exception(f"{enclave} not found in range_config")

    return enclave


def get_nodes_per_enclave(range_config: Dict[str, Any]) -> Dict[str, Any]:
    """
    Purpose:
        Example of how to get nodes in an enclave
    Args:
        range_config: range config (physical network description)
    Raises:
        Exception: range_config is not valid
        Exception: no enclaves in the range_config
        Exception: no race_nodes in the range_config
        Exception: mismatch between enclaves and nodes
    Returns:
        nodes_per_enclave: Dict of enclaves with nodes in each
    """

    # Creating mapping
    nodes_per_enclave: Dict[str, Any] = {}

    # Getting base from range_config
    race_nodes = range_config.get("range", {}).get("RACE_nodes", {})
    enclaves = range_config.get("range", {}).get("enclaves", {})

    if not enclaves:
        raise Exception("No enclaves found in range_config")
    if not race_nodes:
        raise Exception("No race_nodes found in range_config")

    for enclave in enclaves:
        nodes_per_enclave.setdefault(enclave["name"], [])

    for race_node in race_nodes:
        if race_node["enclave"] not in nodes_per_enclave:
            raise Exception(
                f"Invalid range-config, {race_node['enclave']} is a node's"
                "enclave but is not listed in enclaves"
            )
        nodes_per_enclave[race_node["enclave"]].append(race_node["name"])

    return nodes_per_enclave


def get_internode_connectivity(
    range_config: Dict[str, Any],
    send_node_name: str,
    receive_node_name: str,
) -> Dict[str, Any]:
    """
    Purpose:
        Example of how to get connectivity between two nodes
    Args:
        range_config: range config (physical network description)
        send_node_name: node who wants to send
        receive_node_name: node who wants to receive
    Raises:
        Exception: If the send node is missing
        Exception: If the receipient node is missing
    Returns:
        nodes_per_enclave: Dict of enclaves with nodes in each
    """

    # Creating mapping
    internode_connectivity = {}
    send_node = None
    send_enclave = None
    receive_node = None
    receive_enclave = None

    # Getting nodes and enclaves
    send_node = get_race_node_by_name(range_config, send_node_name)
    send_enclave = get_enclave_by_name(range_config, send_node["enclave"])
    receive_node = get_race_node_by_name(range_config, receive_node_name)

    nodes_per_enclave: Dict[str, Any] = get_nodes_per_enclave(range_config)
    for enclave, race_nodes_in_enclave in nodes_per_enclave.items():
        if receive_node_name in race_nodes_in_enclave:
            receive_enclave = get_enclave_by_name(range_config, enclave)
            break

    if not send_enclave:
        raise Exception(f"enclave for send node not found not found in range_config")
    if not receive_enclave:
        raise Exception(
            f"enclave for recipient node not found not found in range_config"
        )

    # Check if sender and receiver share an enclave or the receiver
    # is not natted
    if receive_enclave == send_enclave:
        internode_connectivity = {"hostname": receive_node["name"], "ports": {"*": "*"}}
    else:
        # Note, we initially were going to set the ip to the router's ip, but APL is
        # updating all hosts to be routable, even if they are behind a nat (just wont
        # have open ports)
        hostname = receive_node["name"]

        # if in different enclaves and receiver is natted, check for port
        # forwarding rules to ensure sender can reach receiver
        possible_connective_ports = {
            external_port: port_details["port"]
            for external_port, port_details in receive_enclave["port_mapping"].items()
            if receive_node["name"] in port_details["hosts"]
        }

        if possible_connective_ports:
            internode_connectivity = {
                "hostname": hostname,
                "ports": possible_connective_ports,
            }

    return internode_connectivity


###
# Network
###


def guess_port_by_protocol(protocol: str) -> Optional[int]:
    """
    Purpose:
        Guess a port number from a specified protocol name. Will use a common protocol
        lookup mapping to see what port that protocol is usually found on.

        e.g. guess_port_by_protocol("ssh") == 22
    Args:
        protocol: Name of comon protocol
    Return:
        port_guess: Port number if one is found, if one is known
    """

    # Build Known Protocol Table
    protocol_table = {
        name[8:].lower(): num
        for name, num in vars(socket).items()
        if name.startswith("IPPROTO")
    }
    protocol_table["ssh"] = 22
    protocol_table["http"] = 80
    protocol_table["https"] = 443

    return protocol_table.get(protocol, None)
