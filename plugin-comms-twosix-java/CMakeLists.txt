
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

cmake_minimum_required(VERSION 3.20)
project(PluginCommsTwoSixJava LANGUAGES CXX)

set(BUILD_VERSION "" CACHE STRING "Version of the plugin being built")
if("${BUILD_VERSION}" STREQUAL "")
    message(SEND_ERROR "Missing or empty BUILD_VERSION argument")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

include(race/clang-format)
include(race/coverage)
include(race/java-format)
include(race/lint)
include(race/test-targets)
include(race/valgrind)
include(race/warnings)

setup_project_test_targets(plugin_comms_twosix_java UNIT)
setup_project_format_targets(plugin_comms_twosix_java)

add_subdirectory(loader/source)
add_subdirectory(plugin)

if(NOT ANDROID)
    enable_testing()
    add_subdirectory(test/loader-test/source EXCLUDE_FROM_ALL)
endif()

setup_project_coverage_target()