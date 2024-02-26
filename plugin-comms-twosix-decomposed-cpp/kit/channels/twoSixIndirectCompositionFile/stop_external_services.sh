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
# Stop external (not running in a RACE node) services required by channel
#
# Note: For Two Six Indirect Links, need to tear down the two six whiteboard. This
# will include running a docker-compose.yml file. 
#
# TODO: Do we want to clear state?
#
# Arguments:
# -h, --help
#     Print help and exit
#
# Example Call:
#    bash stop_external_services.sh \
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
        echo "Example Call: bash stop_external_services.sh"
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


formatlog "INFO" "All Two Six Indirect Channels use the Two Six Whiteboard. Stopping one of the services starts them all"
docker-compose -p twosix-whiteboard -f "${BASE_DIR}/docker-compose.yml" stop
docker-compose -p twosix-whiteboard -f "${BASE_DIR}/docker-compose.yml" down
