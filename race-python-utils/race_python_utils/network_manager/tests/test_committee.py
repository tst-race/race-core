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
        Test File for committe.py
"""

# Python Library Imports
import os
import sys
import pytest
from unittest import mock

# Local Library Imports
from race_python_utils.network_manager import committee
from race_python_utils.network_manager.client import Client
from race_python_utils.network_manager.committee import Committee
from race_python_utils.network_manager.server import Server


###
# Mocks/Data Fixtures
###


# N/A


###
# Tests
###


################################################################################
# __init__
################################################################################


def test___init___empty() -> int:
    """
    Purpose:
        Test __init__ works with nothing set
    Args:
        N/A
    """

    expected_name = ""
    expected_flooding_factor = 2

    committee_obj = Committee()

    assert str(committee_obj) == committee_obj.name
    assert committee_obj.name == expected_name
    assert committee_obj.flooding_factor == expected_flooding_factor
    assert committee_obj.servers == []
    assert committee_obj.clients == []
    assert committee_obj.rings == []


def test___init___name() -> int:
    """
    Purpose:
        Test __init__ works with name set
    Args:
        N/A
    """

    expected_name = "test"
    expected_flooding_factor = 2

    committee_obj = Committee(name=expected_name)

    assert str(committee_obj) == committee_obj.name
    assert committee_obj.name == expected_name
    assert committee_obj.flooding_factor == expected_flooding_factor
    assert committee_obj.servers == []
    assert committee_obj.clients == []
    assert committee_obj.rings == []


def test___init___flooding_factor() -> int:
    """
    Purpose:
        Test __init__ works with flooding factor set
    Args:
        N/A
    """

    expected_name = ""
    expected_flooding_factor = -10000

    committee_obj = Committee(flooding_factor=expected_flooding_factor)

    assert str(committee_obj) == committee_obj.name
    assert committee_obj.name == expected_name
    assert committee_obj.flooding_factor == expected_flooding_factor
    assert committee_obj.servers == []
    assert committee_obj.clients == []
    assert committee_obj.rings == []


################################################################################
# generate_rings
################################################################################


def test_generate_rings_1_ring_0_server() -> int:
    """
    Purpose:
        Test generate_rings works
    Args:
        N/A
    """

    expected_name = "generate-rings-test"
    expected_flooding_factor = 2

    committee_obj_0_servers = Committee(name=expected_name)
    committee_obj_0_servers.generate_rings()

    assert len(committee_obj_0_servers.rings) == 0
    assert sorted(committee_obj_0_servers.rings) == []


def test_generate_rings_1_ring_1_server() -> int:
    """
    Purpose:
        Test generate_rings works
    Args:
        N/A
    """

    expected_name = "generate-rings-test"
    expected_flooding_factor = 2

    committee_obj_1_servers = Committee(name=expected_name)
    race_server_1 = Server(name="race-server-1")
    committee_obj_1_servers.servers.append(race_server_1)
    committee_obj_1_servers.generate_rings()

    assert len(committee_obj_1_servers.rings) == 1
    assert sorted(committee_obj_1_servers.rings[0]) == ["race-server-1"]


def test_generate_rings_1_ring_1_server() -> int:
    """
    Purpose:
        Test generate_rings works
    Args:
        N/A
    """

    expected_name = "generate-rings-test"
    expected_flooding_factor = 2

    committee_obj_2_servers = Committee(name=expected_name)
    race_server_1 = Server(name="race-server-1")
    race_server_2 = Server(name="race-server-2")
    race_server_1.reachable_servers = [race_server_2]
    race_server_2.reachable_servers = [race_server_1]
    committee_obj_2_servers.servers.append(race_server_1)
    committee_obj_2_servers.servers.append(race_server_2)
    committee_obj_2_servers.generate_rings()

    assert len(committee_obj_2_servers.rings) == 1
    assert sorted(committee_obj_2_servers.rings[0]) == [
        "race-server-1",
        "race-server-2",
    ]


def test_generate_rings_1_ring_1_server() -> int:
    """
    Purpose:
        Test generate_rings works
    Args:
        N/A
    """

    expected_name = "generate-rings-test"
    expected_flooding_factor = 2

    committee_obj_3_servers = Committee(name=expected_name)
    race_server_1 = Server(name="race-server-1")
    race_server_2 = Server(name="race-server-2")
    race_server_3 = Server(name="race-server-3")
    race_server_1.reachable_servers = [race_server_2, race_server_3]
    race_server_2.reachable_servers = [race_server_1, race_server_3]
    race_server_3.reachable_servers = [race_server_1, race_server_2]
    committee_obj_3_servers.servers.append(race_server_1)
    committee_obj_3_servers.servers.append(race_server_2)
    committee_obj_3_servers.servers.append(race_server_3)
    committee_obj_3_servers.generate_rings()

    assert len(committee_obj_3_servers.rings) == 1
    assert sorted(committee_obj_3_servers.rings[0]) == [
        "race-server-1",
        "race-server-2",
        "race-server-3",
    ]


def test_generate_rings_1_ring_4_server() -> int:
    """
    Purpose:
        Test generate_rings works
    Args:
        N/A
    """

    expected_name = "generate-rings-test"
    expected_flooding_factor = 2

    committee_obj_4_servers = Committee(name=expected_name)
    race_server_1 = Server(name="race-server-1")
    race_server_2 = Server(name="race-server-2")
    race_server_3 = Server(name="race-server-3")
    race_server_4 = Server(name="race-server-4")
    race_server_1.reachable_servers = [race_server_2, race_server_3, race_server_4]
    race_server_2.reachable_servers = [race_server_1, race_server_3, race_server_4]
    race_server_3.reachable_servers = [race_server_1, race_server_2, race_server_4]
    race_server_4.reachable_servers = [race_server_1, race_server_2, race_server_3]
    committee_obj_4_servers.servers.append(race_server_1)
    committee_obj_4_servers.servers.append(race_server_2)
    committee_obj_4_servers.servers.append(race_server_3)
    committee_obj_4_servers.servers.append(race_server_4)
    committee_obj_4_servers.generate_rings()

    assert len(committee_obj_4_servers.rings) == 1
    assert sorted(committee_obj_4_servers.rings[0]) == [
        "race-server-1",
        "race-server-2",
        "race-server-3",
        "race-server-4",
    ]


################################################################################
# gather_reachable_committees
################################################################################


def test_gather_reachable_committees() -> int:
    """
    Purpose:
        Test gather_reachable_committees works
    Args:
        N/A
    """

    pass


################################################################################
# get_rings_for_server
################################################################################


def test_get_rings_for_server() -> int:
    """
    Purpose:
        Test get_rings_for_server works
    Args:
        N/A
    """

    expected_name = "generate-rings-test"
    expected_flooding_factor = 2

    committee_obj_4_servers = Committee(name=expected_name)
    race_server_1 = Server(name="race-server-1")
    race_server_2 = Server(name="race-server-2")
    race_server_3 = Server(name="race-server-3")
    race_server_4 = Server(name="race-server-4")
    race_server_1.reachable_servers = [race_server_2, race_server_3, race_server_4]
    race_server_2.reachable_servers = [race_server_1, race_server_3, race_server_4]
    race_server_3.reachable_servers = [race_server_1, race_server_2, race_server_4]
    race_server_4.reachable_servers = [race_server_1, race_server_2, race_server_3]
    committee_obj_4_servers.servers.append(race_server_1)
    committee_obj_4_servers.servers.append(race_server_2)
    committee_obj_4_servers.servers.append(race_server_3)
    committee_obj_4_servers.servers.append(race_server_4)
    committee_obj_4_servers.generate_rings()

    returned_rings = committee_obj_4_servers.get_rings_for_server(race_server_4)

    assert len(returned_rings) == 1
    assert returned_rings[0].get("next", None) is not None
    assert returned_rings[0].get("length", None) == 4
