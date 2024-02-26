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
        Network Manager Utilities for interacting with network manager code, configs, etc
"""

# Python Library Imports
import copy
import json
import math
import networkx as nx
import os
from typing import Dict, Any, Tuple, List, Set, Optional

# Local Python Library Imports
from race_python_utils import range_config_utils
from race_python_utils.network_manager import Client, Committee, Server


###
# Persona Functions
###


def generate_personas_config_from_range_config(
    range_config: Dict[str, Any]
) -> List[Dict[str, Any]]:
    """
    Purpose:
        Generate Personas that will be used by Network Manager Plugins from a
        range config object
    Args:
        range_config (Dict): Dict of the range config
    Returns:
        race_personas (List of Dicts): List of Personas to Store
    """

    # Set data structures and base/template dicts
    race_personas = []
    base_race_client_persona = {
        "displayName": "RACE Client {node_id}",
        "raceUuid": "race-client-{node_id}",
        "publicKey": "{node_id}",
        "personaType": "client",
        "aesKeyFile": "./race-client-{node_id}.aes",
    }
    base_race_registry_persona = {
        "displayName": "RACE Registry {node_id}",
        "raceUuid": "race-registry-{node_id}",
        "publicKey": "{node_id}",
        "personaType": "registry",
        "aesKeyFile": "./race-registry-{node_id}.aes",
    }
    base_race_server_persona = {
        "displayName": "RACE Server {node_id}",
        "raceUuid": "race-server-{node_id}",
        "publicKey": "{node_id}",
        "personaType": "server",
        "aesKeyFile": "./race-server-{node_id}.aes",
    }

    # Get genesis clients, registries, and servers from range config
    range_config_clients = range_config_utils.get_client_details_from_range_config(
        range_config=range_config,
        genesis=True,
    )
    range_config_registries = range_config_utils.get_registry_details_from_range_config(
        range_config=range_config,
        genesis=True,
    )
    range_config_servers = range_config_utils.get_server_details_from_range_config(
        range_config=range_config,
        genesis=True,
    )

    for range_config_client in range_config_clients:

        # Get Node ID
        node_id = range_config_client.split("-")[-1].zfill(5)
        public_key = node_id

        # Build Node Config
        node_config = copy.deepcopy(base_race_client_persona)
        node_config["displayName"] = node_config["displayName"].format(node_id=node_id)
        node_config["raceUuid"] = node_config["raceUuid"].format(node_id=node_id)
        node_config["publicKey"] = str(public_key)
        node_config["aesKeyFile"] = node_config["aesKeyFile"].format(node_id=node_id)

        race_personas.append(node_config)

    for range_config_registry in range_config_registries:

        # Get Node ID
        node_id = range_config_registry.split("-")[-1].zfill(5)
        public_key = node_id

        # Build Node Config
        node_config = copy.deepcopy(base_race_registry_persona)
        node_config["displayName"] = node_config["displayName"].format(node_id=node_id)
        node_config["raceUuid"] = node_config["raceUuid"].format(node_id=node_id)
        node_config["publicKey"] = str(public_key)
        node_config["aesKeyFile"] = node_config["aesKeyFile"].format(node_id=node_id)

        race_personas.append(node_config)

    for range_config_server in range_config_servers:

        # Get Node ID
        node_id = range_config_server.split("-")[-1].zfill(5)
        public_key = node_id

        # Build Node Config
        node_config = copy.deepcopy(base_race_server_persona)
        node_config["displayName"] = node_config["displayName"].format(node_id=node_id)
        node_config["raceUuid"] = node_config["raceUuid"].format(node_id=node_id)
        node_config["publicKey"] = str(public_key)
        node_config["aesKeyFile"] = node_config["aesKeyFile"].format(node_id=node_id)

        race_personas.append(node_config)

    return race_personas


###
# AES Key Functions
###


def generate_aes_keys_from_range_config(range_config: Dict[str, Any]) -> Dict[str, Any]:
    """
    Purpose:
        Generate AES Keys for each Persona that will be used by Network Manager Plugins

        Note: Doing symmetric AES 256 keys ATM
    Args:
        range_config (Dict): Dict of the race config
    Returns:
        aes_keys (Dict): Dict of AES keys per persona to store
    """

    # Key settings
    block_size = 32
    # iv = b"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"  # not used at the moments

    aes_keys = {}

    # Get all clients and servers from range config
    range_config_clients = range_config_utils.get_client_details_from_range_config(
        range_config=range_config,
        genesis=True,
    )
    range_config_registries = range_config_utils.get_registry_details_from_range_config(
        range_config=range_config,
        genesis=True,
    )
    range_config_servers = range_config_utils.get_server_details_from_range_config(
        range_config=range_config,
        genesis=True,
    )

    for range_config_client in range_config_clients:
        aes_keys[range_config_client] = os.urandom(block_size)
    for range_config_registry in range_config_registries:
        aes_keys[range_config_registry] = os.urandom(block_size)
    for range_config_server in range_config_servers:
        aes_keys[range_config_server] = os.urandom(block_size)

    return aes_keys


###
# Committee Functions
###


def generate_flat_committees_from_reachability(
    range_config: Dict[str, Any],
    desired_size: int = None,
    flooding_factor: int = 2,
    num_rings: int = 2,
    diff_entrance_exit: bool = False,
) -> Dict[str, Committee]:
    """
    Purpose:
        Creates committees based on K-connected-component analysis of a undirected version
            of the range-config graph. This should maximize the connectivity of each
            committee while attempting to respect the desired_size. This method may not
            work well on sparser reachability graphs, particularly those with many
            unidirectional edges.

    Args:
        range_config (Dict): Dict of the race config
        desired_size (Int): the desired committee size - generated committees will not be
            larger than 2*desired_size and will not be smaller than desired_size
        flooding_factor (Int): the flooding factor to set for the deployment (0 means
            maximum flooding)
        num_rings (Int): the number of ring paths to generate for each committee.
            This will cause this many simultaneous traversals of the committee, so more
            than 2 should be used with caution.
         diff_entrance_exit (Bool): whether to force client entrance and exit committees
            to be different
    Returns:
        committess (Dict): mappings of name -> Committee containing references to the
            input Client and Server objects
    """

    # Set Base Structures
    (servers, clients) = load_nodes_from_range_config(range_config)
    committees: Dict[str, Committee] = {}

    # Create nx graph from reachability
    graph = nx.DiGraph()
    for server in servers.values():
        for reachable in server.reachable_servers:
            graph.add_edge(server.name, reachable.name)

    if desired_size is None:
        desired_size = max(1, int(math.log(len(graph.nodes), 2)))

    # Assign servers to committees
    committee_nodelists = kcomp_draft(graph, desired_size)
    for idx, nodelist in enumerate(committee_nodelists):
        name = f"committee-{idx}"
        new_committee = Committee(name, flooding_factor=flooding_factor)
        for server in [servers[server_name] for server_name in nodelist]:
            new_committee.servers.append(server)
            server.committee = new_committee

        committees[name] = new_committee

    # Assign clients to committees
    committee_idx = 0
    reachable_idx = 0
    committee_keys = list(committees.keys())
    for client in clients.values():
        # If this client can reach a server, use as the basis for choosing a committee
        if len(client.reachable_servers) > 0:
            assign_client(
                committees,
                client.reachable_servers[
                    reachable_idx % len(client.reachable_servers)
                ].committee.name,
                client,
                diff_entrance_exit,
            )
            reachable_idx += 1
        else:
            assign_client(
                committees, committee_keys[committee_idx], client, diff_entrance_exit
            )
        committee_idx = (committee_idx + 1) % len(committee_keys)

    # Committee is now fully initialized, trigger ring generation
    for committee in committees.values():
        committee.generate_rings(num_rings)

    return committees


###
# Helper Functions
###


def load_nodes_from_range_config(
    range_config: Dict[str, Any]
) -> Tuple[Dict[str, Server], Dict[str, Client]]:
    """
    Purpose:
        Create internal Client and Server structures for each client and server node
        listed in the range-config. Reads reachability from the range-config as well.

    Args:
        range_config (Dict): Dict of the race config
    Returns:
        servers: server name to Server obj Mapping
        clients: client name to Client obj Mapping
    """

    # Initialize all the nodes

    # Get genesis clients, registries and servers from range config
    range_config_clients = range_config_utils.get_client_details_from_range_config(
        range_config=range_config,
        genesis=True,
    )
    range_config_registries = range_config_utils.get_registry_details_from_range_config(
        range_config=range_config,
        genesis=True,
    )
    range_config_servers = range_config_utils.get_server_details_from_range_config(
        range_config=range_config,
        genesis=True,
    )

    # Build Client/Server Mapping for
    servers = {}
    clients = {}

    for range_config_client in range_config_clients:
        clients[range_config_client] = Client(range_config_client)
    for range_config_registry in range_config_registries:
        # Registries are a subclass of clients
        clients[range_config_registry] = Client(range_config_registry)
    for range_config_server in range_config_servers:
        servers[range_config_server] = Server(range_config_server)

    # Get Direct Reachability
    full_internode_connectivity = range_config_utils.get_full_internode_connectivity(
        range_config
    )

    # Set direct reachability in objects
    for name, reachable_details in full_internode_connectivity.items():
        if name not in clients and name not in servers:
            continue  # Ignore non-genesis nodes
        node = clients[name] if name in clients else servers[name]
        for reachable, reachable_detail in reachable_details.items():
            if not reachable_detail:
                continue  # Ignore node with no path
            if reachable == node.name:
                continue  # ignore self-reachability
            if reachable not in clients and reachable not in servers:
                continue  # Ignore non-genesis nodes
            if reachable in clients:
                if isinstance(node, Client):
                    continue  # ignore client-client reachability
                node.reachable_clients.append(clients[reachable])
            else:
                node.reachable_servers.append(servers[reachable])

    return servers, clients


def assign_client(
    committees: Dict[str, Committee],
    committee_name: str,
    client: Client,
    diff_entrance_exit=False,
):
    """
    Purpose:
        Assign a client to the named committee as its exit committee, if
            diff_entrance_exit is False then also assign that as its entrance committee,
            else assign the next committee in the committees dictioanary as the entrance
            committee. Assumes clients can reach these committees, the caller should
            ensure this.

    Args:
        committees (Dict): name -> Committee mapping
        committee_name (String): the committee to assign as exit (and entrance,
            if not diff_entrance_exit)
        client (Client): the Client to assign
        diff_entrance_exit (Bool): whether to assign the same committee as both entrance
            and exit
    Returns:
        N/A: modifies committees and client objects
    """
    client.exit_committee = [server for server in committees[committee_name].servers]

    if diff_entrance_exit:
        sorted_committees = list(sorted(committees.keys()))
        # ensure the entrance committee is different if >1 committees total
        committee_idx = sorted_committees.index(committee_name) + 1 % len(committees)
        entrance_committee = committees[sorted_committees[committee_idx]]
        client.entrance_committee = [server for server in entrance_committee.servers]
    else:
        client.entrance_committee = client.exit_committee

    committees[committee_name].clients.append(
        client
    )  # this MUST align with EXIT committee
    for server in committees[committee_name].servers:
        server.exit_clients.append(client)


def kcomp_split(
    graph: nx.Graph, oversized_comp: List, desired_size: int
) -> Optional[Set[str]]:
    """
    Purpose:
        Splits the oversized component into a connected subgraph of desired_size and
            returns the members of that subgraph.

    Args:
        graph (nx.Graph): the overall graph for connectivity
        oversized_comp (List): the list of nodes comprising the component to split
        desired_size (Int): the desired size of the component
    Returns:
        comp (Set): the set of nodes in the graph that make up the split component
    """  # Build node by traversing neighbors to ensure a connected component
    subGraph = graph.subgraph(oversized_comp)
    start_node = list(subGraph.nodes)[0]
    comp = set([start_node])
    new_members = [start_node]
    queue: List[str] = []
    while len(comp) < desired_size:
        if len(queue) == 0:
            if len(new_members) > 0:
                node = new_members.pop(0)
                queue += [n for n in nx.neighbors(graph, node)]
            else:
                return None
        if queue[0] not in comp:
            node = queue.pop(0)
            comp.add(node)
            new_members.append(node)
        else:
            queue.pop(0)

    return comp


def kcomp_draft_helper(
    graph: nx.Graph,
    desired_size: int,
    allow_clean_split: bool = False,
    allow_messy_split: bool = False,
) -> Optional[Set[str]]:
    """
    Purpose:
        Helper function to draft a single component from the graph of the desired size.
            Iterates through K-connected-components from most- to least-connected to create
            the most connected committees possible.
        If allow_clean_split is True then the algorithm will split a component larger than
            desired_size iff it is a multiple of the desired size (so that a future
            iteration can draft the remainder).
        If allow_messy_split is True then the algorithm will split a component larger than
             desired_size regardless of how their sizes relate.
        If no components can be found, None is returned

    Args:
        graph (nx.Graph): the graph to operate on
        desired_size (Int): the desired size of a committee
        allow_clean_split (Bool): whether to allow the drafting to split a component cleanly
        allow_messy_split (Bool): whether to allow the drafting to split a component
    Returns:
        component (Set): Returns the component, a set of nodes
    """
    if len(graph) == 0:
        return None

    try:
        # NOTE: selecting shortest_augmenting_path here because it is better for dense graphs
        comps = nx.k_components(
            graph, flow_func=nx.algorithms.flow.shortest_augmenting_path
        )
    except ValueError:  # networkx function throws error on graphs with no components
        return None

    if len(comps) == 0:
        return None

    clean_split_candidates = []
    messy_split_candidates = []

    for k in range(max(comps.keys()), 0, -1):
        for comp in comps[k]:
            if len(comp) == desired_size:
                return comp

            if len(comp) > desired_size:
                messy_split_candidates.append(comp)
                if len(comp) % desired_size == 0:
                    clean_split_candidates.append(comp)

    if allow_clean_split and len(clean_split_candidates) > 0:
        return kcomp_split(graph, clean_split_candidates[0], desired_size)

    if (allow_messy_split or allow_clean_split) and len(messy_split_candidates) > 0:
        return kcomp_split(graph, messy_split_candidates[0], desired_size)

    return None


def attach_node(
    node: str, committees: List[Set[str]], kcomps: nx.Graph, fresh_graph: nx.Graph
) -> List[Set[str]]:
    """
    Purpose:
        Attach the node to the smallest committee of those with the highest K-connectivity
            that contain this node. So, prioritize connectivity, but then try to balance
            committee sizes if choices remain.

    Args:
        node (String): node to attach
        committees (List): list of committees
        kcomps (nx.Graph): k-connected-components of the graph
        fresh_graph (nx.Graph): underlying graph
    Returns:
        committees (List): committees with node attached (if possible)
    """
    if len(kcomps) == 0:
        return committees

    sorted_committees = sorted(committees, key=lambda c: len(c))
    for idx, committee in enumerate(sorted_committees):
        if any([fresh_graph.has_edge(node, member) for member in committee]) and any(
            [fresh_graph.has_edge(member, node) for member in committee]
        ):
            for k in range(max(kcomps.keys()), 0, -1):
                for kcomp in kcomps[k]:
                    if node in kcomp:
                        if committee.issubset(kcomp):
                            committee.add(node)
                            return sorted_committees

    return committees


def undirectionalize(orig_graph: nx.DiGraph):
    """
    Purpose:
        Makes a directed graph into an undirected graph by pruning edges without reverses.
        If the input graph was already undirected, it returns a copy

    Args:
        orig_graph (nx.DiGraph): a directed graph
    Returns:
        graph (nx.Graph): undirected graph version of orig_graph
    """
    if isinstance(orig_graph, nx.DiGraph):
        graph = nx.Graph()
        for edge in orig_graph.edges:
            if (edge[1], edge[0]) in orig_graph.edges:
                graph.add_edge(edge[0], edge[1])
    else:
        graph = nx.Graph(orig_graph)

    return graph


def kcomp_draft(orig_graph: nx.DiGraph, desired_size: int):
    """
    Purpose:
        Attempts to build committees of desired_size from an undirectionalized version of
            orig_graph using K-connected-component drafting.

    Args:
        orig_graph (nx.DiGraph): a directed graph representing reachability and using the
            server namesas the nodes
        desired_size (Int): the desired size of a committee
    Returns:
        committees (List): List of sets of nodes (server names) representing committees
    """
    committees = []
    undrafted_graph = undirectionalize(orig_graph)
    while True:
        drafted = kcomp_draft_helper(undrafted_graph, desired_size)
        if drafted is None:  # clean comp->committee mappings are gone
            # Try partially-inhabiting larger components
            drafted = kcomp_draft_helper(
                undrafted_graph, desired_size, allow_messy_split=True
            )
            if drafted is None:  # Components large enough for a committee are gone
                break

        committees.append(drafted)
        for node in drafted:
            undrafted_graph.remove_node(node)

    # Clean up undrafted nodes by adding them to committees in their component
    fresh_graph = undirectionalize(orig_graph)
    # NOTE: selecting shortest_augmenting_path here because it is better for dense graphs
    kcomps = nx.k_components(
        fresh_graph, flow_func=nx.algorithms.flow.shortest_augmenting_path
    )
    for node in undrafted_graph.nodes:
        committees = attach_node(node, committees, kcomps, fresh_graph)

    return committees


def analyze_committees(committees: Dict[str, Committee]):
    """
    Purpose:
        Analyze the connectedness of the committees
    Args:
        committees (List): list of committees
    Returns:
        N/A
    """

    digraph = nx.DiGraph()
    for src in committees.values():
        reachable = src.gather_reachable_committees()
        for dst in reachable:
            digraph.add_edge(src, dst)

    graph = undirectionalize(digraph)
    flooding_factor = list(committees.values())[0].flooding_factor
    if (
        max([v for k, v in digraph.out_degree()]) > flooding_factor
        and flooding_factor > 0
    ):
        # Danger that some committee might never be notified if committee graph is not complete
        if min([v for k, v in nx.degree(graph)]) < len(graph) - 1:
            print(
                "WARNING: committee graph is not a _complete_ graph and flooding-factor is less than the out-degree of some committees. It is possible messages could be lost during routing."
            )

    if not nx.is_strongly_connected(digraph):
        raise Exception(
            "Committee graph was not strongly connected, try changing the --committee-size parameter."
        )
