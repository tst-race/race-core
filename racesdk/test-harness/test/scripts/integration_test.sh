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

###
# Script to run basic integration test for RACE application
###

NOCOLOR='\033[0m'

docker exec -d race-server-1 bash -c 'raceserver -i &> /tmp/integration-test.out'
docker exec -d race-server-2 bash -c 'raceserver -i &> /tmp/integration-test.out'
docker exec -d race-client-2 bash -c 'raceclient -i &> /tmp/integration-test.out'

docker exec race-client-1 bash -c 'raceclient -i -r race-client-2'

if [ $? -ne 0 ]; then
  RED='\033[0;31m'
  printf "${RED}FAIL${NOCOLOR}: integration test failed!\n"
  echo "To check the logs run:"
  echo "docker exec race-client-2 bash -c 'cat /tmp/integration-test.out'"
  echo "docker exec race-server-1 bash -c 'cat /tmp/integration-test.out'"
  echo "docker exec race-server-2 bash -c 'cat /tmp/integration-test.out'"
  exit 1
else
  GREEN='\033[0;32m'
  printf "${GREEN}PASS${NOCOLOR}: integration test has passed\n"
fi
