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
        Test File for RaceCrypto.py
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
from RaceCrypto import RaceCrypto


###
# Fixtures
###


@pytest.fixture
def encryptor():
    """
    Purpose:
        Create encryptor object to test
    Args:
        N/A
    Return:
        encryptor (Pytest Fixture (RaceCrypto Object)): RaceCrypto Object to test
    """

    return RaceCrypto()


@pytest.fixture
def example_plain_msg():
    """
    Purpose:
        Set example msg for encryption
    Args:
        N/A
    Return:
        example_plain_msg (Pytest Fixture (String)): example plain msg
    """

    return "clrMsg~~~Hey man, what's up with you today~~~race-client-1~~~race-client-2"


@pytest.fixture
def example_seed():
    """
    Purpose:
        Set example seed for encryption/decryption
    Args:
        N/A
    Return:
        example_seed (Pytest Fixture (int)): example seed
    """

    return 1


###
# Mocked Functions
###


# None at the Moment (Empty Test Suite)


###
# Test Functions
###


def test_encryptor_constructs(encryptor):
    """
    Purpose:
        Tests that RaceCrypto is initialized as expected and has
        the epected methods, args, and values
    Args:
        encryptor (Pytest Fixture (RaceCrypto Object)): RaceCrypto Object to test
    Return:
        N/A
    """

    assert isinstance(encryptor, RaceCrypto)
    assert encryptor.delimiter == "~~~"

    expected_methods = [
        "encryptClrMsg",
        "decryptEncPkg",
        "getDelimiter",
        "formatDelimitedMessage",
        "parseDelimitedMessage",
    ]
    encryptor_methods = inspect.getmembers(encryptor, predicate=inspect.ismethod)
    encryptor_method_names = [
        encryptor_method[0] for encryptor_method in encryptor_methods
    ]
    for expected_method in expected_methods:
        assert expected_method in encryptor_method_names


def test_encryptClrMsg_decryptEncPkg(encryptor, example_plain_msg, example_seed):
    """
    Purpose:
        Tests that encryptClrMsg encrypts a message as expected
        then decryptEncPkg decrypts that message as expected
    Args:
        encryptor (Pytest Fixture (RaceCrypto Object)): RaceCrypto Object to test
        example_plain_msg (Pytest Fixture (String)): example plain msg
        example_seed (Pytest Fixture (int)): example seed
    Return:
        N/A
    """

    assert (
        encryptor.decryptEncPkg(
            encryptor.encryptClrMsg(example_plain_msg, example_seed), example_seed
        )
        == example_plain_msg
    )
