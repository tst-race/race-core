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


THIS_DIR = os.path.dirname(os.path.abspath(__file__))
ALL_KITS = [
    "race-node-daemon",
    "racetestapp-linux",
    "race-registry",
    "raceclient-android",
    "plugin-network-manager-twosix-cpp",
    "plugin-network-manager-twosix-python",
    "plugin-comms-twosix-cpp",
    "plugin-comms-twosix-decomposed-cpp",
    "plugin-comms-twosix-golang",
    "plugin-comms-twosix-java",
    "plugin-comms-twosix-python",
    "plugin-comms-twosix-rust",
    "plugin-artifact-manager-twosix-cpp",
    "plugin-artifact-manager-twosix-cpp-local",
]
PLATFORMS = [
    "android-arm64-v8a",
    "android-x86_64",
    "linux-arm64-v8a",
    "linux-x86_64",
]
NODE_TYPES = ["client", "server"]


def get_cli_arguments():
    parser = argparse.ArgumentParser(
        description="Create artifact kits for apps and plugins",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "kits",
        help="Specific artifact kits to create (defaults to all)",
        nargs="*",
        type=str,
    )
    parser.add_argument(
        "--out-dir",
        default=os.path.join(THIS_DIR, "kits"),
        help="Output directory for all created artifact kits",
        type=str,
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Enable verbose output",
    )
    return parser.parse_intermixed_args()


def create_kit(kit: str, out_dir: str, verbose: bool) -> bool:
    kit_dir = os.path.join(THIS_DIR, kit, "kit")

    found_artifacts = False
    for platform in PLATFORMS:
        for node_type in NODE_TYPES:
            kit_platform_dir = os.path.join(kit_dir, "artifacts", f"{platform}-{node_type}")
            if os.path.exists(kit_platform_dir) and os.listdir(kit_platform_dir):
                found_artifacts = True
                break

    if not found_artifacts:
        print(f"*** No artifacts found for {kit}, it must be built before a kit can be created")
        return False

    print(f"Creating kit for {kit}")

    kit_tar = os.path.join(out_dir, f"{kit}.tar.gz")
    cmd = ["tar", "--create", "--gzip", "--dereference", f"--directory={kit_dir}", f"--file={kit_tar}", "."]
    if verbose:
        print(f"Executing: {' '.join(cmd)}")
    subprocess.run(cmd, check=True)

    return True


if __name__ == "__main__":
    args = get_cli_arguments()

    if not os.path.exists(args.out_dir):
        os.makedirs(args.out_dir, exist_ok=True)

    any_created = False
    for kit in ALL_KITS:
        if not args.kits or kit in args.kits:
            any_created |= create_kit(kit, args.out_dir, args.verbose)

    if not any_created:
        print(f"*** No kits were created")
        sys.exit(1)
