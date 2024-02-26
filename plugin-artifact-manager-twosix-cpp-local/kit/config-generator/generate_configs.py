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

    Will take in --range-config argument to generate configs against.

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
        [--overwrite] \
        [--config-dir CONFIG_DIR]

example call:
    ./generate_plugin_configs.py \
        --range=./2x2.json \
        --config-dir=./config \
        --overwrite
"""

# Python Library Imports
import argparse
import logging
import os
import sys
from typing import Any, Dict

# Local Lib Imports
RACE_UTILS_PATH = f"{os.path.dirname(os.path.realpath(__file__))}/../race-python-utils"
sys.path.insert(0, RACE_UTILS_PATH)
from race_python_utils import file_utils
from race_python_utils import range_config_utils


###
# Global
###


###
# Main Execution
###


def main() -> None:
    """
    Purpose:
        Generate configs for Two Six Tech Exemplar ArtifactManager Plugin
    Args:
        N/A
    Returns:
        N/A
    Raises:
        Exception: Config Generation fails
    """
    logging.info("Starting Process To Generate RACE Configs")

    # Parsing Configs
    cli_args = get_cli_arguments()

    # Load and validate Range Config
    range_config = file_utils.read_json(cli_args.range_config_file)
    range_config_utils.validate_range_config(
        range_config=range_config, allow_no_clients=True
    )

    # Prepare the dir to store configs in, check for previous values and overwrite
    file_utils.prepare_comms_config_dir(cli_args.config_dir, cli_args.overwrite)

    # Generate Configs
    generate_configs(
        range_config,
        cli_args.config_dir,
        cli_args.local_override,
    )

    logging.info("Process To Generate RACE Plugin Config Complete")


###
# Generate Config Functions
###


def generate_configs(
    range_config: Dict[str, Any],
    config_dir: str,
    local_override: bool,
) -> None:
    """
    Purpose:
        Generate the plugin configs based on range config and network manager request
    Args:
        range_config: range config to generate against
        config_dir: where to store configs
        local_override: Ignore range config services. Use Two Six Local Information.
    Raises:
        Exception: generation fails
    Returns:
        None
    """

    # Store request files for historical comparison (inputs -> outputs)
    file_utils.write_json(range_config, f"{config_dir}/range-config.json")

    user_responses = generate_user_responses(range_config)
    file_utils.write_json(user_responses, f"{config_dir}/user-responses.json")


def generate_user_responses(
    range_config: Dict[str, Any],
) -> Dict[str, Dict[str, str]]:
    """
    Purpose:
        Generate the user input responses based on the range config
    Args:
        range_config: range config to generate against
    Return:
        Mapping of node persona to mapping of prompt to response
    """
    responses = {}
    return responses


###
# Helper Functions
###


def get_cli_arguments() -> argparse.Namespace:
    """
    Purpose:
        Parse CLI arguments for script
    Args:
        N/A
    Return:
        cli_arguments (ArgumentParser Obj): Parsed Arguments Object
    """
    logging.info("Getting and Parsing CLI Arguments")

    parser = argparse.ArgumentParser(description="Generate RACE Config Files")
    required = parser.add_argument_group("Required Arguments")
    optional = parser.add_argument_group("Optional Arguments")

    # Required Arguments
    required.add_argument(
        "--range",
        dest="range_config_file",
        help="Range config of the physical network",
        required=True,
        type=str,
    )

    # Optional Arguments
    optional.add_argument(
        "--overwrite",
        dest="overwrite",
        help="Overwrite configs if they exist",
        required=False,
        default=False,
        action="store_true",
    )
    optional.add_argument(
        "--local",
        dest="local_override",
        help=(
            "Ignore range config service connectivity, utilize "
            "local configs (e.g. local hostname/port vs range services fields)"
        ),
        required=False,
        default=False,
        action="store_true",
    )
    optional.add_argument(
        "--config-dir",
        dest="config_dir",
        help="Where should configs be stored",
        required=False,
        default="./configs",
        type=str,
    )

    return parser.parse_args()


###
# Entrypoint
###


if __name__ == "__main__":

    LOG_LEVEL = logging.INFO
    logging.getLogger().setLevel(LOG_LEVEL)
    logging.basicConfig(
        stream=sys.stdout,
        level=LOG_LEVEL,
        format="[generate_artifact_manager_exemplar_configs] %(asctime)s.%(msecs)03d %(levelname)s %(message)s",
        datefmt="%a, %d %b %Y %H:%M:%S",
    )

    try:
        main()
    except Exception as err:
        print(f"{os.path.basename(__file__)} failed due to error: {err}")
        raise err
