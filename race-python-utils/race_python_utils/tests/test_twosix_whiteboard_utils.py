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
        Test File for twosix_whiteboard_utils.py
"""

# Python Library Imports
import os
import sys
import pytest
from unittest import mock

# Local Library Imports
from race_python_utils import twosix_whiteboard_utils


###
# Fixtures / Mocks
###

# N/A


###
# Test Payload
###


def test_generate_local_two_six_whiteboard_details():
    generated_local_details = (
        twosix_whiteboard_utils.generate_local_two_six_whiteboard_details()
    )

    expected = {
        "authenticated_service": False,
        "hostname": "twosix-whiteboard",
        "name": "twosix-whiteboard",
        "port": 5000,
        "protocol": "http",
        "url": "twosix-whiteboard:5000",
    }

    assert expected == generated_local_details
