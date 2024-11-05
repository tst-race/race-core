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
# Clean previously built artifact dirs to ensure clean build
# -----------------------------------------------------------------------------


set -e


###
# Helper functions
###


# Load Helper Functions
CURRENT_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) >/dev/null 2>&1 && pwd)
. ${CURRENT_DIR}/helper_functions.sh


###
# Arguments
###


HELP=\
"Clean previously built artifact dirs to ensure clean build

Build Arguments:
    N/A

Help Arguments:
    -h, --help
        Print this message

Examples:
    ./clean_artifacts.sh
"

while [ $# -gt 0 ]
do
    key="$1"

    case $key in
        -h|--help)
        printf "%s" "${HELP}"
        shift
        exit 1;
        ;;
        *)
        echo "${CALL_NAME} unknown argument \"$1\""
        exit 1
        ;;
    esac
done

if [ ! -z "${VERBOSE}" ] ; then
    set -x
fi


###
# Main Execution
###


formatlog "INFO" "Removing previous build artifacts"
rm -rf ${CURRENT_DIR}/build/*

formatlog "INFO" "Cleaning artifacts in the kit dir"
rm -rf ${CURRENT_DIR}/kit/artifacts/*
