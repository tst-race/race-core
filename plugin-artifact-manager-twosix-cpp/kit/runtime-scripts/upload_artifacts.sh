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
# Upload distribution artifacts to the file server
# -----------------------------------------------------------------------------

set -e


###
# Helper functions
###


# Load Helper Functions
BASE_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) >/dev/null 2>&1 && pwd)
. ${BASE_DIR}/helper_functions.sh


###
# Arguments
###


ARTIFACTS_DIR=""
BASE_URL="http://twosix-file-server:8080"

HELP=\
"Upload distribution artifacts to the file server.

Arguments:
--artifacts-dir=DIR
    Directory containing artifacts to be uploaded
--config-dir=DIR
    Directory containing configs created during config generation
--base-url=URL
    Base URL of the file server
-h, --help
    Print help and exit

Example call:
    bash upload_artifacts.sh --artifacts-dir=/path/to/artifacts
"


# Parse CLI Arguments
while [ $# -gt 0 ]
do
    key="$1"

    case $key in
        --artifacts-dir)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "Missing artifacts directory" >&2
            exit 1
        fi
        ARTIFACTS_DIR="$2"
        shift
        shift
        ;;
        --artifacts-dir=*)
        ARTIFACTS_DIR="${1#*=}"
        shift
        ;;

        --config-dir)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "Missing configs directory" >&2
            exit 1
        fi
        # We don't care about this argument
        # CONFIG_DIR="$2"
        shift
        shift
        ;;
        --config-dir=*)
        # We don't care about this argument
        # CONFIG_DIR="${1#*=}"
        shift
        ;;
    
        --base-url)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "Missing base URL" >&2
            exit 1
        fi
        BASE_URL="$2"
        shift
        shift
        ;;
        --base-dir=*)
        BASE_URL="${1#*=}"
        shift
        ;;

        -h|--help)
        printf "%s" "${HELP}"
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

for ARTIFACT in "$ARTIFACTS_DIR"/*
do
    FILENAME=$(basename $ARTIFACT)
    URL=$BASE_URL/upload
    formatlog "INFO" "Uploading artifact ${FILENAME} from path ${ARTIFACT}"
    curl -X POST -F "file=@${ARTIFACT}" $URL
done
