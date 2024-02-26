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
        Test File for client.py
"""

# Python Library Imports
import os
import sys
import pytest
from unittest import mock

# Local Library Imports
from race_python_utils.network_manager.client import Client
from race_python_utils.network_manager.committee import Committee


###
# Mocks/Data Fixtures
###


# N/A


###
# Test Payload
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

    expected_name = "race-client-1"
    client_obj = Client(name=expected_name)

    assert str(client_obj) == client_obj.name
    assert client_obj.name == expected_name
    assert client_obj.reachable_servers == []
    assert client_obj.entrance_committee == []
    assert client_obj.exit_committee == []


################################################################################
# json_config
################################################################################


def test_json_config():
    c = Client()
    conf = c.json_config()
    assert isinstance(conf["entranceCommittee"], list)
    assert isinstance(conf["exitCommittee"], list)
