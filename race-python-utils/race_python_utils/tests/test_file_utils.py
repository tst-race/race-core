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
        Test File for file_utils.py
"""

# Python Library Imports
import os
import pathlib
import pytest
import sys
from unittest import mock

# Local Library Imports
from race_python_utils import file_utils


###
# Fixtures / Mocks
###


def _get_test_file_path(file_name: str) -> str:
    """
    Test File Path
    """

    return f"{os.path.join(os.path.dirname(__file__))}/files/{file_name}"


###
# Tests
###

###
# load_file_into_memory
###


def test_load_file_into_memory_reads_json() -> int:
    """
    TODO
    """

    pass

    # bytes_file = _get_test_file_path("test.json")
    # data = file_utils.load_file_into_memory(bytes_file, "json")

    # assert type(data) is dict
    # assert len(data.keys()) == 1
    # assert len(data[list(data.keys())[0]].keys()) == 2
