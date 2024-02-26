
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
add_executable(integrationTestRaceSdkCore
    main.cpp
    PythonLoaderWrapperTest.cpp
    RaceSdk_cleanShutdown.cpp
    RaceSdk_receiveEncPkg.cpp
)

file(GLOB INTEGRATION_TEST_RACE_SDK_CORE_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/*.h
)
set_target_properties(integrationTestRaceSdkCore PROPERTIES
    PUBLIC_HEADER "${INTEGRATION_TEST_RACE_SDK_CORE_PUBLIC_HEADERS}"
)
file(GLOB INTEGRATION_TEST_RACE_SDK_CORE_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
set_target_properties(integrationTestRaceSdkCore PROPERTIES
    PRIVATE_HEADER "${INTEGRATION_TEST_RACE_SDK_CORE_PRIVATE_HEADERS}"
)

# Link external dependencies
find_package(GTest REQUIRED CONFIG)
find_package(Threads REQUIRED)

target_link_libraries(integrationTestRaceSdkCore
    GTest::gmock
    GTest::gtest
    Threads::Threads
    raceSdkCore
)

add_dependencies(build_racesdk_tests integrationTestRaceSdkCore)
add_test(core-integration ${CMAKE_CURRENT_BINARY_DIR}/integrationTestRaceSdkCore)
set_tests_properties(core-integration PROPERTIES LABELS "integration;racesdk")
setup_coverage_for_target(
    TARGET integrationTestRaceSdkCore
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../..
)
setup_valgrind_for_target(integrationTestRaceSdkCore)

# Setup style validation
setup_clang_format_for_target(integrationTestRaceSdkCore PARENT racesdk)
