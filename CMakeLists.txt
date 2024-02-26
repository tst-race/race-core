
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
project(finishline)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/race-cmake-modules)

# Set warning flags for the compiler
include(race/warnings)

set(GLOBAL_BUILD true)
enable_testing()

################################################################################
# Repos
################################################################################

# Check that Android dependencies have been setup for M1 Mac builds.
if (ANDROID AND ${CMAKE_HOST_SYSTEM} MATCHES "Darwin")
    if (NOT EXISTS ${CMAKE_SOURCE_DIR}/m1-build/android_arm64-v8a/arm64-v8a/)
        message(FATAL_ERROR "please call setup script: ./m1-build/android_arm64-v8a/setup.sh")
    endif()
endif()

# Node daemon repo
if (ANDROID)
    set(BUILD_ANDROID_DAEMON ON CACHE BOOL "")
else()
    set(BUILD_LINUX_DAEMON ON CACHE BOOL "")
endif()
add_subdirectory(race-node-daemon)

# SDK repo
add_subdirectory(racesdk)

# Plugin Repos
add_subdirectory(plugin-network-manager-twosix-cpp)
add_subdirectory(plugin-network-manager-twosix-python)

add_subdirectory(plugin-comms-twosix-cpp)
add_subdirectory(plugin-comms-twosix-decomposed-cpp)
add_subdirectory(plugin-comms-twosix-golang)
add_subdirectory(plugin-comms-twosix-java)
add_subdirectory(plugin-comms-twosix-python)
add_subdirectory(plugin-comms-twosix-rust)

add_subdirectory(plugin-artifact-manager-twosix-cpp)
add_subdirectory(plugin-artifact-manager-twosix-cpp-local)

# App Repos
if(ANDROID)
    add_subdirectory(raceclient-android)
else()
    add_subdirectory(racetestapp-linux)
    add_subdirectory(race-registry)
endif()


################################################################################
# Tests
################################################################################

if(NOT ANDROID)
    include(race/test-targets)
    setup_project_test_targets(finishline)
endif()
