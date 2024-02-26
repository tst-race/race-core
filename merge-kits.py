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

import argparse
import os
import subprocess
import sys
import tempfile


THIS_DIR = os.path.dirname(os.path.abspath(__file__))


def get_cli_arguments():
    parser = argparse.ArgumentParser(
        description="Merge platform-specific artifact kits for apps and plugins",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "in_dir",
        help="Directory containing platform-specific artifact kits to be merged",
        type=str,
    )
    parser.add_argument(
        "--out-dir",
        default=os.path.join(THIS_DIR, "kits"),
        help="Output directory for all merged artifact kits",
        type=str,
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Enable verbose output",
    )
    return parser.parse_intermixed_args()


def is_kit(file_path: str) -> bool:
    return file_path.endswith(".tar.gz")


def extract_kit(in_file_path: str, out_dir: str, verbose: bool) -> bool:
    kit_file_name = os.path.basename(file_path)
    kit_name = kit_file_name.split(".")[0]
    kit_out_dir = os.path.join(out_dir, kit_name)

    os.makedirs(kit_out_dir, exist_ok=True)

    print(f"Extracting kit for {kit_name}")
    cmd = ["tar", "--extract", f"--directory={kit_out_dir}", f"--file={in_file_path}"]
    if verbose:
        print(f"Executing: {' '.join(cmd)}")
    subprocess.run(cmd, check=True)

    return True


def create_kit(kit: str, in_dir: str, out_dir: str, verbose: bool) -> bool:
    print(f"Creating kit for {kit}")

    kit_dir = os.path.join(in_dir, kit)
    kit_tar = os.path.join(out_dir, f"{kit}.tar.gz")
    cmd = ["tar", "--create", "--gzip", f"--directory={kit_dir}", f"--file={kit_tar}", "."]
    if verbose:
        print(f"Executing: {' '.join(cmd)}")
    subprocess.run(cmd, check=True)

    return True


if __name__ == "__main__":
    args = get_cli_arguments()

    if not os.path.exists(args.out_dir):
        os.makedirs(args.out_dir, exist_ok=True)

    any_created = False
    with tempfile.TemporaryDirectory() as tmp_dir:
        for root, dirs, files in os.walk(args.in_dir):
            for file in files:
                file_path = os.path.join(root, file)
                if is_kit(file_path):
                    extract_kit(file_path, tmp_dir, args.verbose)

        for kit in os.listdir(tmp_dir):
            any_created |= create_kit(kit, tmp_dir, args.out_dir, args.verbose)

    if not any_created:
        print(f"*** No kits were created")
        sys.exit(1)
