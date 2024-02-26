
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

# Add the source to the test executable
add_executable(unitTestPluginNMTestHarness
    helpersTest.cpp
    main.cpp
    MessageSerializerTest.cpp
    MessageSerializerHelpersTest.cpp
    PluginNMTestHarnessTest.cpp
)

# Link external dependencies
find_package(GTest REQUIRED CONFIG)
find_package(Threads REQUIRED)

target_link_libraries(unitTestPluginNMTestHarness
    GTest::gmock
    GTest::gtest
    Threads::Threads
    raceSdkCommon
    PluginNMTwoSixTestHarness
    raceSdkTestMocks
)

add_custom_command(
    COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/../files ${CMAKE_CURRENT_BINARY_DIR}/../
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../files
    COMMENT "Copying test harness unit test files"
)

add_custom_target(copyTestHarnessTestFiles DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../files)
add_dependencies(unitTestPluginNMTestHarness copyTestHarnessTestFiles)

add_dependencies(build_racesdk_tests unitTestPluginNMTestHarness)
add_test(test-harness ${CMAKE_CURRENT_BINARY_DIR}/unitTestPluginNMTestHarness)
set_tests_properties(test-harness PROPERTIES LABELS "unit;racesdk")
setup_coverage_for_target(
    TARGET unitTestPluginNMTestHarness
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../..
)
setup_valgrind_for_target(unitTestPluginNMTestHarness)

# Setup style validation
setup_clang_format_for_target(unitTestPluginNMTestHarness PARENT racesdk)
