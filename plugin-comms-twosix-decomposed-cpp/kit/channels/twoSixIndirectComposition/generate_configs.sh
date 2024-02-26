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
# Generate configs for the plugin
#
# Note: For Two Six Direct Links, we call the generate_configs.py script. This
# file is a wrapped to have a standardized bash interface
#
# Example Call:
#    bash generate_configs.sh {arguments}
# -----------------------------------------------------------------------------


set -e


###
# Arguments
###


# Get Path
BASE_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) >/dev/null 2>&1 && pwd)


###
# Main Execution
###


python3 ${BASE_DIR}/generate_configs.py "$@"
