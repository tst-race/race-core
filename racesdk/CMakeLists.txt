
# 
# Copyright 2023 Two Six Technologies
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# You may obtain a copy of the License at
# you may not use this file except in compliance with the License.
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 

cmake_minimum_required(VERSION 3.13)
project(racesdk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

set(BUILD_VERSION "" CACHE STRING "Version of the SDK being built")
if("${BUILD_VERSION}" STREQUAL "")
    message(FATAL_ERROR "Missing or empty BUILD_VERSION argument")
endif()

if (ANDROID AND ${CMAKE_HOST_SYSTEM} MATCHES "Darwin")
    include_directories(${CMAKE_SOURCE_DIR}/m1-build/android_arm64-v8a/include/)
endif()

include(race/clang-format)
include(race/coverage)
include(race/java-format)
include(race/lint)
include(race/test-targets)
include(race/valgrind)
include(race/warnings)

setup_project_test_targets(racesdk UNIT INTEGRATION)
setup_project_format_targets(racesdk)

add_subdirectory(common)
add_subdirectory(storage-encryption)
add_subdirectory(core)
add_subdirectory(java-shims)
add_subdirectory(language-shims)
add_subdirectory(test-harness)
add_subdirectory(racetestapp-shared)

if(NOT ANDROID)
    add_subdirectory(test-mocks)
endif()

# Create this after all subdirectories have been added
setup_project_coverage_target()

# Install/export all targets
install(
    TARGETS
        raceSdkCommon
        raceSdkStorageEncryption
        raceSdkCore
        RaceJavaShims
        language-shims-headers
    EXPORT racesdk
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
install(
    EXPORT racesdk
    DESTINATION lib/cmake
)
install(
    TARGETS
        raceSdkCommon
        raceSdkStorageEncryption
        raceSdkCore
        RaceJavaShims
        language-shims-headers
        commsPluginBindingsGolang
    # LIBRARY DESTINATION lib
    # PUBLIC_HEADER DESTINATION include
    COMPONENT sdk
)

if(NOT ANDROID)
    install(
        TARGETS raceSdkTestMocks
        EXPORT racesdk-mocks
        PUBLIC_HEADER DESTINATION include/race/mocks
    )
    install(
        EXPORT racesdk-mocks
        DESTINATION lib/cmake
    )
endif()
