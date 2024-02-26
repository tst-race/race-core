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
        RACE Network Vizualizer. As a network manager plugin, load configs
        to visualize the RACE network as configured.
"""

# Python Library Imports
import copy
import json
import logging
import matplotlib.pyplot as plt
import networkx as nx
import os
import statistics
import sys

# Path Config
BASE_DEPLOYMENT_PATH = f"{os.path.dirname(os.path.realpath(__file__))}"
sys.path.insert(0, BASE_DEPLOYMENT_PATH)


###
# Build Graph Functions
###


def build_network_graph(race_persona_configs, race_committee_configs):
    """
    Purpose:
        Build Network Graph from Configs
    Args:
        race_persona_configs (List of Dicts): Dict of RACE Personas
        race_committee_configs (Dict): Dicts of Committee configs
    Returns:
        race_graph (Graph Obj): RACE Network Graph Object
    """
    logging.info("Building Network Graph")

    race_graph = nx.MultiDiGraph()
    for race_persona_config in race_persona_configs:
        if race_persona_config["personaType"] == "client":
            race_graph.add_node(race_persona_config["raceUuid"], nodeType="client")
        elif race_persona_config["personaType"] == "server":
            race_graph.add_node(race_persona_config["raceUuid"], nodeType="server")

    network_edges = []
    for race_persona, race_committee_config in race_committee_configs.items():

        committe_keys = []
        for race_persona_config in race_persona_configs:
            if race_persona == race_persona_config["raceUuid"]:
                if race_persona_config["personaType"] == "client":
                    committe_keys = ["entranceCommittee", "exitCommittee"]
                elif race_persona_config["personaType"] == "server":
                    committe_keys = [
                        "reachableClients",
                        "reachableIntraCommitteeServers",
                        "reachableInterCommitteeServers",
                    ]
                break

        for committe_key in committe_keys:
            for connected_race_persona in race_committee_config[committe_key]:
                network_edges.append((race_persona, connected_race_persona))

    # network_edges = list(set(network_edges))
    race_graph.add_edges_from(set(network_edges))

    return race_graph


###
# Vizualize Graph Functions
###


def visualize_network_graph(race_graph, show_results=True, save_results=None):
    """
    Purpose:
        Build Network Graph from Configs
    Args:
        race_graph (Graph Obj): RACE Network Graph Object
    Returns:
        N/A
    """
    logging.info("Visualize Graph")

    # Get Node Positions
    node_positions = nx.spring_layout(race_graph)
    node_x_position_values = [
        node_position[0] for node_id, node_position in node_positions.items()
    ]
    node_x_position_stdev = statistics.stdev(node_x_position_values)
    node_y_position_values = [
        node_position[1] for node_id, node_position in node_positions.items()
    ]
    node_y_position_stdev = statistics.stdev(node_y_position_values)

    # Get Edge Positions
    edge_positions = node_positions

    # Get Label Positions
    label_positions = {}
    for node_id, node_position in node_positions.items():
        label_positions[node_id] = copy.deepcopy(node_position)
        # if label_positions[node_id][0] > 0:
        #     label_positions[node_id][0] -= node_x_position_stdev / 2.0
        # else:
        #     label_positions[node_id][0] += node_x_position_stdev / 2.0
        label_positions[node_id][0] += node_x_position_stdev / 4.0

    # Get Nodes of each type
    client_nodes = [
        node[0] for node in race_graph.nodes.data("nodeType") if node[1] == "client"
    ]
    server_nodes = [
        node[0] for node in race_graph.nodes.data("nodeType") if node[1] == "server"
    ]

    # Draw Graph
    nx.draw_networkx_nodes(
        race_graph,
        node_positions,
        node_size=400,
        nodelist=client_nodes,
        node_color="blue",
    )
    nx.draw_networkx_nodes(
        race_graph,
        node_positions,
        node_size=400,
        nodelist=server_nodes,
        node_color="green",
    )
    nx.draw_networkx_edges(
        race_graph, edge_positions, edge_color="black", arrows=True, alpha=1, width=2
    )
    nx.draw_networkx_labels(race_graph, label_positions, font_size=12, font_color="k")

    if save_results:
        logging.info(f"Saving Network Graph to: {save_results}")
        plt.savefig(save_results)
    if show_results:
        logging.info(f"Showing Network Graph")
        plt.show()
