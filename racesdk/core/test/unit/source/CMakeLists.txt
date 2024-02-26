
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
add_executable(unitTestRaceSdkCore
    ArtifactManagerTest.cpp
    BootstrapInstanceManagerTest.cpp
    BootstrapManagerTest.cpp
    ComponentManagerTest.cpp
    ComponentActionManagerTest.cpp
    ComponentLifetimeManagerTest.cpp
    ComponentPackageManagerTest.cpp
    ComponentReceivePackageManagerTest.cpp
    HandlerTest.cpp
    helper_test.cpp
    main.cpp
    PluginDefTest.cpp
    RaceChannelsTest.cpp
    RaceConfigTest.cpp
    RaceLinksTest.cpp
    RaceSdkTest.cpp
    NMWrapperTest.cpp
    CommsWrapperTest.cpp
    TestHarnessWrapperTest.cpp
    AppWrapperTest.cpp
)

file(GLOB UNIT_TEST_RACE_SDK_CORE_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/*.h
)
set_target_properties(unitTestRaceSdkCore PROPERTIES
    PUBLIC_HEADER "${UNIT_TEST_RACE_SDK_CORE_PUBLIC_HEADERS}"
)
file(GLOB UNIT_TEST_RACE_SDK_CORE_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
set_target_properties(unitTestRaceSdkCore PROPERTIES
    PRIVATE_HEADER "${UNIT_TEST_RACE_SDK_CORE_PRIVATE_HEADERS}"
)
target_compile_definitions(unitTestRaceSdkCore 
    PUBLIC TEST_DEFAULT_CONFIGS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../../../test-default-configs/"
    PUBLIC EXPECT_LOG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../output"
    PUBLIC TEST_INPUT_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../input-files/"
)

# Link external dependencies
find_package(GTest REQUIRED CONFIG)
find_package(Threads REQUIRED)

target_link_libraries(unitTestRaceSdkCore
    GTest::gmock
    GTest::gtest
    Threads::Threads
    raceSdkCore
    PluginNMTwoSixTestHarness
)

add_dependencies(build_racesdk_tests unitTestRaceSdkCore)
add_test(core-unit ${CMAKE_CURRENT_BINARY_DIR}/unitTestRaceSdkCore)
set_tests_properties(core-unit PROPERTIES LABELS "unit;racesdk")
setup_coverage_for_target(
    TARGET unitTestRaceSdkCore
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../..
)
setup_valgrind_for_target(unitTestRaceSdkCore)

# Setup style validation
setup_clang_format_for_target(unitTestRaceSdkCore PARENT racesdk)
