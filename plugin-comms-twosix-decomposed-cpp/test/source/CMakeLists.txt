
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

# Setup Packages
find_package(CURL)
include_directories(${CURL_INCLUDE_DIR})

# Set warning flags for the compiler
# include(../../source/warnings.cmake.txt)

add_executable(unitTestPluginCommsDecomposedCpp
    ../../source/common/log.cpp
    ../../source/encoding/PluginCommsTwoSixStubEncoding.cpp
    ../../source/transport/Link.cpp
    ../../source/transport/LinkAddress.cpp
    ../../source/transport/LinkMap.cpp
    ../../source/transport/MessageHashQueue.cpp
    ../../source/transport/PluginCommsTwoSixStubTransport.cpp
    ../../source/user-model/LinkUserModel.cpp
    ../../source/user-model/MarkovModel.cpp
    ../../source/user-model/PluginCommsTwoSixStubUserModel.cpp

    main.cpp
    transport/TestLink.cpp
    transport/TestLinkMap.cpp
    transport/TestPluginCommsTwoSixStubTransport.cpp
    user-model/TestLinkUserModel.cpp
    user-model/TestMarkovModel.cpp
    user-model/TestPluginCommsTwoSixStubUserModel.cpp
)

target_compile_definitions(unitTestPluginCommsDecomposedCpp PUBLIC TESTBUILD JSON_DIAGNOSTICS=1)

target_include_directories(unitTestPluginCommsDecomposedCpp PRIVATE
    common/
    ../../source/common/
    ../../source/encoding/
    ../../source/transport/
    ../../source/user-model/
)

find_package(GTest REQUIRED CONFIG)
find_package(Threads REQUIRED)

find_library(LIB_CPPREST cpprest)
find_library(LIB_CRYPTO crypto)
find_library(LIB_SSL ssl)

target_link_libraries(unitTestPluginCommsDecomposedCpp
    GTest::gmock
    GTest::gtest
    raceSdkCommon
    Threads::Threads
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES}
    stdc++fs
    ${LIB_CPPREST}
    ${LIB_CRYPTO}
    ${LIB_SSL}
)

if (TARGET raceSdkTestMocks)
    target_link_libraries(unitTestPluginCommsDecomposedCpp raceSdkTestMocks)
endif()

add_dependencies(build_plugin_comms_twosix_decomposed_cpp_tests unitTestPluginCommsDecomposedCpp)
add_test(plugin_comms_twosix_cpp_decomposed ${CMAKE_CURRENT_BINARY_DIR}/unitTestPluginCommsDecomposedCpp)
set_tests_properties(plugin_comms_twosix_cpp_decomposed PROPERTIES LABELS "unit;plugin_comms_twosix_decomposed_cpp")
setup_coverage_for_target(
    TARGET unitTestPluginCommsDecomposedCpp
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../..
)
setup_valgrind_for_target(unitTestPluginCommsDecomposedCpp)
setup_clang_format_for_target(unitTestPluginCommsDecomposedCpp PARENT plugin_comms_twosix_decomposed_cpp)
