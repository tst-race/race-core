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

set -o errexit
set -o nounset
set -o pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

PACKAGE_DIR="${DIR}/package"
mkdir -p "${PACKAGE_DIR}"

cd "${DIR}/../"

PACKAGE_ANDROID=false
PACKAGE_ANDROID_ARM64=false
PACKAGE_LINUX=false
PACKAGE_LINUX_ARM64=false
SKIP_BUILD_ARTIFACTS=false
SKIP_FILE_ARTIFACTS=false


HELP=\
"This script prepares the RACE sdk artifacts to be packaged for use in the race-sdk
docker image.

Arguments:
    -A, --android
        Package artifacts for ANDROID_x86_64.
    -L, --linux
        Package artifacts for LINUX_x86_64.
    --android-arm64
        Package artifacts for ANDROID_arm64-v8a.
    --linux-arm64
        Package artifacts for LINUX_arm64-v8a.
    --skip-build-artifacts
        Skip the architecture specific build artifacts, i.e. only package mocks and cmake files.
    --skip-file-artifacts
        Skip the architecture-agnostic file artifacts, i.e mocks and cmake files.

Help Arguments:
    -h, --help
        Print this message

Examples:
    $0
    $0 --android
    $0 --linux --linux-arm64
    $0 --help
"


while [ $# -gt 0 ]
do
    key="$1"

    case $key in
        -A|--android)
        PACKAGE_ANDROID=true
        shift
        ;;
        --android-arm64)
        PACKAGE_ANDROID_ARM64=true
        shift
        ;;
        -L|--linux)
        PACKAGE_LINUX=true
        shift
        ;;
        --linux-arm64)
        PACKAGE_LINUX_ARM64=true
        shift
        ;;

        --skip-build-artifacts)
        SKIP_BUILD_ARTIFACTS=true
        shift
        ;;

        --skip-file-artifacts)
        SKIP_FILE_ARTIFACTS=true
        shift
        ;;

        -h|--help)
        printf "%s" "${HELP}"
        shift
        exit 1;
        ;;

        *)
        echo "unknown argument \"$1\""
        exit 1
        ;;
    esac
done



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

function isEnvM1() {
    if uname -a | grep -qE 'Darwin.*arm64'; then
        return 0
    else
        return 1
    fi
}

function isARM() {
    if uname -a | grep -qE 'aarch64'; then
        return 0
    else
        return 1
    fi
}

if [ "$SKIP_BUILD_ARTIFACTS" = true ] && [ "$SKIP_FILE_ARTIFACTS" = true ]; then
  formatlog "ERROR" "Skipping all artifacts"
  exit 1
fi

# If no architectures are specfied explicitly then package everything.
if [ "$PACKAGE_ANDROID" = false ] && [ "$PACKAGE_ANDROID_ARM64" = false ] && [ "$PACKAGE_LINUX" = false ] && [ "$PACKAGE_LINUX_ARM64" = false ]; then
  if [ "$SKIP_BUILD_ARTIFACTS" = false ]; then
    PACKAGE_ANDROID=true
    PACKAGE_ANDROID_ARM64=true
    PACKAGE_LINUX=true
    PACKAGE_LINUX_ARM64=true
  fi
elif [ "$SKIP_BUILD_ARTIFACTS" = true ]; then
  formatlog "WARNING" "skip build artifacts argument provided. Architecture build artifacts will not be packaged."
  PACKAGE_ANDROID=false
  PACKAGE_ANDROID_ARM64=false
  PACKAGE_LINUX=false
  PACKAGE_LINUX_ARM64=false
fi


if [ "$PACKAGE_ANDROID" = true ] || [ "$PACKAGE_ANDROID_ARM64" = true ] || [ "$PACKAGE_LINUX" = true ]; then
  if isARM ; then
    formatlog "ERROR" "Linux ARM only supports packaging Linux ARM artifacts"
    exit 1
  fi
fi

if [ "$PACKAGE_ANDROID" = true ] || [ "$PACKAGE_LINUX" = true ] || [ "$PACKAGE_LINUX_ARM64" = true ]; then
  if isEnvM1 ; then
    formatlog "ERROR" "M1 Mac ARM only supports packaging Android ARM artifacts"
    exit 1
  fi
fi

if $PACKAGE_ANDROID ; then
  cmake --install build/ANDROID_x86_64/racesdk --component sdk --verbose
fi

if $PACKAGE_ANDROID_ARM64 ; then
  cmake --install build/ANDROID_arm64-v8a/racesdk --component sdk --verbose
fi

if $PACKAGE_LINUX ; then
  cmake --install build/LINUX_x86_64/racesdk --component sdk --verbose
fi

if $PACKAGE_LINUX_ARM64 ; then
  cmake --install build/LINUX_arm64-v8a/racesdk --component sdk --verbose
fi

if [ "$SKIP_FILE_ARTIFACTS" = false ]; then
  cp -r "${DIR}/test-mocks" "${PACKAGE_DIR}/"
  cp -r "${DIR}/../race-cmake-modules" "${PACKAGE_DIR}/"
fi
