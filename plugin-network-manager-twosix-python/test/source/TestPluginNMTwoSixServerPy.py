#!/usr/bin/env python3.6

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
        Test File for PluginNMTwoSixServerPy.py
"""

# Python Library Imports
import inspect
import os
import sys
import pytest
from unittest import mock

# Set Base Path
BASE_PROJECT_PATH = f"{os.path.dirname(os.path.realpath(__file__))}/../../"
sys.path.insert(0, f"{BASE_PROJECT_PATH}/source")
sys.path.insert(0, f"{BASE_PROJECT_PATH}/lib")
sys.path.insert(0, f"{BASE_PROJECT_PATH}/include")

# RACE Libraries
from PluginNMTwoSixServerPy import PluginNMTwoSixServerPy
from RaceCrypto import RaceCrypto


###
# Fixtures
###


@pytest.fixture
def server_obj():
    """
    Purpose:
        Create server_obj object to test
    Args:
        N/A
    Return:
        server_obj (Pytest Fixture (PluginNMTwoSixServerPy Object)): Object to test
    """

    return PluginNMTwoSixServerPy()


###
# Mocked Functions
###


# None at the Moment (Empty Test Suite)


###
# Test Functions
###


def test_server_constructs(server_obj):
    """
    Purpose:
        Tests that PluginNMTwoSixServerPy is initialized as expected and has
        the epected methods, args, and values
    Args:
        server_obj (Pytest Fixture (PluginNMTwoSixServerPy Object)): Object to test
    Return:
        N/A
    """

    # Checking initialization values
    assert isinstance(server_obj, PluginNMTwoSixServerPy)
    assert server_obj.racePersona == None
    assert server_obj.racePersonas == {}
    assert server_obj.etcDirectory == None
    assert server_obj.reachableClients == []
    assert server_obj.reachableIntraCommitteeServers == []
    assert server_obj.reachableInterCommitteeServers == []
    assert server_obj.connectionPersonaMap == {}
    assert isinstance(server_obj.encryptor, RaceCrypto)

    # Commented out methods not enforced by core
    expected_methods = [
        "init",
        "start",
        "shutdown",
        "processClrMsg",
        "processEncPkg",
        "getPersonaPublicKey",
        # "openRecvConns",
        # "closeRecvConns",
        # "loadPersonas",
        # "configureCommittees",
    ]
    server_obj_methods = inspect.getmembers(server_obj, predicate=inspect.ismethod)
    server_obj_method_names = [
        server_obj_method[0] for server_obj_method in server_obj_methods
    ]
    for expected_method in expected_methods:
        assert expected_method in server_obj_method_names


# if __name__ == "__main__":

#     # Init and startup
#     sdk = RaceSdk()
#     plugin_obj = PluginNMTwoSixServerPy()
#     plugin_obj.init(sdk, "./global.json", "../config/")
#     plugin_obj.start()

#     # Message
#     msg_to_send = "Hey man, what's up with you today"
#     msg_from = "race-server-1"
#     msg_to = "race-server-2"
#     msg_time = 1_571_880_413
#     msg_nonce = 1
#     clrmsg_to_send = ClrMsg(msg_to_send, msg_from, msg_to, msg_time, msg_nonce)
#     formatted_msg = plugin_obj.encryptor.formatDelimitedMessage(clrmsg_to_send)
#     parsed_msg = plugin_obj.encryptor.parseDelimitedMessage(formatted_msg)

#     # Send
#     try:
#         plugin_obj.processClrMsg(clrmsg_to_send)
#     except Exception as err:
#         print(f"Got exception calling processClrMsg as expected: {err}")

#     # EncPkg
#     encoded_cipher = plugin_obj.encryptor.encryptClrMsg(formatted_msg, 1)
#     ePkg = EncPkg(encoded_cipher)

#     # Receive
#     plugin_obj.processEncPkg(ePkg, ["race-server-1_link_LT_RECV_conn"])

#     plugin_obj.shutdown()

#     import pdb, gnureadline

#     pdb.set_trace()
