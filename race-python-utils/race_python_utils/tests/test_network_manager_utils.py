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
        Test File for network_manager_utils.py
"""

# Python Library Imports
import networkx as nx
import os
import sys
import pytest
from typing import Any, Dict
from unittest import mock

# Local Library Imports
from race_python_utils import network_manager_utils
from race_python_utils.network_manager import Client, Committee, Server


###
# Fixtures / Mocks
###


@pytest.fixture
def example_1ax1cx4s_range_config() -> Dict[str, Any]:
    """
    Purpose:
        Example range config with 1 android client, 1 linux client,
        and 4 linux servers
    Args:
        N/A
    Return:
        example_1ax1cx4s_range_config: Dict of range config
    """

    return {
        "range": {
            "name": "Example1ax1cx4s",
            "bastion": {},
            "RACE_nodes": [
                {
                    "name": "race-client-00001",
                    "type": "RACE android client",
                    "enclave": "Enclave3",
                    "nat": True,
                    "identities": [
                        {
                            "email": "race-client-00001@race-client-00001.race-client-00001",
                            "password": "password1234",
                            "service": "imagegram",
                            "username": "race-client-00001",
                        }
                    ],
                },
                {
                    "name": "race-client-00003",
                    "type": "RACE android client",
                    "enclave": "Enclave3",
                    "nat": True,
                    "genesis": False,
                    "identities": [],
                },
                {
                    "name": "race-client-00002",
                    "type": "RACE linux client",
                    "enclave": "Enclave4",
                    "nat": True,
                    "identities": [
                        {
                            "email": "race-client-00002@race-client-00002.race-client-00002",
                            "password": "password1234",
                            "service": "imagegram",
                            "username": "race-client-00002",
                        }
                    ],
                },
                {
                    "name": "race-client-00004",
                    "type": "RACE linux client",
                    "enclave": "Enclave4",
                    "nat": True,
                    "genesis": False,
                    "identities": [],
                },
                {
                    "name": "race-server-00001",
                    "type": "RACE linux server",
                    "enclave": "Enclave1",
                    "nat": False,
                    "identities": [
                        {
                            "email": "race-server-00001@race-server-00001.race-server-00001",
                            "password": "password1234",
                            "service": "imagegram",
                            "username": "race-server-00001",
                        }
                    ],
                },
                {
                    "name": "race-server-00002",
                    "type": "RACE linux server",
                    "enclave": "Enclave1",
                    "nat": False,
                    "identities": [
                        {
                            "email": "race-server-00002@race-server-00002.race-server-00002",
                            "password": "password1234",
                            "service": "imagegram",
                            "username": "race-server-00002",
                        }
                    ],
                },
                {
                    "name": "race-server-00003",
                    "type": "RACE linux server",
                    "enclave": "Enclave2",
                    "nat": True,
                    "identities": [
                        {
                            "email": "race-server-00003@race-server-00003.race-server-00003",
                            "password": "password1234",
                            "service": "imagegram",
                            "username": "race-server-00003",
                        }
                    ],
                },
                {
                    "name": "race-server-00004",
                    "type": "RACE linux server",
                    "enclave": "Enclave2",
                    "nat": True,
                    "identities": [
                        {
                            "email": "race-server-00004@race-server-00004.race-server-00004",
                            "password": "password1234",
                            "service": "imagegram",
                            "username": "race-server-00004",
                        }
                    ],
                },
                {
                    "name": "race-server-00005",
                    "type": "RACE linux server",
                    "enclave": "Enclave2",
                    "nat": True,
                    "genesis": False,
                    "identities": [],
                },
            ],
            "enclaves": [
                {
                    "name": "Enclave1",
                    "ip": "1.2.3.4",
                    "port_mapping": {
                        "80": {
                            "hosts": ["race-server-00001", "race-server-00002"],
                            "port": "80",
                        }
                    },
                },
                {
                    "name": "Enclave2",
                    "ip": "2.3.4.5",
                    "port_mapping": {
                        "8080": {"hosts": ["race-server-00003"], "port": "80"},
                        "8081": {"hosts": ["race-server-00004"], "port": "80"},
                    },
                },
                {"name": "Enclave3", "ip": "3.4.5.6", "port_mapping": {}},
                {"name": "Enclave4", "ip": "4.5.6.7", "port_mapping": {}},
            ],
            "services": [
                {
                    "access": [{"protocol": "http", "url": "twosix-whiteboard:5000"}],
                    "auth-req-post": "anonymous",
                    "auth-req-reply": "anonymous",
                    "auth-req-view": "anonymous",
                    "auth-req_delete": "anonymous",
                    "name": "twosix-whiteboard",
                    "type": "twosix-whiteboard",
                },
                {
                    "access": [{"protocol": "https", "url": "race.example2"}],
                    "auth-req-post": "authenticate",
                    "auth-req-reply": "anonymous",
                    "auth-req-view": "anonymous",
                    "auth-req_delete": "authenticate",
                    "name": "imagegram",
                    "type": "social-pixelfed",
                },
            ],
        }
    }


###
# Tests
###


def test_undirectionalize_cycle():
    cycle = nx.cycle_graph(5)
    digraph = nx.DiGraph()
    digraph.add_edges_from(cycle.edges())
    graph = network_manager_utils.undirectionalize(digraph)
    assert len(graph.edges()) == 0
    digraph.add_edge(1, 0)
    graph = network_manager_utils.undirectionalize(digraph)
    assert len(graph.edges()) == 1


def test_undirectionalize_graph():
    graph = nx.cycle_graph(5)
    graph2 = network_manager_utils.undirectionalize(graph)
    graph.add_node(5)
    assert graph != graph2
    assert isinstance(graph2, nx.Graph)


def test_kcomp_split():
    graph = nx.cycle_graph(5)
    nodeset = network_manager_utils.kcomp_split(graph, [0, 1, 2, 3, 4], 3)
    assert len(nodeset) == 3
    assert nx.is_connected(graph.subgraph(nodeset))


def test_kcomp_split_wrong():
    graph = nx.cycle_graph(3)
    nodeset = network_manager_utils.kcomp_split(graph, [0, 1, 2], 4)
    assert nodeset is None


def test_assign_client():
    servers = [Server(f"{idx}") for idx in range(3)]
    clients = [Client(f"{idx}") for idx in range(3)]
    committees = {f"{idx}": Committee(f"{idx}") for idx in range(3)}
    for idx in range(3):
        committees[f"{idx}"].servers.append(servers[idx])
        servers[idx].committee = committees[f"{idx}"]

    for idx in range(3):
        network_manager_utils.assign_client(committees, f"{idx}", clients[idx], False)

    for idx in range(3):
        assert committees[f"{idx}"].clients == [clients[idx]]
        assert clients[idx].exit_committee == [servers[idx]]
        assert clients[idx].entrance_committee == [servers[idx]]
        assert servers[idx].exit_clients == [clients[idx]]
        assert servers[idx].committee == committees[f"{idx}"]


def test_generate_aes_keys_from_range_config():
    config = {
        "range": {
            "RACE_nodes": [
                {
                    "name": "race-client-00001",
                    "type": "RACE android client",
                },
                {
                    "name": "race-server-00001",
                    "type": "RACE linux server",
                },
            ]
        }
    }
    keys = network_manager_utils.generate_aes_keys_from_range_config(config)
    for node in config["range"]["RACE_nodes"]:
        assert len(keys[node["name"]]) == 32


#############
# generate_personas_config_from_range_config
#############


def test_generate_personas_config_from_range_config(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    personas_config = network_manager_utils.generate_personas_config_from_range_config(
        example_1ax1cx4s_range_config
    )

    assert len(personas_config) == 6
    assert {
        "displayName": "RACE Client 00001",
        "raceUuid": "race-client-00001",
        "publicKey": "00001",
        "personaType": "client",
        "aesKeyFile": "./race-client-00001.aes",
    } in personas_config
    assert {
        "displayName": "RACE Client 00002",
        "raceUuid": "race-client-00002",
        "publicKey": "00002",
        "personaType": "client",
        "aesKeyFile": "./race-client-00002.aes",
    } in personas_config
    assert {
        "displayName": "RACE Server 00001",
        "raceUuid": "race-server-00001",
        "publicKey": "00001",
        "personaType": "server",
        "aesKeyFile": "./race-server-00001.aes",
    } in personas_config
    assert {
        "displayName": "RACE Server 00002",
        "raceUuid": "race-server-00002",
        "publicKey": "00002",
        "personaType": "server",
        "aesKeyFile": "./race-server-00002.aes",
    } in personas_config
    assert {
        "displayName": "RACE Server 00003",
        "raceUuid": "race-server-00003",
        "publicKey": "00003",
        "personaType": "server",
        "aesKeyFile": "./race-server-00003.aes",
    } in personas_config
    assert {
        "displayName": "RACE Server 00004",
        "raceUuid": "race-server-00004",
        "publicKey": "00004",
        "personaType": "server",
        "aesKeyFile": "./race-server-00004.aes",
    } in personas_config
