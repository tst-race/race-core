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
        Test File for comms_link_utils.py
"""

# Python Library Imports
import os
import sys
import pytest
from unittest import mock

# Local Library Imports
from race_python_utils import comms_link_utils


###
# Fixtures
###


# None at the Moment (Empty Test Suite)


###
# Mocked Functions
###


# None at the Moment (Empty Test Suite)


###
# Test Payload
###


def test_generate_link_properties_dict():

    properties = comms_link_utils.generate_link_properties_dict(
        "send", "unicast", False, 16, 25_700_000, supported_hints=[]
    )
    expected = {
        "type": "send",
        "unicast": True,
        "reliable": False,
        "supported_hints": [],
        "best": {"send": {"bandwidth_bps": 28270000, "latency_ms": 14}},
        "expected": {"send": {"bandwidth_bps": 25_700_000, "latency_ms": 16}},
        "worst": {"send": {"bandwidth_bps": 23130000, "latency_ms": 17}},
    }

    assert expected == properties

    properties = comms_link_utils.generate_link_properties_dict(
        "receive",
        "multicast",
        True,
        1234567890,
        987654321,
        supported_hints=["foo", "bar"],
    )
    expected = {
        "type": "receive",
        "multicast": True,
        "reliable": True,
        "supported_hints": ["foo", "bar"],
        "best": {"receive": {"bandwidth_bps": 1086419753, "latency_ms": 1111111101}},
        "expected": {"receive": {"bandwidth_bps": 987654321, "latency_ms": 1234567890}},
        "worst": {"receive": {"bandwidth_bps": 888888888, "latency_ms": 1358024679}},
    }

    assert expected == properties

    properties = comms_link_utils.generate_link_properties_dict(
        "send", "multicast", False, 1010101010, 101010101, supported_hints=["baz"]
    )
    expected = {
        "type": "send",
        "multicast": True,
        "reliable": False,
        "supported_hints": ["baz"],
        "best": {"send": {"bandwidth_bps": 111111111, "latency_ms": 909090909}},
        "expected": {"send": {"bandwidth_bps": 101010101, "latency_ms": 1010101010}},
        "worst": {"send": {"bandwidth_bps": 90909090, "latency_ms": 1111111111}},
    }

    assert expected == properties
