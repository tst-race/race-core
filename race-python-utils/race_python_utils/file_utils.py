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
        Utilities for dealing with files and i/o
"""

# Python Library Imports
import logging
import json
import os
import shutil
from typing import Any, Dict, List, Optional, Union


###
# Prepare Config Dir Functions
###


def prepare_network_manager_config_dir(
    config_dir: str, overwrite: bool, race_personas: List
) -> None:
    """
    Purpose:
        Prepare the network manager config dir
    Args:
        config_dir: Parent directory where all config files and subdirectories are
            stored.
        overwrite: Should we overwrite the working dir if it exists?
        race_personas: Personas to create node specific config dirs for
    Return:
        N/A
    """

    # Check for existing dir and overwrite
    if os.path.isdir(config_dir):
        if overwrite:
            logging.info(f"{config_dir} exists and overwrite set, removing")
            shutil.rmtree(config_dir)
        else:
            raise Exception(f"{config_dir} exists and overwrite not set, exiting")

    # Make base dir, files located at this directory will not be copied onto nodes
    os.makedirs(config_dir)
    # Make Shared Dirs (Place any configs meant to be shared to all genesis RACE nodes here)
    os.makedirs(f"{config_dir}/shared/", exist_ok=True)
    os.makedirs(f"{config_dir}/shared/personas/", exist_ok=True)
    # Make Node Specific Dirs
    for persona_obj in race_personas:
        os.makedirs(f"{config_dir}/{persona_obj['raceUuid']}/", exist_ok=True)


def prepare_comms_config_dir(config_dir: str, overwrite: bool) -> None:
    """
    Purpose:
        Prepare the base config dir.
    Args:
        config_dir: Parent directory where all config files and subdirectories are
            stored.
        overwrite: Should we overwrite the config dir if it exists?
    Return:
        N/A
    """

    # Check for existing dir and overwrite
    if os.path.isdir(config_dir):
        if overwrite:
            logging.info(f"{config_dir} exists and overwrite set, removing")
            shutil.rmtree(config_dir)
        else:
            raise Exception(f"{config_dir} exists and overwrite not set, exiting")

    # Make dirs
    os.makedirs(config_dir, exist_ok=True)


###
# Read Functions
###


def read_json(
    json_filename: str,
) -> Any:  # should be Union[Dict[str, Any], List[Any]] but mypy makes that painful
    """
    Purpose:
        Load the range config file into memory as python Dict
    Args:
        json_filename: JSON filename to read
    Raises:
        Exception: if json is invalid
        Exception: if json is not found
    Returns:
        loaded_json: Loaded JSON
    """

    try:
        with open(json_filename, "r") as json_file_onj:
            return json.load(json_file_onj)
    except Exception as load_err:
        logging.error(f"Failed loading {json_filename}")
        raise load_err


###
# Write Functions
###


def write_json(json_object: Union[Dict[str, Any], List[Any]], json_file: str) -> None:
    """
    Purpose:
        Load Dictionary into JSON File
    Args:
        json_object: Dictionary to be stored in .json format
        json_file: Filename for JSON file to store (including path)
    Returns:
        N/A
    Examples:
        >>> json_file = 'some/path/to/file.json'
        >>> json_object = {
        >>>     'key': 'value'
        >>> }
        >>> write_json_into_file(json_file, json_object)
    """
    logging.info(f"Writing JSON File Into Memory: {json_file}")

    with open(json_file, "w") as file:
        json.dump(json_object, file, sort_keys=True, indent=4, separators=(",", ": "))


def write_bytes(bytes_object: Any, bytes_file: str) -> None:
    """
    Purpose:
        Load Bytes into File
    Args:
        bytes_object: bytes to be stored to the file
        bytes_file: Filename for file to store (including path)
    Returns:
        N/A
    """
    logging.info(f"Writing Bytes File Into Memory: {bytes_file}")

    with open(bytes_file, "wb") as file_obj:
        file_obj.write(bytes_object)
