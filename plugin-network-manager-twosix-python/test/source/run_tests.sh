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

#
# Run all tests for the plugin
#
# Example: sh run_tests.sh
#

PYTHONPATH=${PYTHONPATH}:/usr/local/lib/:/usr/local/lib/race/python/
LD_LIBRARY_PATH=/usr/lib/:/lib/:/lib64//usr/local/lib/:/usr/local/lib/race/python/
pytest \
    TestRaceCrypto.py \
    TestPluginNMTwoSixClientPy.py \
    TestPluginNMTwoSixServerPy.py
