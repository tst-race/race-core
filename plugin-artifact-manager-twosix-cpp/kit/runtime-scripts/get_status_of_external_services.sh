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
# Get status of external (not running in a RACE node) services required by plugin.
# Intent is to ensure that the channel will be functional if a RACE deployment
# were to be started and connections established
#
# Note: For Two Six Exemplar, need to check the status of the two six file server
#
# Arguments:
# -h, --help
#     Print help and exit
#
# Example Call:
#    bash get_status_of_external_services.sh \
#        {--help}
# -----------------------------------------------------------------------------


###
# Helper functions
###


# Load Helper Functions
BASE_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) >/dev/null 2>&1 && pwd)
. ${BASE_DIR}/helper_functions.sh


###
# Arguments
###


# Parse CLI Arguments
while [ $# -gt 0 ]
do
    key="$1"

    case $key in
        -h|--help)
        echo "Example Call: bash get_status_of_external_services.sh"
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


formatlog "INFO" "Printing compose status."
docker-compose -p twosix-file-server -f "${BASE_DIR}/docker-compose.yml" ps

formatlog "INFO" "Getting Expected Container Statuses. twosix-file-server needs to be running"
# TODO, once we have health checks, we need to add '--filter "health=healthy"' to the status commands
FILE_SERVER_RUNNING=$(docker ps -q --filter "name=twosix-file-server" --filter "status=running")
if [ -z "${FILE_SERVER_RUNNING}" ]; then
    echo "File server is not running, Exit 1"
    exit 1
else 
    echo "File server is running, Exit 0"
    exit 0
fi

