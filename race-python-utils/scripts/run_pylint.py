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
        Run Pylint, Fail if Score under threshold
"""

# Python Library Imports
import argparse
import logging
import os
import sys
from pylint.lint import Run


def main() -> None:
    """
    Purpose:
        Run Pylint
    Args:
        N/A
    Returns:
        N/A
    Raises:
        Exception: Pylint fails
    """

    cli_arguments = get_cli_arguments()

    results = Run(
        [f"--rcfile={cli_arguments.rcfile}", "race_python_utils"], do_exit=False
    )

    try:
        if results.linter.stats["global_note"] > cli_arguments.min_score:
            # Success, exit 0
            sys.exit(0)
        else:
            # Failure, exit 1
            sys.exit(1)

    except Exception as err:
        logging.exception(f"Error Parsing Pylint Results: {err}")
        raise


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

    parser = argparse.ArgumentParser(description="Clear Whiteboard post")
    required = parser.add_argument_group("Required Arguments")
    optional = parser.add_argument_group("Optional Arguments")

    # Required Arguments
    required.add_argument(
        "--min-score",
        dest="min_score",
        help="Minimum Pylint Score",
        required=True,
        type=float,
    )

    # Optional Arguments
    optional.add_argument(
        "--rcfile",
        dest="rcfile",
        help="RC File for Pylint",
        required=False,
        type=str,
        default=f"{os.getcwd()}/.pylintrc",
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
        format="[run_pylint] %(asctime)s.%(msecs)03d %(levelname)s %(message)s",
        datefmt="%a, %d %b %Y %H:%M:%S",
    )

    try:
        main()
    except Exception as err:
        print(f"{os.path.basename(__file__)} failed due to error: {err}")
        raise err
