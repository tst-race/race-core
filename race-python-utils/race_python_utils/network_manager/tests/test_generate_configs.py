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
                Test File for generate_configs.py
"""

# Python Library Imports
import os
import sys
import pytest
from typing import List
from unittest import mock

# Local Library Imports
import race_python_utils.network_manager.generate_configs as generate_configs
import race_python_utils.network_manager_utils as network_manager_utils


def default_channel():
    return {
        "bootstrap": False,
        "channelGid": "test-channel-id",
        "connectionType": "CT_INDIRECT",
        "creatorExpected": {
            "receive": {"bandwidth_bps": 277200, "latency_ms": 3190, "loss": 0.1},
            "send": {"bandwidth_bps": 277200, "latency_ms": 3190, "loss": 0.1},
        },
        "creatorsPerLoader": -1,
        "description": "Test Channel description",
        "duration_s": -1,
        "isFlushable": False,
        "linkDirection": "LD_BIDI",
        "loaderExpected": {
            "receive": {"bandwidth_bps": 277200, "latency_ms": 3190, "loss": 0.1},
            "send": {"bandwidth_bps": 277200, "latency_ms": 3190, "loss": 0.1},
        },
        "loadersPerCreator": -1,
        "maxLinks": 1000,
        "mtu": -1,
        "multiAddressable": False,
        "period_s": -1,
        "reliable": False,
        "roles": [
            {
                "behavioralTags": [],
                "linkSide": "LS_BOTH",
                "mechanicalTags": [],
                "roleName": "default",
            }
        ],
        "sendType": "ST_STORED_ASYNC",
        "supported_hints": [],
        "transmissionType": "TT_MULTICAST",
    }


DEFAULT_ROLE_INDEX = 0


def loader_creator_channel():
    channel = default_channel()
    channel["roles"] = [
        {
            "behavioralTags": [],
            "linkSide": "LS_LOADER",
            "mechanicalTags": [],
            "roleName": "loader",
        },
        {
            "behavioralTags": [],
            "linkSide": "LS_CREATOR",
            "mechanicalTags": [],
            "roleName": "creator",
        },
    ]
    return channel


LOADER_ROLE_INDEX = 0
CREATOR_ROLE_INDEX = 1


def default_role():
    return {
        "behavioralTags": [],
        "linkSide": "LS_BOTH",
        "mechanicalTags": [],
        "roleName": "default",
    }


def default_state():
    class Object(object):
        pass

    cli_args = Object()
    cli_args.complete_connectivity = False
    cli_args.overwrite = False
    cli_args.disable_dynamic_links = False
    cli_args.genesis_c2s_channels = ""
    cli_args.genesis_s2s_channels = ""
    cli_args.dynamic_c2s_channels = ""
    cli_args.dynamic_s2s_channels = ""

    range_config = {
        "range": {
            "RACE_nodes": [
                {
                    "bridge": False,
                    "enclave": "global",
                    "environment": "any",
                    "genesis": True,
                    "gpu": False,
                    "identities": [],
                    "name": "race-client-00001",
                    "nat": False,
                    "platform": "linux",
                    "type": "client",
                },
                {
                    "bridge": False,
                    "enclave": "global",
                    "environment": "any",
                    "genesis": True,
                    "gpu": False,
                    "identities": [],
                    "name": "race-client-00002",
                    "nat": False,
                    "platform": "linux",
                    "type": "client",
                },
                {
                    "bridge": False,
                    "enclave": "global",
                    "environment": "any",
                    "genesis": True,
                    "gpu": False,
                    "identities": [],
                    "name": "race-client-00003",
                    "nat": False,
                    "platform": "linux",
                    "type": "client",
                },
                {
                    "bridge": False,
                    "enclave": "global",
                    "environment": "any",
                    "genesis": True,
                    "gpu": False,
                    "identities": [],
                    "name": "race-server-00001",
                    "nat": False,
                    "platform": "linux",
                    "type": "server",
                },
                {
                    "bridge": False,
                    "enclave": "global",
                    "environment": "any",
                    "genesis": True,
                    "gpu": False,
                    "identities": [],
                    "name": "race-server-00002",
                    "nat": False,
                    "platform": "linux",
                    "type": "server",
                },
                {
                    "bridge": False,
                    "enclave": "global",
                    "environment": "any",
                    "genesis": True,
                    "gpu": False,
                    "identities": [],
                    "name": "race-server-00003",
                    "nat": False,
                    "platform": "linux",
                    "type": "server",
                },
            ],
            "bastion": {},
            "enclaves": [{"ip": "localhost", "name": "global", "port_mapping": {}}],
            "name": "config-gen-test",
            "services": [],
        }
    }

    personas = [
        {
            "aesKeyFile": "./race-client-00001.aes",
            "displayName": "RACE Client 00001",
            "personaType": "client",
            "publicKey": "00001",
            "raceUuid": "race-client-00001",
        },
        {
            "aesKeyFile": "./race-client-00002.aes",
            "displayName": "RACE Client 00002",
            "personaType": "client",
            "publicKey": "00002",
            "raceUuid": "race-client-00002",
        },
        {
            "aesKeyFile": "./race-client-00003.aes",
            "displayName": "RACE Client 00003",
            "personaType": "client",
            "publicKey": "00003",
            "raceUuid": "race-client-00003",
        },
        {
            "aesKeyFile": "./race-server-00001.aes",
            "displayName": "RACE Server 00001",
            "personaType": "server",
            "publicKey": "00001",
            "raceUuid": "race-server-00001",
        },
        {
            "aesKeyFile": "./race-server-00002.aes",
            "displayName": "RACE Server 00002",
            "personaType": "server",
            "publicKey": "00002",
            "raceUuid": "race-server-00002",
        },
        {
            "aesKeyFile": "./race-server-00003.aes",
            "displayName": "RACE Server 00003",
            "personaType": "server",
            "publicKey": "00003",
            "raceUuid": "race-server-00003",
        },
    ]

    return generate_configs.ConfigGenState(
        prev_attempts=0,
        cli_args=cli_args,
        enable_multicast=False,
        range_config=range_config,
        committees=network_manager_utils.generate_flat_committees_from_reachability(range_config),
        personas=personas,
        req_gen_c2s=[],
        req_gen_s2s=[],
        req_dyn_c2s=[],
        req_dyn_s2s=[],
        possible_c2s=[],
        possible_s2s=[],
        bootstrap_channels=[],
        channels=[],
        requested_links=[],
        fulfilled_links=[],
        status={"attempt": 1},
    )


def default_node_state(name: str = "race-client-00001"):
    return generate_configs.NodeState(name, {}, set(), {}, [])


def default_node_states(count: int):
    return {str(i): default_node_state(str(i)) for i in range(count)}


def test_filter_channel_request1():
    request_str = "abc,ijk,geh"
    channel_dict = {"abc": 1, "def": 2, "geh": 3, "ijk": 4}
    ret = generate_configs.filter_channel_request(request_str, channel_dict)
    assert ret == [1, 4, 3]


def test_filter_channel_request2():
    request_str = "abc,ijk"
    channel_dict = {"abc": 1}
    with pytest.raises(Exception):
        generate_configs.filter_channel_request(request_str, channel_dict)


def test_filter_channel_request3():
    request_str = ""
    channel_dict = {}
    ret = generate_configs.filter_channel_request(request_str, channel_dict)
    assert ret == []


def test_roles_compatible1():
    roleA = default_role()
    roleB = default_role()
    assert generate_configs.roles_compatible(roleA, roleB)
    assert generate_configs.roles_compatible(roleB, roleA)


def test_roles_compatible2():
    roleA = default_role()
    roleA["mechanicalTags"] = ["tag1"]
    roleB = default_role()
    assert generate_configs.roles_compatible(roleA, roleB)
    assert generate_configs.roles_compatible(roleB, roleA)


def test_roles_compatible3():
    roleA = default_role()
    roleA["mechanicalTags"] = ["tag1"]
    roleB = default_role()
    roleB["mechanicalTags"] = ["tag2"]
    assert generate_configs.roles_compatible(roleA, roleB)
    assert generate_configs.roles_compatible(roleB, roleA)


def test_roles_compatible4():
    roleA = default_role()
    roleA["mechanicalTags"] = ["tag1"]
    roleB = default_role()
    roleB["mechanicalTags"] = ["tag1"]
    assert not generate_configs.roles_compatible(roleA, roleB)
    assert not generate_configs.roles_compatible(roleB, roleA)


def test_possible_roles1():
    channelA = default_channel()
    channelB = default_channel()
    assert len(generate_configs.possible_roles(channelA, channelB)) == 1


def test_possible_roles2():
    channelA = default_channel()
    channelA["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["tag1"]
    channelB = default_channel()
    channelB["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["tag1"]
    assert len(generate_configs.possible_roles(channelA, channelB)) == 0


def test_possible_roles3():
    channelA = loader_creator_channel()
    channelA["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = ["tag1"]
    channelB = default_channel()
    channelB["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["tag1"]

    # loader role conflicts, creator role is fine
    assert len(generate_configs.possible_roles(channelA, channelB)) == 1


def test_possible_roles4():
    channelA = loader_creator_channel()
    channelA["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = ["tag1"]
    channelA["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = ["tag1"]
    channelB = default_channel()
    channelB["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["tag1"]

    # both roles of A conflict
    assert len(generate_configs.possible_roles(channelA, channelB)) == 0


def test_possible_roles5():
    channelA = loader_creator_channel()
    channelA["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = []
    channelA["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = []
    channelB = default_channel()
    channelB["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["tag1"]

    # both roles of A are fine
    assert len(generate_configs.possible_roles(channelA, channelB)) == 2


def test_possible_roles6():
    channelA = default_channel()
    channelA["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = []
    channelB = loader_creator_channel()
    channelB["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = []
    channelB["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = []

    # default role doesn't conflict
    assert len(generate_configs.possible_roles(channelA, channelB)) == 1


def test_possible_roles7():
    channelA = default_channel()
    channelA["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["1"]
    channelB = loader_creator_channel()
    channelB["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = ["1"]
    channelB["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = []

    # default role conflicts with one but not the other
    assert len(generate_configs.possible_roles(channelA, channelB)) == 1


def test_possible_roles8():
    channelA = default_channel()
    channelA["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["1"]
    channelB = loader_creator_channel()
    channelB["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = ["1"]
    channelB["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = ["1"]

    # default role conflicts with both
    assert len(generate_configs.possible_roles(channelA, channelB)) == 0


def test_possible_roles9():
    channelA = loader_creator_channel()
    channelA["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = []
    channelA["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = []
    channelB = loader_creator_channel()
    channelB["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = []
    channelB["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = []

    # nothing conflict
    assert len(generate_configs.possible_roles(channelA, channelB)) == 2


def test_possible_roles10():
    channelA = loader_creator_channel()
    channelA["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = ["1"]
    channelA["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = ["1"]
    channelB = loader_creator_channel()
    channelB["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = ["1"]
    channelB["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = ["1"]

    # everything conflicts
    assert len(generate_configs.possible_roles(channelA, channelB)) == 0


def test_possible_roles10():
    channelA = loader_creator_channel()
    channelA["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = ["1"]
    channelA["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = ["2"]
    channelB = loader_creator_channel()
    channelB["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = ["2"]
    channelB["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = ["1"]

    # each role conflicts with one of the other roles, but not the other
    assert len(generate_configs.possible_roles(channelA, channelB)) == 2


def test_channels_compatible():
    channelA = default_channel()
    channelB = default_channel()

    # channel Gid are the same
    assert not generate_configs.channels_compatible(channelA, channelB)


def test_channels_compatible2():
    channelA = default_channel()
    channelA["channelGid"] = "test-channel-id-2"
    channelB = default_channel()

    # channel Gid are different and roles compatible
    assert generate_configs.channels_compatible(channelA, channelB)


def test_channels_compatible3():
    channelA = default_channel()
    channelA["channelGid"] = "test-channel-id-2"
    channelA["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["1"]
    channelB = default_channel()
    channelB["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["1"]

    # channel Gid are different and roles not compatible
    assert not generate_configs.channels_compatible(channelA, channelB)


def test_channels_compatible4():
    channelA = loader_creator_channel()
    channelA["channelGid"] = "test-channel-id-2"
    channelA["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = ["1"]
    channelA["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = ["1"]
    channelB = loader_creator_channel()
    channelB["roles"][LOADER_ROLE_INDEX]["mechanicalTags"] = ["1"]
    channelB["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = ["2"]

    # channel Gid are different and A is compatible with B, but B is not with A
    assert not generate_configs.channels_compatible(channelA, channelB)


def test_filter_compatible_channels():
    channelA = default_channel()
    channelA["channelGid"] = "test-channel-id-A"
    channelA["connectionType"] = "CT_DIRECT"
    channelA["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["1"]

    channelB = default_channel()
    channelB["channelGid"] = "test-channel-id-B"
    channelB["connectionType"] = "CT_DIRECT"
    channelB["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["2"]

    channelC = default_channel()
    channelC["channelGid"] = "test-channel-id-C"
    channelC["connectionType"] = "CT_DIRECT"
    channelC["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["3"]

    channelD = default_channel()
    channelD["channelGid"] = "test-channel-id-D"
    channelD["connectionType"] = "CT_INDIRECT"
    channelD["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["2"]

    channelE = default_channel()
    channelE["channelGid"] = "test-channel-id-E"
    channelE["connectionType"] = "CT_LOCAL"
    channelE["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = []

    channelF = default_channel()
    channelF["channelGid"] = "test-channel-id-F"
    channelF["connectionType"] = "CT_DIRECT"
    channelF["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["1"]

    channelG = default_channel()
    channelG["channelGid"] = "test-channel-id-G"
    channelG["connectionType"] = "CT_INDIRECT"
    channelG["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = ["3"]

    channels = [channelA, channelB, channelC, channelD, channelE, channelF, channelG]
    required_channels = [channelF, channelG]

    compatible_channels = generate_configs.filter_compatible_channels(
        channels, required_channels, "CT_INDIRECT"
    )
    assert len(compatible_channels) == 1

    compatible_channels = generate_configs.filter_compatible_channels(
        channels, required_channels, "CT_DIRECT"
    )
    assert len(compatible_channels) == 1

    compatible_channels = generate_configs.filter_compatible_channels(
        channels, required_channels, "CT_LOCAL"
    )
    assert len(compatible_channels) == 1


def test_link_hash1():
    link = {
        "channels": ["twoSixDirectCpp"],
        "details": {},
        "groupId": None,
        "recipients": ["race-server-00003"],
        "sender": "race-server-00001",
    }

    assert (
        generate_configs.link_hash_function(link)
        == "sender-race-server-00001-recipients-['race-server-00003']"
    )


def test_link_hash2():
    link = {
        "channels": ["twoSixDirectCpp"],
        "details": {},
        "groupId": None,
        "recipients": [
            "race-server-00001",
            "race-server-00003",
        ],
        "sender": "race-server-00002",
    }

    assert (
        generate_configs.link_hash_function(link)
        == "sender-race-server-00002-recipients-['race-server-00001', 'race-server-00003']"
    )


def test_link_hash3():
    link = {
        "channels": ["twoSixDirectCpp"],
        "details": {},
        "groupId": None,
        "recipients": [
            "race-server-00002",
            "race-server-00003",
        ],
        "sender": "race-server-00001",
    }
    link2 = {
        "channels": ["twoSixDirectCpp"],
        "details": {},
        "groupId": None,
        "recipients": [
            "race-server-00003",
            "race-server-00002",
        ],
        "sender": "race-server-00001",
    }

    # Order of recipients shouldn't matter
    assert generate_configs.link_hash_function(
        link
    ) == generate_configs.link_hash_function(link)


def test_handle_final1():
    state = default_state()

    generate_configs.handle_final_attempt(state)

    assert state.status["status"] == "complete"
    assert state.status["reason"] == "success"


def test_handle_final2():
    state = default_state()
    state.requested_links = [
        {
            "channels": ["twoSixDirectCpp"],
            "details": {},
            "groupId": None,
            "recipients": ["race-server-00003"],
            "sender": "race-server-00001",
        },
    ]

    # Have requested that's not fulfilled
    with pytest.raises(Exception):
        generate_configs.handle_final_attempt(state)


def test_handle_final3():
    state = default_state()
    state.requested_links = [
        {
            "channels": ["twoSixDirectCpp"],
            "details": {},
            "groupId": None,
            "recipients": ["race-server-00003"],
            "sender": "race-server-00001",
        },
    ]
    state.fulfilled_links = [
        {
            "channels": ["twoSixDirectCpp"],
            "details": {},
            "groupId": None,
            "recipients": ["race-server-00003"],
            "sender": "race-server-00001",
        },
    ]

    generate_configs.handle_final_attempt(state)

    assert state.status["status"] == "complete"
    assert state.status["reason"] == "success"


def assert_expected_links_match(
    expected_links: List[generate_configs.Link],
    actual_links: List[generate_configs.Link],
) -> None:
    """Assert that the given list of links are equivalent, ignoring order"""
    for expected_sender, expected_recipients in expected_links:
        found = False
        for actual_sender, actual_recipients in actual_links:
            if expected_sender == actual_sender and sorted(
                expected_recipients
            ) == sorted(actual_recipients):
                found = True
                break
        if not found:
            raise AssertionError(
                f"Expected link {(expected_sender, expected_recipients)} not found in actual links {actual_links}"
            )
    assert len(expected_links) == len(actual_links)


def test_determine_links_needed():
    state = default_state()

    c2s_links_needed, s2s_links_needed = generate_configs.determine_links_needed(state)

    c2s_expected = [
        ("race-client-00001", ["race-server-00001"]),
        ("race-server-00001", ["race-client-00001"]),
        ("race-client-00001", ["race-server-00003"]),
        ("race-server-00003", ["race-client-00001"]),
        ("race-client-00003", ["race-server-00001"]),
        ("race-server-00001", ["race-client-00003"]),
        ("race-client-00003", ["race-server-00003"]),
        ("race-server-00003", ["race-client-00003"]),
        ("race-client-00002", ["race-server-00002"]),
        ("race-server-00002", ["race-client-00002"]),
    ]

    s2s_expected = [
        ("race-server-00001", ["race-server-00003"]),
        ("race-server-00003", ["race-server-00001"]),
        ("race-server-00001", ["race-server-00002"]),
        ("race-server-00002", ["race-server-00001"]),
        ("race-server-00002", ["race-server-00003"]),
        ("race-server-00003", ["race-server-00002"]),
    ]

    assert_expected_links_match(c2s_expected, c2s_links_needed)
    assert_expected_links_match(s2s_expected, s2s_links_needed)


def test_determine_links_needed_multicast():
    state = default_state()
    state.enable_multicast = True

    c2s_links_needed, s2s_links_needed = generate_configs.determine_links_needed(state)

    c2s_expected = [
        ("race-client-00001", ["race-server-00001"]),
        ("race-server-00001", ["race-client-00001"]),
        ("race-client-00001", ["race-server-00003"]),
        ("race-server-00003", ["race-client-00001"]),
        ("race-client-00001", ["race-server-00001", "race-server-00003"]),
        ("race-client-00003", ["race-server-00001"]),
        ("race-server-00001", ["race-client-00003"]),
        ("race-client-00003", ["race-server-00003"]),
        ("race-server-00003", ["race-client-00003"]),
        ("race-client-00003", ["race-server-00001", "race-server-00003"]),
        ("race-client-00002", ["race-server-00002"]),
        ("race-server-00002", ["race-client-00002"]),
    ]

    s2s_expected = [
        ("race-server-00001", ["race-server-00003"]),
        ("race-server-00003", ["race-server-00001"]),
        ("race-server-00001", ["race-server-00002"]),
        ("race-server-00002", ["race-server-00001"]),
        ("race-server-00002", ["race-server-00003"]),
        ("race-server-00003", ["race-server-00002"]),
    ]

    assert_expected_links_match(c2s_expected, c2s_links_needed)
    assert_expected_links_match(s2s_expected, s2s_links_needed)


def test_determine_links_needed_complete_connectivity():
    state = default_state()
    state.cli_args.complete_connectivity = True

    c2s_links_needed, s2s_links_needed = generate_configs.determine_links_needed(state)

    c2s_expected = [
        ("race-client-00001", ["race-server-00001"]),
        ("race-server-00001", ["race-client-00001"]),
        ("race-client-00001", ["race-server-00002"]),
        ("race-server-00002", ["race-client-00001"]),
        ("race-client-00001", ["race-server-00003"]),
        ("race-server-00003", ["race-client-00001"]),
        ("race-client-00002", ["race-server-00001"]),
        ("race-server-00001", ["race-client-00002"]),
        ("race-client-00002", ["race-server-00002"]),
        ("race-server-00002", ["race-client-00002"]),
        ("race-client-00002", ["race-server-00003"]),
        ("race-server-00003", ["race-client-00002"]),
        ("race-client-00003", ["race-server-00001"]),
        ("race-server-00001", ["race-client-00003"]),
        ("race-client-00003", ["race-server-00002"]),
        ("race-server-00002", ["race-client-00003"]),
        ("race-client-00003", ["race-server-00003"]),
        ("race-server-00003", ["race-client-00003"]),
    ]

    s2s_expected = [
        ("race-server-00001", ["race-server-00002"]),
        ("race-server-00002", ["race-server-00001"]),
        ("race-server-00001", ["race-server-00003"]),
        ("race-server-00003", ["race-server-00001"]),
        ("race-server-00002", ["race-server-00003"]),
        ("race-server-00003", ["race-server-00002"]),
    ]

    assert_expected_links_match(c2s_expected, c2s_links_needed)
    assert_expected_links_match(s2s_expected, s2s_links_needed)


def test_find_role1():
    roles = [
        default_role(),
        default_role(),
        default_role(),
    ]
    roles[0]["linkSide"] = "LS_BOTH"
    roles[0]["roleName"] = "both"
    roles[1]["linkSide"] = "LS_CREATOR"
    roles[1]["roleName"] = "creator"
    roles[2]["linkSide"] = "LS_LOADER"
    roles[2]["roleName"] = "loader"

    assert generate_configs.find_role(roles, ["LS_BOTH"], "")["roleName"] == "both"
    assert (
        generate_configs.find_role(roles, ["LS_CREATOR"], "")["roleName"] == "creator"
    )
    assert generate_configs.find_role(roles, ["LS_LOADER"], "")["roleName"] == "loader"
    assert (
        generate_configs.find_role(roles, ["LS_BOTH", "LS_LOADER"], "")["roleName"]
        == "both"
    )
    assert (
        generate_configs.find_role(roles, ["LS_BOTH", "LS_CREATOR"], "")["roleName"]
        == "both"
    )
    assert (
        generate_configs.find_role(roles, ["LS_CREATOR", "LS_BOTH"], "")["roleName"]
        == "creator"
    )
    assert (
        generate_configs.find_role(roles, ["LS_LOADER", "LS_BOTH"], "")["roleName"]
        == "loader"
    )


def test_find_role2():
    roles = [
        default_role(),
        default_role(),
    ]
    roles[0]["linkSide"] = "LS_CREATOR"
    roles[0]["roleName"] = "creator"
    roles[1]["linkSide"] = "LS_LOADER"
    roles[1]["roleName"] = "loader"

    with pytest.raises(Exception):
        generate_configs.find_role(roles, ["LS_BOTH"], "")
    assert (
        generate_configs.find_role(roles, ["LS_CREATOR"], "")["roleName"] == "creator"
    )
    assert generate_configs.find_role(roles, ["LS_LOADER"], "")["roleName"] == "loader"
    assert (
        generate_configs.find_role(roles, ["LS_BOTH", "LS_LOADER"], "")["roleName"]
        == "loader"
    )
    assert (
        generate_configs.find_role(roles, ["LS_BOTH", "LS_CREATOR"], "")["roleName"]
        == "creator"
    )
    assert (
        generate_configs.find_role(roles, ["LS_CREATOR", "LS_BOTH"], "")["roleName"]
        == "creator"
    )
    assert (
        generate_configs.find_role(roles, ["LS_LOADER", "LS_BOTH"], "")["roleName"]
        == "loader"
    )


def test_find_role3():
    roles = [
        default_role(),
    ]
    roles[0]["linkSide"] = "LS_BOTH"
    roles[0]["roleName"] = "both"

    assert generate_configs.find_role(roles, ["LS_BOTH"], "")["roleName"] == "both"
    with pytest.raises(Exception):
        generate_configs.find_role(roles, ["LS_CREATOR"], "")
    with pytest.raises(Exception):
        generate_configs.find_role(roles, ["LS_LOADER"], "")
    assert (
        generate_configs.find_role(roles, ["LS_BOTH", "LS_LOADER"], "")["roleName"]
        == "both"
    )
    assert (
        generate_configs.find_role(roles, ["LS_BOTH", "LS_CREATOR"], "")["roleName"]
        == "both"
    )
    assert (
        generate_configs.find_role(roles, ["LS_CREATOR", "LS_BOTH"], "")["roleName"]
        == "both"
    )
    assert (
        generate_configs.find_role(roles, ["LS_LOADER", "LS_BOTH"], "")["roleName"]
        == "both"
    )


def test_role_compatible_with_node1():
    node = default_node_state()
    role = default_role()
    channel_id = "channel-id"
    assert generate_configs.role_compatible_with_node(node, role, channel_id)


def test_role_compatible_with_node1():
    node = default_node_state()
    role = default_role()
    channel_id = "channel-id"
    node.channel_roles[channel_id] = role
    assert generate_configs.role_compatible_with_node(node, role, channel_id)


def test_role_compatible_with_node2():
    node = default_node_state()
    role = default_role()
    role["roleName"] = "other"
    channel_id = "channel-id"
    node.channel_roles[channel_id] = default_role()
    assert not generate_configs.role_compatible_with_node(node, role, channel_id)


def test_role_compatible_with_node3():
    node = default_node_state()
    node.mechanical_tags.add("tag")
    role = default_role()
    role["mechanicalTags"] = ["tag"]
    channel_id = "channel-id"
    assert not generate_configs.role_compatible_with_node(node, role, channel_id)


def test_role_compatible_with_node4():
    node = default_node_state()
    node.mechanical_tags.add("tag1")
    role = default_role()
    role["mechanicalTags"] = ["tag2"]
    channel_id = "channel-id"
    assert generate_configs.role_compatible_with_node(node, role, channel_id)


def test_add_role_to_node1():
    node = default_node_state()
    role = default_role()
    channel_id = "channel-id"

    generate_configs.add_role_to_node(node, role, channel_id)

    assert node.channel_roles[channel_id] == role
    assert node.mechanical_tags == set()


def test_add_role_to_node2():
    node = default_node_state()
    node.mechanical_tags.add("tag1")
    node.mechanical_tags.add("tag2")
    role = default_role()
    role["mechanicalTags"] = ["tag3", "tag4"]
    channel_id = "channel-id"

    generate_configs.add_role_to_node(node, role, channel_id)

    assert node.channel_roles[channel_id] == role
    assert sorted(node.mechanical_tags) == ["tag1", "tag2", "tag3", "tag4"]


def test_matching_link_side():
    assert generate_configs.matching_link_side("LS_BOTH") == "LS_BOTH"
    assert generate_configs.matching_link_side("LS_LOADER") == "LS_CREATOR"
    assert generate_configs.matching_link_side("LS_CREATOR") == "LS_LOADER"
    with pytest.raises(Exception):
        generate_configs.matching_link_side("invalid")


def test_fulfill_request1():
    channel = default_channel()

    sender = default_node_state()
    receiver = default_node_state()

    assert generate_configs.fulfill_request(channel, sender, [receiver], True)
    assert (
        sender.channel_roles[channel["channelGid"]]
        == channel["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        receiver.channel_roles[channel["channelGid"]]
        == channel["roles"][DEFAULT_ROLE_INDEX]
    )


def test_fulfill_request2():
    channel = default_channel()
    channel["linkDirection"] = "LD_LOADER_TO_CREATOR"

    sender = default_node_state()
    receiver = default_node_state()

    assert generate_configs.fulfill_request(channel, sender, [receiver], True)
    assert (
        sender.channel_roles[channel["channelGid"]]
        == channel["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        receiver.channel_roles[channel["channelGid"]]
        == channel["roles"][DEFAULT_ROLE_INDEX]
    )


def test_fulfill_request3():
    channel = default_channel()
    channel["linkDirection"] = "LD_CREATOR_TO_LOADER"

    sender = default_node_state()
    receiver = default_node_state()

    assert generate_configs.fulfill_request(channel, sender, [receiver], True)
    assert (
        sender.channel_roles[channel["channelGid"]]
        == channel["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        receiver.channel_roles[channel["channelGid"]]
        == channel["roles"][DEFAULT_ROLE_INDEX]
    )


def test_fulfill_request4():
    channel = loader_creator_channel()

    sender = default_node_state()
    receiver = default_node_state()

    assert generate_configs.fulfill_request(channel, sender, [receiver], True)
    assert (
        sender.channel_roles[channel["channelGid"]]
        == channel["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        receiver.channel_roles[channel["channelGid"]]
        == channel["roles"][CREATOR_ROLE_INDEX]
    )


def test_fulfill_request5():
    channel = loader_creator_channel()
    channel["linkDirection"] = "LD_LOADER_TO_CREATOR"

    sender = default_node_state()
    receiver = default_node_state()

    assert generate_configs.fulfill_request(channel, sender, [receiver], True)
    assert (
        sender.channel_roles[channel["channelGid"]]
        == channel["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        receiver.channel_roles[channel["channelGid"]]
        == channel["roles"][CREATOR_ROLE_INDEX]
    )


def test_fulfill_request6():
    channel = loader_creator_channel()
    channel["linkDirection"] = "LD_CREATOR_TO_LOADER"

    sender = default_node_state()
    receiver = default_node_state()

    assert generate_configs.fulfill_request(channel, sender, [receiver], True)
    assert (
        sender.channel_roles[channel["channelGid"]]
        == channel["roles"][CREATOR_ROLE_INDEX]
    )
    assert (
        receiver.channel_roles[channel["channelGid"]]
        == channel["roles"][LOADER_ROLE_INDEX]
    )


def test_fulfill_request7():
    channelA = loader_creator_channel()
    channelB = loader_creator_channel()

    sender = default_node_state()
    receiver = default_node_state()

    assert generate_configs.fulfill_request(channelA, sender, [receiver], True)
    assert (
        sender.channel_roles[channelA["channelGid"]]
        == channelA["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        receiver.channel_roles[channelA["channelGid"]]
        == channelA["roles"][CREATOR_ROLE_INDEX]
    )

    assert generate_configs.fulfill_request(channelB, sender, [receiver], True)
    assert (
        sender.channel_roles[channelB["channelGid"]]
        == channelB["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        receiver.channel_roles[channelB["channelGid"]]
        == channelB["roles"][CREATOR_ROLE_INDEX]
    )

    # Make sure that doesn't change channelA role
    assert (
        sender.channel_roles[channelA["channelGid"]]
        == channelA["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        receiver.channel_roles[channelA["channelGid"]]
        == channelA["roles"][CREATOR_ROLE_INDEX]
    )


def test_fulfill_request8():
    channelA = loader_creator_channel()

    node1 = default_node_state()
    node2 = default_node_state()

    assert generate_configs.fulfill_request(channelA, node1, [node2], True)
    assert (
        node1.channel_roles[channelA["channelGid"]]
        == channelA["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        node2.channel_roles[channelA["channelGid"]]
        == channelA["roles"][CREATOR_ROLE_INDEX]
    )

    assert generate_configs.fulfill_request(channelA, node2, [node1], True)
    assert (
        node1.channel_roles[channelA["channelGid"]]
        == channelA["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        node2.channel_roles[channelA["channelGid"]]
        == channelA["roles"][CREATOR_ROLE_INDEX]
    )


def test_fulfill_request9():
    channelA = loader_creator_channel()
    channelA["linkDirection"] = "LD_LOADER_TO_CREATOR"

    node1 = default_node_state()
    node2 = default_node_state()

    assert generate_configs.fulfill_request(channelA, node1, [node2], True)
    assert (
        node1.channel_roles[channelA["channelGid"]]
        == channelA["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        node2.channel_roles[channelA["channelGid"]]
        == channelA["roles"][CREATOR_ROLE_INDEX]
    )

    assert not generate_configs.fulfill_request(channelA, node2, [node1], True)


def test_fulfill_request10():
    channelA = default_channel()
    channelA["linkDirection"] = "LD_LOADER_TO_CREATOR"

    node1 = default_node_state()
    node2 = default_node_state()

    assert generate_configs.fulfill_request(channelA, node1, [node2], True)
    assert (
        node1.channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        node2.channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )

    assert generate_configs.fulfill_request(channelA, node2, [node1], True)
    assert (
        node1.channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        node2.channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )


def test_fulfill_request11():
    channelA = loader_creator_channel()

    node1 = default_node_state()
    node2 = default_node_state()
    node3 = default_node_state()

    assert generate_configs.fulfill_request(channelA, node1, [node2], True)
    assert (
        node1.channel_roles[channelA["channelGid"]]
        == channelA["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        node2.channel_roles[channelA["channelGid"]]
        == channelA["roles"][CREATOR_ROLE_INDEX]
    )

    assert generate_configs.fulfill_request(channelA, node3, [node1], True)
    assert (
        node1.channel_roles[channelA["channelGid"]]
        == channelA["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        node3.channel_roles[channelA["channelGid"]]
        == channelA["roles"][CREATOR_ROLE_INDEX]
    )

    assert not generate_configs.fulfill_request(channelA, node3, [node2], True)


def test_fulfill_request12():
    channelA = loader_creator_channel()
    channelA["linkDirection"] = "Invalid"

    node1 = default_node_state()
    node2 = default_node_state()

    with pytest.raises(Exception):
        generate_configs.fulfill_request(channelA, node1, [node2], True)


def test_fulfill_request13():
    channel = default_channel()

    sender = default_node_state()
    receiver = default_node_state()

    assert generate_configs.fulfill_request(channel, sender, [receiver], False)
    assert sender.channel_roles.get(channel["channelGid"]) == None
    assert receiver.channel_roles.get(channel["channelGid"]) == None


def test_fulfill_request14():
    channel = loader_creator_channel()
    channel["linkDirection"] = "LD_CREATOR_TO_LOADER"

    sender = default_node_state()
    receiver1 = default_node_state()
    receiver2 = default_node_state()

    assert generate_configs.fulfill_request(
        channel, sender, [receiver1, receiver2], True
    )
    assert (
        sender.channel_roles[channel["channelGid"]]
        == channel["roles"][CREATOR_ROLE_INDEX]
    )
    assert (
        receiver1.channel_roles[channel["channelGid"]]
        == channel["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        receiver2.channel_roles[channel["channelGid"]]
        == channel["roles"][LOADER_ROLE_INDEX]
    )


def test_create_links1():
    channelA = default_channel()
    node_states = default_node_states(3)
    links_needed = [("0", "1"), ("1", "0")]
    network_manager_request = []

    generate_configs.create_links([channelA], node_states, network_manager_request, links_needed)

    assert len(network_manager_request) == 2
    assert (
        node_states["0"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        node_states["1"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )
    assert node_states["2"].channel_roles.get(channelA["channelGid"]) == None


def test_create_links2():
    channelA = default_channel()
    node_states = default_node_states(3)
    links_needed = [
        ("0", "1"),
        ("1", "0"),
        ("1", "2"),
        ("2", "1"),
        ("2", "0"),
        ("0", "2"),
    ]
    network_manager_request = []

    generate_configs.create_links([channelA], node_states, network_manager_request, links_needed)

    assert len(network_manager_request) == 6
    assert (
        node_states["0"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        node_states["1"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        node_states["2"].channel_roles.get(channelA["channelGid"])
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )


def test_create_links3():
    channelA = loader_creator_channel()
    node_states = default_node_states(3)
    links_needed = [
        ("0", "1"),
        ("1", "0"),
        ("1", "2"),
        ("2", "1"),
        ("2", "0"),
        ("0", "2"),
    ]
    network_manager_request = []

    with pytest.raises(Exception):
        generate_configs.create_links(
            [channelA], node_states, network_manager_request, links_needed
        )


def test_create_links4():
    channelA = loader_creator_channel()
    channelA["channelGid"] = "channelA"
    channelB = loader_creator_channel()
    channelB["channelGid"] = "channelB"
    node_states = default_node_states(3)
    links_needed = [
        ("0", "1"),
        ("1", "0"),
        ("1", "2"),
        ("2", "1"),
        ("2", "0"),
        ("0", "2"),
    ]
    network_manager_request = []

    generate_configs.create_links(
        [channelA, channelB], node_states, network_manager_request, links_needed
    )

    assert len(network_manager_request) == 6
    assert (
        node_states["0"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        node_states["1"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][CREATOR_ROLE_INDEX]
    )
    assert (
        node_states["2"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        node_states["0"].channel_roles[channelB["channelGid"]]
        == channelB["roles"][CREATOR_ROLE_INDEX]
    )
    assert node_states["1"].channel_roles.get(channelB["channelGid"]) == None
    assert (
        node_states["2"].channel_roles[channelB["channelGid"]]
        == channelB["roles"][LOADER_ROLE_INDEX]
    )


def test_assign_dyn_channel_roles1():
    channelA = loader_creator_channel()
    channelA["channelGid"] = "channelA"
    channelB = loader_creator_channel()
    channelB["channelGid"] = "channelB"
    node_states = default_node_states(3)
    links_needed = [
        ("0", "1"),
        ("1", "0"),
        ("1", "2"),
        ("2", "1"),
        ("2", "0"),
        ("0", "2"),
    ]

    generate_configs.assign_dyn_channel_roles(
        [channelA, channelB], node_states, links_needed
    )

    assert (
        node_states["0"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        node_states["1"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][CREATOR_ROLE_INDEX]
    )
    assert (
        node_states["2"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        node_states["0"].channel_roles[channelB["channelGid"]]
        == channelB["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        node_states["1"].channel_roles[channelB["channelGid"]]
        == channelB["roles"][CREATOR_ROLE_INDEX]
    )
    assert (
        node_states["2"].channel_roles[channelB["channelGid"]]
        == channelB["roles"][LOADER_ROLE_INDEX]
    )


def test_assign_dyn_channel_roles2():
    channelA = default_channel()
    channelA["channelGid"] = "channelA"
    channelA["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = "tag"
    channelB = default_channel()
    channelB["channelGid"] = "channelB"
    channelB["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = "tag"

    node_states = default_node_states(3)
    links_needed = [
        ("0", "1"),
        ("1", "0"),
        ("1", "2"),
        ("2", "1"),
        ("2", "0"),
        ("0", "2"),
    ]

    generate_configs.assign_dyn_channel_roles(
        [channelA, channelB], node_states, links_needed
    )

    assert (
        node_states["0"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        node_states["1"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        node_states["2"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )
    assert node_states["0"].channel_roles.get(channelB["channelGid"]) == None
    assert node_states["1"].channel_roles.get(channelB["channelGid"]) == None
    assert node_states["2"].channel_roles.get(channelB["channelGid"]) == None


def test_assign_dyn_channel_roles3():
    channelA = loader_creator_channel()
    channelA["channelGid"] = "channelA"
    channelA["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = "tag"
    channelB = default_channel()
    channelB["channelGid"] = "channelB"
    channelB["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = "tag"

    node_states = default_node_states(3)
    links_needed = [
        ("0", "1"),
        ("1", "0"),
        ("1", "2"),
        ("2", "1"),
        ("2", "0"),
        ("0", "2"),
    ]

    generate_configs.assign_dyn_channel_roles(
        [channelA, channelB], node_states, links_needed
    )

    assert (
        node_states["0"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        node_states["1"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][CREATOR_ROLE_INDEX]
    )
    assert (
        node_states["2"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][LOADER_ROLE_INDEX]
    )
    assert (
        node_states["0"].channel_roles[channelB["channelGid"]]
        == channelB["roles"][DEFAULT_ROLE_INDEX]
    )
    assert node_states["1"].channel_roles.get(channelB["channelGid"]) == None
    assert (
        node_states["2"].channel_roles[channelB["channelGid"]]
        == channelB["roles"][DEFAULT_ROLE_INDEX]
    )


def test_assign_bootstrap_channel_roles1():
    channelA = default_channel()
    channelA["channelGid"] = "channelA"
    channelB = default_channel()
    channelB["channelGid"] = "channelB"
    node_states = default_node_states(3)

    generate_configs.assign_bootstrap_channel_roles([channelA, channelB], node_states)

    assert (
        node_states["0"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        node_states["1"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        node_states["2"].channel_roles[channelA["channelGid"]]
        == channelA["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        node_states["0"].channel_roles[channelB["channelGid"]]
        == channelB["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        node_states["1"].channel_roles[channelB["channelGid"]]
        == channelB["roles"][DEFAULT_ROLE_INDEX]
    )
    assert (
        node_states["2"].channel_roles[channelB["channelGid"]]
        == channelB["roles"][DEFAULT_ROLE_INDEX]
    )


def test_assign_bootstrap_channel_roles2():
    channelA = default_channel()
    channelA["channelGid"] = "channelA"
    channelA["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = "tag"
    channelB = default_channel()
    channelB["channelGid"] = "channelB"
    channelB["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = "tag"
    node_states = default_node_states(3)

    with pytest.raises(Exception):
        generate_configs.assign_bootstrap_channel_roles(
            [channelA, channelB], node_states
        )


def test_assign_bootstrap_channel_roles3():
    channelA = loader_creator_channel()
    node_states = default_node_states(3)

    with pytest.raises(Exception):
        generate_configs.assign_bootstrap_channel_roles([channelA], node_states)


def test_check_req_links1():
    channelA = default_channel()
    channelA["channelGid"] = "channelA"
    channelB = default_channel()
    channelB["channelGid"] = "channelB"
    node_states = default_node_states(3)
    links_needed = [
        ("0", "1"),
        ("1", "0"),
        ("1", "2"),
        ("2", "1"),
        ("2", "0"),
        ("0", "2"),
    ]
    network_manager_request = []

    generate_configs.create_links([channelA], node_states, network_manager_request, links_needed)

    # No exception
    generate_configs.check_req_links([channelA], node_states, links_needed)

    # Exception
    with pytest.raises(Exception):
        generate_configs.check_req_links([channelB], node_states, links_needed)
    with pytest.raises(Exception):
        generate_configs.check_req_links(
            [channelA, channelB], node_states, links_needed
        )


def test_check_req_links2():
    channelA = loader_creator_channel()
    channelA["channelGid"] = "channelA"
    channelA["roles"][CREATOR_ROLE_INDEX]["mechanicalTags"] = "tag"
    channelB = default_channel()
    channelB["channelGid"] = "channelB"
    channelB["roles"][DEFAULT_ROLE_INDEX]["mechanicalTags"] = "tag"

    node_states = default_node_states(3)
    links_needed = [
        ("0", "1"),
        ("1", "0"),
        ("1", "2"),
        ("2", "1"),
        ("2", "0"),
        ("0", "2"),
    ]

    generate_configs.assign_dyn_channel_roles(
        [channelA, channelB], node_states, links_needed
    )

    # No Exception
    generate_configs.check_req_links([channelA, channelB], node_states, links_needed)
