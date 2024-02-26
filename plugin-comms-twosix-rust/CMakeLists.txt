
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
project(PluginCommsTwoSixRust LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

set(BUILD_VERSION "" CACHE STRING "Version of the plugin being built")
if("${BUILD_VERSION}" STREQUAL "")
    message(SEND_ERROR "Missing or empty BUILD_VERSION argument")
endif()

include(race/clang-format)
include(race/coverage)
include(race/lint)
include(race/test-targets)
include(race/valgrind)
include(race/warnings)

function(setup_rustfmt NAME)
    add_custom_target(format_${NAME}
        COMMAND rustfmt src/**/*.rs
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Formatting ${NAME} files..."
    )
    add_local_target(format DEPENDS format_${NAME})
    add_dependencies(format_plugin_comms_twosix_rust format_${NAME})

    add_custom_target(check_format_${NAME}
        COMMAND rustfmt --check src/**/*.rs
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Checking format for ${NAME} files..."
    )
    add_local_target(check_format DEPENDS check_format_${NAME})
    add_dependencies(check_format_plugin_comms_twosix_rust check_format_${NAME})
endfunction()

setup_project_test_targets(plugin_comms_twosix_rust UNIT)
setup_project_format_targets(plugin_comms_twosix_rust)

if ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_x86_64")
    set(CARGO_FLAGS "--target=x86_64-linux-android")
    set(CARGO_ENV X86_64_LINUX_ANDROID_OPENSSL_DIR=/android/x86_64)
    set(CARGO_TARGET x86_64-linux-android/)
elseif ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_arm64-v8a")
    set(CARGO_FLAGS "--target=aarch64-linux-android")
    if (${CMAKE_HOST_SYSTEM} MATCHES "Darwin")
        set(CARGO_ENV AARCH64_LINUX_ANDROID_OPENSSL_DIR=${CMAKE_SOURCE_DIR}/m1-build/android_arm64-v8a/arm64-v8a)
    else()
        set(CARGO_ENV AARCH64_LINUX_ANDROID_OPENSSL_DIR=/android/arm64-v8a)
    endif()
    set(CARGO_TARGET aarch64-linux-android/)
elseif ("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_x86_64")
    set(CARGO_FLAGS "--target=x86_64-unknown-linux-gnu")
    set(CARGO_TARGET x86_64-unknown-linux-gnu/)
elseif ("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_arm64-v8a")
    set(CARGO_FLAGS "--target=aarch64-unknown-linux-gnu")
    set(CARGO_TARGET aarch64-unknown-linux-gnu/)
    if(CMAKE_CROSSCOMPILING)
        set(CARGO_ENV
            PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig
            PKG_CONFIG_SYSROOT_DIR=/usr/aarch64-linux-gnu
        )
    endif()
endif()

add_subdirectory(loader)
add_subdirectory(plugin)
add_subdirectory(pluginwrapper)
add_subdirectory(shims)

if(NOT ANDROID)
    add_subdirectory(test/loader-test/source EXCLUDE_FROM_ALL)
endif()

setup_project_coverage_target()
