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
        Test File for server.py
"""

# Python Library Imports
import os
import sys
import pytest
from unittest import mock

# Local Library Imports
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


def test___init__() -> int:
    """
    Purpose:
        Test __init__ works
    Args:
        N/A
    """

    expected_name = "race-server-1"
    server_obj = Server(name=expected_name)

    assert str(server_obj) == server_obj.name
    assert server_obj.name == expected_name
    assert server_obj.reachable_servers == []
    assert server_obj.reachable_clients == []
    assert server_obj.exit_clients == []
    assert isinstance(server_obj.committee, Committee)


################################################################################
# json_config
################################################################################


def test_json_config() -> int:
    """
    Purpose:
        Test json_config works
    Args:
        N/A
    """

    race_server_1 = Server(name="race-server-1")
    race_server_2 = Server(name="race-server-2")
    race_server_1.reachable_servers = [race_server_2]
    race_server_2.reachable_servers = [race_server_1]

    conf = race_server_1.json_config()
    assert isinstance(conf["committeeName"], str)
    assert isinstance(conf["exitClients"], list)
    assert isinstance(conf["committeeClients"], list)
    assert isinstance(conf["reachableCommittees"], dict)
    assert isinstance(conf["rings"], list)
    assert isinstance(conf["floodingFactor"], int)
