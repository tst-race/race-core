#!/usr/bin/env bash

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

# -----------------------------------------------------------------------------
# Automated script for building the docker images from dockerfiles
#
# Arguments:
# -v [value], --version [value], --version=[value]
#     Specify the version to build
# --local
#     Flag to use a local base image (race-linux-client/race-linux-server)
# -----------------------------------------------------------------------------

set -e

# Version values
WHITEBOARD_VERSION=$(cat VERSION)

# No-Cache Flag
NO_CACHE=""

# Parse CLI Arguments
while [ $# -gt 0 ]
do
    case "$1" in
        -v|--version)
        if [ $# -lt 2 ]; then
            echo "$(date +%c) - ERROR: missing verion" >&2
            exit 1
        fi
        WHITEBOARD_VERSION="$2"
        shift
        shift
        ;;
        --version=*)
        WHITEBOARD_VERSION="${1#*=}"
        shift
        ;;
        --no-cache)
        NO_CACHE="--no-cache"
        shift
        ;;
        *)
        echo "$(date +%c) - ERROR: unknown argument \"$1\""
        exit 1
        ;;
    esac
done

echo "$(date +%c) - Building Whiteboard version $WHITEBOARD_VERSION"

# Build the client image
CONTAINER_NAME="twosix-whiteboard"
echo "$(date +%c) Building Client Image: $CONTAINER_NAME:$WHITEBOARD_VERSION (no-cache = $NO_CACHE)"
docker build $NO_CACHE \
    --file "Dockerfile" \
    --tag "$CONTAINER_NAME:$WHITEBOARD_VERSION" \
    src/