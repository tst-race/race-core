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

    Will take in --range-config and --link-request arguments to generate 
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
        [--overwrite] \
        [--config-dir CONFIG_DIR] \
        [--network-manager-request NETWORK_MANAGER_REQUEST_FILE]

example call:
    ./generate_plugin_configs.py \
        --range=./2x2.json \
        --config-dir=./config \
        --overwrite
"""

# Python Library Imports
import argparse
import json
import logging
import math
import os
import sys
from typing import Any, Dict, List, Optional, Tuple

# Local Lib Imports
RACE_UTILS_PATH = f"{os.path.dirname(os.path.realpath(__file__))}/../race-python-utils"
sys.path.insert(0, RACE_UTILS_PATH)
from race_python_utils import file_utils
from race_python_utils import range_config_utils
from race_python_utils import network_manager_request_utils
from race_python_utils import network_manager_utils
from race_python_utils import comms_link_utils
from race_python_utils import twosix_whiteboard_utils


###
# Global
###


CHANNEL_ID = "twoSixBootstrapFileCpp"
try:
    with open(
        f"{os.path.dirname(os.path.realpath(__file__))}/channel_properties.json"
    ) as json_file:
        CHANNEL_PROPERTIES = json.load(json_file)
except Exception as err:
    print(f"ERROR: Failed to read channel properties: {repr(err)}")
    CHANNEL_PROPERTIES = {}

###
# Main Execution
###


def main() -> None:
    """
    Purpose:
        Generate configs for twoSixBootstrapFileCpp
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

    # Load (or create) and validate Network Manager Request
    if cli_args.network_manager_request_file:
        network_manager_request = file_utils.read_json(cli_args.network_manager_request_file)
    else:
        network_manager_request = network_manager_request_utils.generate_network_manager_request_from_range_config(
            range_config,
            [CHANNEL_PROPERTIES],
            "unicast",
            "direct",
        )
    network_manager_request_utils.validate_network_manager_request(network_manager_request, range_config)

    # Prepare the dir to store configs in, check for previous values and overwrite
    file_utils.prepare_comms_config_dir(cli_args.config_dir, cli_args.overwrite)

    # Generate Configs
    generate_configs(range_config, network_manager_request, cli_args)

    logging.info("Process To Generate RACE Plugin Config Complete")


###
# Generate Config Functions
###


def generate_configs(
    range_config: Dict[str, Any],
    network_manager_request: Dict[str, Any],
    cli_args: argparse.Namespace,
) -> None:
    """
    Purpose:
        Generate the plugin configs based on range config and network manager request
    Args:
        range_config: range config to generate against
        network_manager_request: requested links to generate against
        config_dir: where to store configs
        local_override: Ignore range config services. Use Two Six Local Information.
    Raises:
        Exception: generation fails
    Returns:
        None
    """

    (link_addresses, fulfilled_network_manager_request) = generate_genesis_link_addresses(
        range_config, network_manager_request, cli_args
    )
    file_utils.write_json(
        {CHANNEL_ID: link_addresses},
        f"{cli_args.config_dir}/genesis-link-addresses.json",
    )

    # Store the fufilled links to compare to the request (may need more iterations)
    file_utils.write_json(
        fulfilled_network_manager_request, f"{cli_args.config_dir}/fulfilled-network-manager-request.json"
    )

    user_responses = generate_user_responses(range_config)
    file_utils.write_json(user_responses, f"{cli_args.config_dir}/user-responses.json")


def generate_genesis_link_addresses(
    range_config: Dict[str, Any],
    network_manager_request: Dict[str, Any],
    cli_args: argparse.Namespace,
) -> Tuple[Dict[str, List[Dict[str, Any]]], List[Dict[str, Any]]]:
    """
    Purpose:
        Generate empty links profile. The bootstrap channel does not support genesis links.
    Args:
        range_config: range config to generate against
        network_manager_request: requested links to generate against
        local_override: Ignore range config services. Does nothing ATM (may later impact
            enclaves and connectivity between nodes. standardizing between all exemplar
            channels for now to allow easier flexibility)
    Returns:
        link_profiles: configs for the generated links
        fulfilled_network_manager_request: which network manager links in the request were fufilled
    """

    link_profiles = {}
    fulfilled_network_manager_request: Dict[str, Any] = {"links": []}

    return (link_profiles, fulfilled_network_manager_request)


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

    range_config_clients = range_config_utils.get_client_details_from_range_config(
        range_config
    )
    for client_persona in range_config_clients.keys():
        responses[client_persona] = {
            "directory": "/tmp/bootstrapping",
        }

    range_config_servers = range_config_utils.get_server_details_from_range_config(
        range_config
    )
    for server_persona in range_config_servers.keys():
        responses[server_persona] = {
            "directory": "/tmp/bootstrapping",
        }

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
            "Ignore range config service connectivity, utilized "
            "local configs (e.g. local hostname/port vs range services fields). "
            "Does nothing for Bootstrap Links at the moment"
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
    optional.add_argument(
        "--network-manager-request",
        dest="network_manager_request_file",
        help=(
            "Requested links from the network manager. Configs should generate only these"
            " links and as many of the links as possible. RiB local will not have the "
            "same network connectivty as the T&E range."
        ),
        required=False,
        default=False,
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
        format="[generate_comms_cpp_bootstrap_configs] %(asctime)s.%(msecs)03d %(levelname)s %(message)s",
        datefmt="%a, %d %b %Y %H:%M:%S",
    )

    try:
        main()
    except Exception as err:
        print(f"{os.path.basename(__file__)} failed due to error: {err}")
        raise err
