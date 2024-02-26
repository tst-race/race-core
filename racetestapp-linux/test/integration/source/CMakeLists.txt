
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

SET(INTEGRATION_TEST_APP_SOURCE
    main.cpp
    RaceTestAppInputFifoTest.cpp
)

SET(RACE_TEST_APP_SOURCE
    ../../../source/createPidFile.h
    ../../../source/input/RaceTestAppInputFifo.h
    ../../../source/input/RaceTestAppInputFifo.cpp
    ../../../source/input/RaceTestAppInputStdin.cpp
    ../../../source/input/RaceTestAppInputStdin.h
    ../../../source/input/IRaceTestAppInput.h
    ../../../source/output/NodeDaemonPublisher.cpp
    ../../../source/output/NodeDaemonPublisher.h
    ../../../source/createPidFile.cpp
)

add_executable(integrationTestRaceTestApp
    ${INTEGRATION_TEST_APP_SOURCE}
    ${RACE_TEST_APP_SOURCE}
)

target_include_directories(integrationTestRaceTestApp PUBLIC
    $<INSTALL_INTERFACE:include>
)

find_package(GTest REQUIRED CONFIG)
find_package(Threads REQUIRED)

target_link_libraries(integrationTestRaceTestApp
    raceSdkCommon
    raceSdkCore # only need RaceConfig.h
    raceTestAppShared
    crypto
    GTest::gmock
    GTest::gtest
    stdc++fs
    Threads::Threads
)

add_dependencies(build_racetestapp_tests integrationTestRaceTestApp)
add_test(racetestapp ${CMAKE_CURRENT_BINARY_DIR}/integrationTestRaceTestApp)
set_tests_properties(racetestapp PROPERTIES LABELS "unit;racetestapp")
setup_coverage_for_target(
    TARGET integrationTestRaceTestApp
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../..
)
setup_valgrind_for_target(integrationTestRaceTestApp)

# Setup style validation
setup_clang_format_for_target(integrationTestRaceTestApp PARENT racetestapp_linux)
