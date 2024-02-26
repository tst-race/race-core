
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

add_executable(unitTestRaceTestAppShared
    helpers.createRandomString.test.cpp
    helpers.makeClrMsg.test.cpp
    helpers.getEnvironmentVariable.test.cpp
    helpers.getPersona.test.cpp
    helpers.getTimeInMicroseconds.test.cpp
    helpers.strip.test.cpp
    helpers.tokenizeString.test.cpp
    main.cpp
    MessageTest.cpp
    OutputMessageTest.cpp
    RaceAppTest.cpp
    RaceTestAppTest.cpp
    UserInputResponseCacheTest.cpp
    UserInputResponseParserTest.cpp
)

find_package(GTest REQUIRED CONFIG)
find_package(Threads REQUIRED)

target_link_libraries(unitTestRaceTestAppShared
    raceSdkCommon
    raceSdkStorageEncryption
    raceSdkCore # only need RaceConfig.h
    raceTestAppShared
    crypto
    GTest::gmock
    GTest::gtest
    stdc++fs
    Threads::Threads
)

add_dependencies(build_racesdk_tests unitTestRaceTestAppShared)
add_test(racetestapp_shared ${CMAKE_CURRENT_BINARY_DIR}/unitTestRaceTestAppShared)
set_tests_properties(racetestapp_shared PROPERTIES LABELS "unit;racesdk")
setup_coverage_for_target(
    TARGET unitTestRaceTestAppShared
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../..
)
setup_valgrind_for_target(unitTestRaceTestAppShared)

# Setup style validation
setup_clang_format_for_target(unitTestRaceTestAppShared PARENT racesdk)