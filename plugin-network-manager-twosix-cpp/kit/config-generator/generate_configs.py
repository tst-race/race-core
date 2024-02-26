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
    Generate the plugin configs based on a provided range config and save 
    config files to the specified config directory.

    Will take in --range-config arguments to generate 
    configs against.

    Note: if config is not empty, --override will need
    to be run to remove old configs

Steps:
    - Parse CLI args
    - Check for existing configs
        - remove if --override is set and configs exist
        - fail if --override is not set and configs exist
    - Load and Parse Range Config File
    - Generate configs for the plugin
    - Store configs in the specified config directory

usage:
    generate_configs.py \
        [-h] \
        --range RANGE_CONFIG_FILE \
        --channel-props CHANNEL_PROPS_FILE \
        --config-dir CONFIG_DIR \
        [--overwrite]

example call:
    ./generate_plugin_configs.py \
        --range=./2x2.json \
        --channel-props=./channel_props.json \
        --config-dir=./config \
        --overwrite
"""

# Python Library Imports
import logging
import os
import sys

# Local Lib Imports
RACE_UTILS_PATH = f"{os.path.dirname(os.path.realpath(__file__))}/../race-python-utils"
sys.path.insert(0, RACE_UTILS_PATH)
from race_python_utils.network_manager import generate_configs


###
# Main Execution
###


def main() -> None:
    """
    Purpose:
        Generate configs for Network Manager Two Six Labs C++ Plugin
    Args:
        N/A
    Returns:
        N/A
    Raises:
        Exception: Config Generation fails
    """
    generate_configs.run(enable_multicast=True)


###
# Entrypoint
###


if __name__ == "__main__":

    LOG_LEVEL = logging.INFO
    logging.getLogger().setLevel(LOG_LEVEL)
    logging.basicConfig(
        stream=sys.stdout,
        level=LOG_LEVEL,
        format="[generate_network_manager_cpp_configs] %(asctime)s.%(msecs)03d %(levelname)s %(message)s",
        datefmt="%a, %d %b %Y %H:%M:%S",
    )

    try:
        main()
    except Exception as err:
        print(f"{os.path.basename(__file__)} failed due to error: {err}")
        raise err
