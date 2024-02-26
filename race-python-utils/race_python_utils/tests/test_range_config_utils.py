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
        Test File for range_config_utils.py
"""

# Python Library Imports
import copy
import os
import sys
import pytest
import requests
from typing import Any, Dict
from unittest import mock

# Local Library Imports
from race_python_utils import range_config_utils


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


#######
# get_client_details_from_range_config
#######


def test_get_client_details_from_range_config(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    clients = range_config_utils.get_client_details_from_range_config(
        example_1ax1cx4s_range_config
    )

    assert sorted(list(clients.keys())) == [
        "race-client-00001",
        "race-client-00002",
    ]

    assert clients["race-client-00001"] == {
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
    }


#######
# get_server_details_from_range_config
#######


def test_get_server_details_from_range_config(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    servers = range_config_utils.get_server_details_from_range_config(
        example_1ax1cx4s_range_config
    )

    assert sorted(list(servers.keys())) == [
        "race-server-00001",
        "race-server-00002",
        "race-server-00003",
        "race-server-00004",
    ]

    assert servers["race-server-00001"] == {
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
    }


#######
# get_service_from_range_config
#######


def test_get_service_from_range_config_found(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    service = range_config_utils.get_service_from_range_config(
        example_1ax1cx4s_range_config, "twosix-whiteboard"
    )

    assert service == {
        "name": "twosix-whiteboard",
        "url": "twosix-whiteboard:5000",
        "protocol": "http",
        "hostname": "twosix-whiteboard",
        "port": 5000,
        "authenticated_service": False,
    }


def test_get_service_from_range_config_not_found(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    service = range_config_utils.get_service_from_range_config(
        example_1ax1cx4s_range_config, "not_present"
    )

    assert service == {}


#######
# validate_range_config
#######


def test_validate_range_config_valid(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    range_config_utils.validate_range_config(example_1ax1cx4s_range_config)


def test_validate_range_config_invalid_missing_key(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    example_range_config = copy.deepcopy(example_1ax1cx4s_range_config)
    with pytest.raises(Exception, match=r"Config missing range"):
        # Break Range Config
        del example_range_config["range"]["RACE_nodes"]
        range_config_utils.validate_range_config(example_range_config)

    example_range_config = copy.deepcopy(example_1ax1cx4s_range_config)
    with pytest.raises(Exception, match=r"Config missing range"):
        # Break Range Config
        del example_range_config["range"]["enclaves"]
        range_config_utils.validate_range_config(example_range_config)

    example_range_config = copy.deepcopy(example_1ax1cx4s_range_config)
    with pytest.raises(Exception, match=r"Config missing range"):
        # Break Range Config
        del example_range_config["range"]["services"]
        range_config_utils.validate_range_config(example_range_config)


def test_validate_range_config_invalid_no_clients(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    example_range_config = copy.deepcopy(example_1ax1cx4s_range_config)
    with pytest.raises(Exception, match=r"No clients"):
        # Break Range Config
        example_range_config["range"]["RACE_nodes"] = []
        example_range_config["range"]["RACE_nodes"].append(
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
            }
        )
        range_config_utils.validate_range_config(example_range_config)


def test_validate_range_config_invalid_no_servers(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    example_range_config = copy.deepcopy(example_1ax1cx4s_range_config)
    with pytest.raises(Exception, match=r"No servers"):
        # Break Range Config
        example_range_config["range"]["RACE_nodes"] = []
        example_range_config["range"]["RACE_nodes"].append(
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
            }
        )
        range_config_utils.validate_range_config(example_range_config)


def test_validate_range_config_invalid_no_enclaves(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    example_range_config = copy.deepcopy(example_1ax1cx4s_range_config)
    with pytest.raises(Exception, match=r"No enclaves"):
        # Break Range Config
        example_range_config["range"]["enclaves"] = []
        range_config_utils.validate_range_config(example_range_config)


#######
# get_full_internode_connectivity
#######


def test_get_full_internode_connectivity(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    full_internode_connectivity = range_config_utils.get_full_internode_connectivity(
        example_1ax1cx4s_range_config
    )

    assert full_internode_connectivity == {
        "race-client-00001": {
            "race-client-00002": {},
            "race-server-00001": {
                "hostname": "race-server-00001",
                "ports": {"80": "80"},
            },
            "race-server-00002": {
                "hostname": "race-server-00002",
                "ports": {"80": "80"},
            },
            "race-server-00003": {
                "hostname": "race-server-00003",
                "ports": {"8080": "80"},
            },
            "race-server-00004": {
                "hostname": "race-server-00004",
                "ports": {"8081": "80"},
            },
        },
        "race-client-00002": {
            "race-client-00001": {},
            "race-server-00001": {
                "hostname": "race-server-00001",
                "ports": {"80": "80"},
            },
            "race-server-00002": {
                "hostname": "race-server-00002",
                "ports": {"80": "80"},
            },
            "race-server-00003": {
                "hostname": "race-server-00003",
                "ports": {"8080": "80"},
            },
            "race-server-00004": {
                "hostname": "race-server-00004",
                "ports": {"8081": "80"},
            },
        },
        "race-server-00001": {
            "race-client-00001": {},
            "race-client-00002": {},
            "race-server-00002": {"hostname": "race-server-00002", "ports": {"*": "*"}},
            "race-server-00003": {
                "hostname": "race-server-00003",
                "ports": {"8080": "80"},
            },
            "race-server-00004": {
                "hostname": "race-server-00004",
                "ports": {"8081": "80"},
            },
        },
        "race-server-00002": {
            "race-client-00001": {},
            "race-client-00002": {},
            "race-server-00001": {"hostname": "race-server-00001", "ports": {"*": "*"}},
            "race-server-00003": {
                "hostname": "race-server-00003",
                "ports": {"8080": "80"},
            },
            "race-server-00004": {
                "hostname": "race-server-00004",
                "ports": {"8081": "80"},
            },
        },
        "race-server-00003": {
            "race-client-00001": {},
            "race-client-00002": {},
            "race-server-00001": {
                "hostname": "race-server-00001",
                "ports": {"80": "80"},
            },
            "race-server-00002": {
                "hostname": "race-server-00002",
                "ports": {"80": "80"},
            },
            "race-server-00004": {"hostname": "race-server-00004", "ports": {"*": "*"}},
        },
        "race-server-00004": {
            "race-client-00001": {},
            "race-client-00002": {},
            "race-server-00001": {
                "hostname": "race-server-00001",
                "ports": {"80": "80"},
            },
            "race-server-00002": {
                "hostname": "race-server-00002",
                "ports": {"80": "80"},
            },
            "race-server-00003": {"hostname": "race-server-00003", "ports": {"*": "*"}},
        },
    }


#######
# get_race_node_by_name
#######


def test_get_race_node_by_name_found(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    race_node = range_config_utils.get_race_node_by_name(
        example_1ax1cx4s_range_config, "race-client-00001"
    )

    assert race_node == {
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
    }


def test_get_race_node_by_name_not_found(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    with pytest.raises(Exception, match=r"not found in range_config"):
        race_node = range_config_utils.get_race_node_by_name(
            example_1ax1cx4s_range_config, "race-client-00100"
        )


#######
# get_enclave_by_name
#######


def test_get_enclave_by_name_found(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    enclave = range_config_utils.get_enclave_by_name(
        example_1ax1cx4s_range_config, "Enclave1"
    )

    assert enclave == {
        "name": "Enclave1",
        "ip": "1.2.3.4",
        "port_mapping": {
            "80": {"hosts": ["race-server-00001", "race-server-00002"], "port": "80"}
        },
    }


def test_get_enclave_by_name_not_found(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    with pytest.raises(Exception, match=r"not found in range_config"):
        enclave = range_config_utils.get_enclave_by_name(
            example_1ax1cx4s_range_config, "Enclave100"
        )


#######
# get_nodes_per_enclave
#######


def test_get_nodes_per_enclave(example_1ax1cx4s_range_config: Dict[str, Any]) -> int:

    nodes_per_enclave = range_config_utils.get_nodes_per_enclave(
        example_1ax1cx4s_range_config
    )

    assert nodes_per_enclave == {
        "Enclave1": ["race-server-00001", "race-server-00002"],
        "Enclave2": ["race-server-00003", "race-server-00004"],
        "Enclave3": ["race-client-00001"],
        "Enclave4": ["race-client-00002"],
    }


#######
# get_internode_connectivity
#######


def test_get_internode_connectivity(
    example_1ax1cx4s_range_config: Dict[str, Any]
) -> int:

    s1_s2_internode_connectivity = range_config_utils.get_internode_connectivity(
        example_1ax1cx4s_range_config, "race-server-00001", "race-server-00002"
    )
    s3_s2_internode_connectivity = range_config_utils.get_internode_connectivity(
        example_1ax1cx4s_range_config, "race-server-00003", "race-server-00002"
    )
    c1_c2_internode_connectivity = range_config_utils.get_internode_connectivity(
        example_1ax1cx4s_range_config, "race-client-00001", "race-client-00002"
    )
    c1_s1_internode_connectivity = range_config_utils.get_internode_connectivity(
        example_1ax1cx4s_range_config, "race-client-00001", "race-server-00001"
    )

    assert s1_s2_internode_connectivity == {
        "hostname": "race-server-00002",
        "ports": {"*": "*"},
    }
    assert s3_s2_internode_connectivity == {
        "hostname": "race-server-00002",
        "ports": {"80": "80"},
    }
    assert c1_c2_internode_connectivity == {}
    assert c1_s1_internode_connectivity == {
        "hostname": "race-server-00001",
        "ports": {"80": "80"},
    }


#######
# guess_port_by_protocol
#######


def test_guess_port_by_protocol(example_1ax1cx4s_range_config: Dict[str, Any]) -> int:

    assert range_config_utils.guess_port_by_protocol("http") == 80
    assert range_config_utils.guess_port_by_protocol("https") == 443
    assert range_config_utils.guess_port_by_protocol("ssh") == 22
    assert range_config_utils.guess_port_by_protocol("na") == None
