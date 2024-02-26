
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

# Include header and binaries from the target project being tested
include_directories("../../../source")
link_directories("../../../build/")

# Add the source files to the project
add_executable(unitTestRaceRegistry
    ../../../source/RaceRegistry.cpp
    ../../../source/RaceRegistryApp.cpp
    ../../../source/input/RaceTestAppInputFifo.cpp
    ../../../source/output/NodeDaemonPublisher.cpp

    RaceRegistryTest.cpp
    main.cpp
)

target_compile_definitions(unitTestRaceRegistry PUBLIC TESTBUILD)

find_package(GTest REQUIRED CONFIG)
find_package(Threads REQUIRED)

target_link_libraries(unitTestRaceRegistry
    raceSdkCommon
    raceSdkCore # only need RaceConfig.h
    raceTestAppShared
    crypto
    GTest::gmock
    GTest::gtest
    stdc++fs
    Threads::Threads
)

if (TARGET raceSdkTestMocks)
    target_link_libraries(unitTestPluginNMCpp raceSdkTestMocks)
endif()

add_dependencies(build_raceregistry_tests unitTestRaceRegistry)
add_test(raceregistry ${CMAKE_CURRENT_BINARY_DIR}/unitTestRaceRegistry)
set_tests_properties(raceregistry PROPERTIES LABELS "unit;raceregistry")
setup_coverage_for_target(
    TARGET unitTestRaceRegistry
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../..
)
setup_valgrind_for_target(unitTestRaceRegistry)
setup_clang_format_for_target(unitTestRaceRegistry PARENT raceregistry)
