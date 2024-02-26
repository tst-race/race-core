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
        Test File for PluginNMTwoSixClientPy.py
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
from PluginNMTwoSixClientPy import PluginNMTwoSixClientPy
from RaceCrypto import RaceCrypto


###
# Fixtures
###


@pytest.fixture
def client_obj():
    """
    Purpose:
        Create client_obj object to test
    Args:
        N/A
    Return:
        client_obj (Pytest Fixture (PluginNMTwoSixClientPy Object)): Object to test
    """

    return PluginNMTwoSixClientPy()


###
# Mocked Functions
###


# None at the Moment (Empty Test Suite)


###
# Test Functions
###


def test_client_constructs(client_obj):
    """
    Purpose:
        Tests that PluginNMTwoSixClientPy is initialized as expected and has
        the epected methods, args, and values
    Args:
        client_obj (Pytest Fixture (PluginNMTwoSixClientPy Object)): Object to test
    Return:
        N/A
    """

    # Checking initialization values
    assert isinstance(client_obj, PluginNMTwoSixClientPy)
    assert client_obj.racePersona == None
    assert client_obj.racePersonas == {}
    assert client_obj.etcDirectory == None
    assert client_obj.entranceCommittee == []
    assert client_obj.exitCommittee == []
    assert client_obj.connectionPersonaMap == {}
    assert isinstance(client_obj.encryptor, RaceCrypto)

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
    client_obj_methods = inspect.getmembers(client_obj, predicate=inspect.ismethod)
    client_obj_method_names = [
        client_obj_method[0] for client_obj_method in client_obj_methods
    ]
    for expected_method in expected_methods:
        assert expected_method in client_obj_method_names


# if __name__ == "__main__":

#     # Init and startup
#     sdk = IRaceSdkNM()
#     plugin_obj = PluginNMTwoSixClientPy()
#     plugin_obj.init(sdk, "./global.json", "../config/")
#     plugin_obj.start()

#     # Message
#     msg_to_send = "Hey man, what's up with you today"
#     msg_from = "race-client-1"
#     msg_to = "race-client-2"
#     msg_time = 1_571_880_413
#     msg_nonce = 1
#     clrmsg_to_send = ClrMsg(msg_to_send, msg_from, msg_to, msg_time, msg_nonce)
#     formatted_msg = plugin_obj.encryptor.formatDelimitedMessage(clrmsg_to_send)
#     parsed_msg = plugin_obj.encryptor.parseDelimitedMessage(formatted_msg)

#     # Send
#     plugin_obj.processClrMsg(clrmsg_to_send)

#     # EncPkg
#     encoded_cipher = plugin_obj.encryptor.encryptClrMsg(formatted_msg, 1)
#     ePkg = EncPkg(encoded_cipher)

#     # Receive
#     plugin_obj.processEncPkg(ePkg, ["race-server-1_link_LT_RECV_conn"])

#     plugin_obj.shutdown()

#     import pdb, gnureadline

#     pdb.set_trace()
