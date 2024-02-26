#!/usr/bin/env bash

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

# -----------------------------------------------------------------------------
# Build the SDK docker image
#
# Arguments:
# -n [value], --namespace [value], --namespace=[value]
#     Namespace of image to build
# -v [value], --version [value], --version=[value]
#     Version of image to build
# -b [value], --base-version [value], --base-version=[value]
#     Version of base race-compile image to use
# --base-namespace [value], --base-namespace=[value]
#     Namespace of base race-compile image to use
# -h, --help
#     Print help and exit
#
# Example Call:
#    bash build_sdk_image.sh {--version=1.0.0} {--tabel=testing}
# -----------------------------------------------------------------------------


###
# Helper Functions and Setup
###


set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
CALL_NAME="$0"

formatlog() {
    LOG_LEVELS=("DEBUG" "INFO" "WARNING" "ERROR")
    if [ "$1" = "ERROR" ]; then
        RED='\033[0;31m'
        NO_COLOR='\033[0m'
        printf "${RED}%s (%s): %s${NO_COLOR}\n" "$(date +%c)" "${1}" "${2}"
    elif [ "$1" = "WARNING" ]; then
        YELLOW='\033[0;33m'
        NO_COLOR='\033[0m'
        printf "${YELLOW}%s (%s): %s${NO_COLOR}\n" "$(date +%c)" "${1}" "${2}"
    elif [ ! "$(echo "${LOG_LEVELS[@]}" | grep -co "${1}")" = "1" ]; then
        echo "$(date +%c): ${1}"
    else
        echo "$(date +%c) (${1}): ${2}"
    fi
}

###
# Arguments
###

HELP=\
"Build the SDK docker image

Arguments:
    --platform-x86_64
        Build the image for the x86_64 architecture
    --platform-arm64
        Build the image for the ARM 64 architecture
    -n [value], --namespace [value], --namespace=[value]
        Namespace of image to build
    -v [value], --version [value], --version=[value]
        Version of image to build
    -h, --help
        Print help and exit

Example Call:
    bash build_sdk_image.sh --version=1.0.0
"

# Default values
PLATFORM_x86_64=
PLATFORM_arm64=
IMAGE_NAMESPACE=
IMAGE_VERSION=latest

# Parse CLI Arguments
while [ "$#" -gt 0 ]
do
    key="$1"

    case $key in
        --platform-x86_64)
        PLATFORM_x86_64=true
        shift
        ;;

        --platform-arm64)
        PLATFORM_arm64=true
        shift
        ;;

        -n|--namespace)
        IMAGE_NAMESPACE="$2"
        shift
        shift
        ;;

        --namespace=*)
        IMAGE_NAMESPACE="${1#*=}"
        shift
        ;;

        -v|--version)
        IMAGE_VERSION="$2"
        shift
        shift
        ;;

        --version=*)
        IMAGE_VERSION="${1#*=}"
        shift
        ;;

        -h|--help)
        printf "%s" "${HELP}"
        shift
        exit 1;
        ;;

        *)
        formatlog "ERROR" "unknown argument \"$1\""
        exit 1
        ;;
    esac
done


###
# Main Execution
###

function isEnvM1() {
    if uname -a | grep -qE 'Darwin.*arm64'; then
        return 0
    else
        return 1
    fi
}

# Set platform
PLATFORM=
MULTIARCH=
if [ ! -z $PLATFORM_x86_64 ]; then
    if isEnvM1 ; then
        formatlog "ERROR" "x86 builds not supported on M1"
        exit 1
    fi
    PLATFORM="linux/amd64"
fi
if [ ! -z $PLATFORM_arm64 ]; then
    if [ ! -z $PLATFORM ]; then
        PLATFORM="${PLATFORM},"
        MULTIARCH=true
    fi
    PLATFORM="${PLATFORM}linux/arm64"
fi

if [ -z $PLATFORM ]; then
    echo "No Linux platform specified. Use either --platform-x86_64 or --platform-arm64 to specify the target architecture for the image."
    exit 1
fi

if [ ! -z $MULTIARCH ] && [ -z $IMAGE_NAMESPACE ]; then
    echo "Multi-platform build must be pushed but no image namespace was specified. Use --namespace to specify the image namespace."
    exit 1
fi

if [ ! -z $IMAGE_NAMESPACE ]; then
    IMAGE_NAMESPACE="${IMAGE_NAMESPACE}/"
fi

PUSH_OPTION=
if [ ! -z $MULTIARCH ]; then
    PUSH_OPTION="--push"
fi

set -x
formatlog "INFO" "Building RACE SDK Image: ${IMAGE_NAMESPACE}race-sdk:${IMAGE_VERSION}"
eval DOCKER_BUILDKIT=1 docker buildx build \
    --platform $PLATFORM \
    --build-arg BUILDKIT_INLINE_CACHE=1 \
    --no-cache \
    -f "${DIR}/Dockerfile" \
    -t "${IMAGE_NAMESPACE}race-sdk:${IMAGE_VERSION}" \
    --load \
    $PUSH_OPTION \
    "${DIR}/../"
