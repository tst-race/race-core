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
        Test File for network_manager_request_utils.py
"""

# Python Library Imports
import copy
import os
import sys
import pytest
import requests
from typing import Any, Dict, List
from unittest import mock

# Local Library Imports
from race_python_utils import network_manager_request_utils


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


@pytest.fixture
def example_1ax1cx4s_network_manager_request() -> Dict[str, Any]:
    """
    Purpose:
        Example network_manager_request with 1 android client, 1 linux client,
        and 4 linux servers
    Args:
        N/A
    Return:
        example_1ax1cx4s_network_manager_request: Dict of network manager request
    """

    return {
        "links": [
            {
                "sender": "race-server-00001",
                "recipients": [
                    "race-server-00002",
                    "race-server-00003",
                    "race-server-00004",
                ],
                "details": {},
                "groupId": None,
            },
            {
                "sender": "race-server-00002",
                "recipients": [
                    "race-server-00001",
                    "race-server-00003",
                    "race-server-00004",
                ],
                "details": {},
                "groupId": None,
            },
            {
                "sender": "race-server-00003",
                "recipients": [
                    "race-server-00001",
                    "race-server-00002",
                    "race-server-00004",
                ],
                "details": {},
                "groupId": None,
            },
            {
                "sender": "race-server-00004",
                "recipients": [
                    "race-server-00001",
                    "race-server-00002",
                    "race-server-00003",
                ],
                "details": {},
                "groupId": None,
            },
        ]
    }


@pytest.fixture
def example_channel_properties_list() -> List[Dict[str, Any]]:
    """
    Purpose:
        Example channel properties list with twoSixIndirectCpp and twoSixDirectCpp
    Args:
        N/A
    Return:
        example_channel_properties_list: list of channel properties
    """

    return [
        {
            "channelGid": "twoSixDirectCpp",
            "description": "Implementation of the Two Six Labs Direct communications utilizing Sockets",
            "reliable": False,
        },
        {
            "channelGid": "twoSixIndirectCpp",
            "description": "Implementation of the Two Six Labs Indirect communications utilizing the Two Six Whiteboard",
            "reliable": False,
        },
    ]


#########
# generate_network_manager_request_from_range_config
#########


def test_generate_network_manager_request_from_range_config_multicast_direct(
    example_1ax1cx4s_range_config: Dict[str, Any],
    example_channel_properties_list: List[Dict[str, Any]],
) -> int:

    network_manager_request_result = network_manager_request_utils.generate_network_manager_request_from_range_config(
        example_1ax1cx4s_range_config,
        example_channel_properties_list,
        "multicast",
        "direct",
    )

    s2s_multicast = (
        network_manager_request_utils.generate_s2s_multicast_network_manager_request_from_range_config(
            example_1ax1cx4s_range_config, example_channel_properties_list
        )
    )

    assert network_manager_request_result == s2s_multicast


def test_generate_network_manager_request_from_range_config_unicast_direct(
    example_1ax1cx4s_range_config: Dict[str, Any],
    example_channel_properties_list: List[Dict[str, Any]],
) -> int:

    network_manager_request_result = network_manager_request_utils.generate_network_manager_request_from_range_config(
        example_1ax1cx4s_range_config,
        example_channel_properties_list,
        "unicast",
        "direct",
    )

    s2s_unicast = network_manager_request_utils.generate_s2s_unicast_network_manager_request_from_range_config(
        example_1ax1cx4s_range_config, example_channel_properties_list
    )

    assert network_manager_request_result == s2s_unicast


def test_generate_network_manager_request_from_range_config_multicast_indirect(
    example_1ax1cx4s_range_config: Dict[str, Any],
    example_channel_properties_list: List[Dict[str, Any]],
) -> int:

    network_manager_request_result = network_manager_request_utils.generate_network_manager_request_from_range_config(
        example_1ax1cx4s_range_config,
        example_channel_properties_list,
        "multicast",
        "indirect",
    )

    s2s_multicast = (
        network_manager_request_utils.generate_s2s_multicast_network_manager_request_from_range_config(
            example_1ax1cx4s_range_config, example_channel_properties_list
        )
    )
    c2s_multicast = (
        network_manager_request_utils.generate_c2s_multicast_network_manager_request_from_range_config(
            example_1ax1cx4s_range_config, example_channel_properties_list
        )
    )

    expected_result = s2s_multicast
    if not expected_result:
        expected_result = c2s_multicast
    elif c2s_multicast:
        expected_result["links"].extend(c2s_multicast["links"])

    assert network_manager_request_result == expected_result


def test_generate_network_manager_request_from_range_config_unicast_indirect(
    example_1ax1cx4s_range_config: Dict[str, Any],
    example_channel_properties_list: List[Dict[str, Any]],
) -> int:

    network_manager_request_result = network_manager_request_utils.generate_network_manager_request_from_range_config(
        example_1ax1cx4s_range_config,
        example_channel_properties_list,
        "unicast",
        "indirect",
    )

    s2s_unicast = network_manager_request_utils.generate_s2s_unicast_network_manager_request_from_range_config(
        example_1ax1cx4s_range_config, example_channel_properties_list
    )
    c2s_unicast = network_manager_request_utils.generate_c2s_unicast_network_manager_request_from_range_config(
        example_1ax1cx4s_range_config, example_channel_properties_list
    )

    expected_result = s2s_unicast
    if not expected_result:
        expected_result = c2s_unicast
    elif c2s_unicast:
        expected_result["links"].extend(c2s_unicast["links"])

    assert network_manager_request_result == expected_result


#########
# Tests
#########


def test_generate_s2s_unicast_network_manager_request_from_range_config(
    example_1ax1cx4s_range_config: Dict[str, Any],
    example_channel_properties_list: List[Dict[str, Any]],
) -> int:

    network_manager_request_s2s = (
        network_manager_request_utils.generate_s2s_unicast_network_manager_request_from_range_config(
            example_1ax1cx4s_range_config, example_channel_properties_list
        )
    )

    # Verify some expected links
    expected_links = [
        {
            "sender": "race-server-00001",
            "recipients": ["race-server-00002"],
            "details": {},
            "groupId": None,
            "channels": ["twoSixDirectCpp", "twoSixIndirectCpp"],
        },
        {
            "sender": "race-server-00002",
            "recipients": ["race-server-00001"],
            "details": {},
            "groupId": None,
            "channels": ["twoSixDirectCpp", "twoSixIndirectCpp"],
        },
    ]
    for expected_link in expected_links:
        assert expected_link in network_manager_request_s2s["links"]

    # Verify No Clients
    for requested_link in network_manager_request_s2s["links"]:
        assert "client" not in requested_link["sender"]
        for recipient in requested_link["recipients"]:
            assert "client" not in recipient

    # Verify Unicast Links
    for requested_link in network_manager_request_s2s["links"]:
        assert len(requested_link["recipients"]) == 1


def test_generate_s2s_multicast_network_manager_request_from_range_config(
    example_1ax1cx4s_range_config: Dict[str, Any],
    example_channel_properties_list: List[Dict[str, Any]],
) -> int:

    network_manager_request_s2s = (
        network_manager_request_utils.generate_s2s_multicast_network_manager_request_from_range_config(
            example_1ax1cx4s_range_config, example_channel_properties_list
        )
    )

    # Verify some expected links
    expected_links = [
        {
            "sender": "race-server-00001",
            "recipients": [
                "race-server-00002",
                "race-server-00003",
                "race-server-00004",
            ],
            "details": {},
            "groupId": None,
            "channels": ["twoSixDirectCpp", "twoSixIndirectCpp"],
        },
        {
            "sender": "race-server-00002",
            "recipients": [
                "race-server-00001",
                "race-server-00003",
                "race-server-00004",
            ],
            "details": {},
            "groupId": None,
            "channels": ["twoSixDirectCpp", "twoSixIndirectCpp"],
        },
    ]
    for expected_link in expected_links:
        assert expected_link in network_manager_request_s2s["links"]

    # Verify No Clients
    for requested_link in network_manager_request_s2s["links"]:
        assert "client" not in requested_link["sender"]
        for recipient in requested_link["recipients"]:
            assert "client" not in recipient

    # Verify Unicast Links
    for requested_link in network_manager_request_s2s["links"]:
        assert len(requested_link["recipients"]) > 1


def test_generate_c2s_unicast_network_manager_request_from_range_config(
    example_1ax1cx4s_range_config: Dict[str, Any],
    example_channel_properties_list: List[Dict[str, Any]],
) -> int:

    network_manager_request_c2s = (
        network_manager_request_utils.generate_c2s_unicast_network_manager_request_from_range_config(
            example_1ax1cx4s_range_config, example_channel_properties_list
        )
    )

    # Verify some expected links
    expected_links = [
        {
            "sender": "race-client-00001",
            "recipients": [
                "race-server-00001",
            ],
            "details": {},
            "groupId": None,
            "channels": ["twoSixDirectCpp", "twoSixIndirectCpp"],
        },
        {
            "sender": "race-server-00001",
            "recipients": [
                "race-client-00001",
            ],
            "details": {},
            "groupId": None,
            "channels": ["twoSixDirectCpp", "twoSixIndirectCpp"],
        },
    ]
    for expected_link in expected_links:
        assert expected_link in network_manager_request_c2s["links"]

    # Verify Unicast Links
    for requested_link in network_manager_request_c2s["links"]:
        assert len(requested_link["recipients"]) == 1


def test_generate_c2s_multicast_network_manager_request_from_range_config(
    example_1ax1cx4s_range_config: Dict[str, Any],
    example_channel_properties_list: List[Dict[str, Any]],
) -> int:

    network_manager_request_c2s = (
        network_manager_request_utils.generate_c2s_multicast_network_manager_request_from_range_config(
            example_1ax1cx4s_range_config, example_channel_properties_list
        )
    )

    # Verify some expected links
    expected_links = [
        {
            "sender": "race-client-00001",
            "recipients": [
                "race-server-00001",
                "race-server-00002",
                "race-server-00003",
                "race-server-00004",
            ],
            "details": {},
            "groupId": None,
            "channels": ["twoSixDirectCpp", "twoSixIndirectCpp"],
        },
        {
            "sender": "race-server-00001",
            "recipients": [
                "race-client-00001",
                "race-client-00002",
            ],
            "details": {},
            "groupId": None,
            "channels": ["twoSixDirectCpp", "twoSixIndirectCpp"],
        },
    ]
    for expected_link in expected_links:
        assert expected_link in network_manager_request_c2s["links"]

    # Verify Unicast Links
    for requested_link in network_manager_request_c2s["links"]:
        assert len(requested_link["recipients"]) > 1


###
# test_validate_network_manager_request
###


def test_validate_network_manager_request_valid(
    example_1ax1cx4s_network_manager_request: Dict[str, Any],
    example_1ax1cx4s_range_config: Dict[str, Any],
) -> int:

    network_manager_request_utils.validate_network_manager_request(
        example_1ax1cx4s_network_manager_request, example_1ax1cx4s_range_config
    )


def test_validate_network_manager_request_invalid_missing_links(
    example_1ax1cx4s_network_manager_request: Dict[str, Any],
    example_1ax1cx4s_range_config: Dict[str, Any],
) -> int:

    example_network_manager_request = copy.deepcopy(example_1ax1cx4s_network_manager_request)
    with pytest.raises(Exception, match=r"missing links key"):
        del example_network_manager_request["links"]
        network_manager_request_utils.validate_network_manager_request(
            example_network_manager_request, example_1ax1cx4s_range_config
        )


def test_validate_network_manager_request_invalid_missing_sender(
    example_1ax1cx4s_network_manager_request: Dict[str, Any],
    example_1ax1cx4s_range_config: Dict[str, Any],
) -> int:

    example_network_manager_request = copy.deepcopy(example_1ax1cx4s_network_manager_request)
    with pytest.raises(Exception, match=r"has no sender"):
        del example_network_manager_request["links"][0]["sender"]
        network_manager_request_utils.validate_network_manager_request(
            example_network_manager_request, example_1ax1cx4s_range_config
        )


def test_validate_network_manager_request_invalid_missing_recipients(
    example_1ax1cx4s_network_manager_request: Dict[str, Any],
    example_1ax1cx4s_range_config: Dict[str, Any],
) -> int:

    example_network_manager_request = copy.deepcopy(example_1ax1cx4s_network_manager_request)
    with pytest.raises(Exception, match=r"has no recipients"):
        del example_network_manager_request["links"][0]["recipients"]
        network_manager_request_utils.validate_network_manager_request(
            example_network_manager_request, example_1ax1cx4s_range_config
        )


def test_validate_network_manager_request_invalid_incorrect_sender(
    example_1ax1cx4s_network_manager_request: Dict[str, Any],
    example_1ax1cx4s_range_config: Dict[str, Any],
) -> int:

    example_network_manager_request = copy.deepcopy(example_1ax1cx4s_network_manager_request)
    with pytest.raises(Exception, match=r"Invalid.*sender"):
        example_network_manager_request["links"][0]["sender"] = "race-client-00100"
        network_manager_request_utils.validate_network_manager_request(
            example_network_manager_request, example_1ax1cx4s_range_config
        )


def test_validate_network_manager_request_invalid_incorrect_recipients(
    example_1ax1cx4s_network_manager_request: Dict[str, Any],
    example_1ax1cx4s_range_config: Dict[str, Any],
) -> int:

    example_network_manager_request = copy.deepcopy(example_1ax1cx4s_network_manager_request)
    with pytest.raises(Exception, match=r"Invalid.*recipients"):
        example_network_manager_request["links"][0]["recipients"].append("race-client-00100")
        network_manager_request_utils.validate_network_manager_request(
            example_network_manager_request, example_1ax1cx4s_range_config
        )
