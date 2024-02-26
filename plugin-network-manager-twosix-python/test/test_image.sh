
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

# _VERY_ basic test script just to check that all the imports work. If a legit test app comes along that does this and more, feel free to delete this script.

set -ex

TEST_COMMAND="mkdir -p /config/global && echo \"disabled: true\" > /config/global/jaeger-config.yml && raceserver"

docker run --rm -it race-linux-server-network-manager-26python37:latest-r1 bash -c "${TEST_COMMAND}"
