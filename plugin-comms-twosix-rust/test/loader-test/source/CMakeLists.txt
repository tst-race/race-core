
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

###############################################################################
# Copy Items to Test
###############################################################################

add_custom_target(CopyForPluginCommsTwoSixRustTest ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../config ${CMAKE_CURRENT_BINARY_DIR}/config
    COMMENT "Running PluginCommsTwoSixRust unit tests..."
)

################################################################################
# plugin loader unit test
################################################################################

add_executable(unitTestPluginCommsTwoSixRust
    main.cpp
)

find_package(GTest REQUIRED CONFIG)
find_package(Threads REQUIRED)

target_link_libraries(unitTestPluginCommsTwoSixRust
    PluginCommsTwoSixRust
    GTest::gmock
    GTest::gtest
    Threads::Threads
    dl
)

if (TARGET raceSdkTestMocks)
    target_link_libraries(unitTestPluginCommsTwoSixRust raceSdkTestMocks)
endif()

add_dependencies(unitTestPluginCommsTwoSixRust CopyForPluginCommsTwoSixRustTest)

add_dependencies(build_plugin_comms_twosix_rust_tests unitTestPluginCommsTwoSixRust)
add_test(plugin_comms_twosix_rust ${CMAKE_CURRENT_BINARY_DIR}/unitTestPluginCommsTwoSixRust)
set_tests_properties(plugin_comms_twosix_rust PROPERTIES LABELS "unit;plugin_comms_twosix_rust")
setup_coverage_for_target(
    TARGET unitTestPluginCommsTwoSixRust
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../..
)
setup_valgrind_for_target(unitTestPluginCommsTwoSixRust)
setup_clang_format_for_target(unitTestPluginCommsTwoSixRust PARENT plugin_comms_twosix_rust)
