
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

add_executable(unitTestPluginCommsCpp
    ../../source/base/Channel.cpp
    ../../source/base/Connection.cpp
    ../../source/base/Link.cpp
    ../../source/base/LinkProfileParser.cpp
    ../../source/bootstrap/BootstrapChannel.cpp
    ../../source/bootstrap/BootstrapLink.cpp
    ../../source/bootstrap/BootstrapServer.cpp
    ../../source/bootstrap-file/BootstrapFileChannel.cpp
    ../../source/bootstrap-file/BootstrapFileLink.cpp
    ../../source/bootstrap-file/BootstrapFileLinkProfileParser.cpp
    ../../source/bootstrap-file/BootstrapFileWatcher.cpp
    ../../source/bootstrap-indirect/IndirectBootstrapChannel.cpp
    ../../source/bootstrap-indirect/IndirectBootstrapLink.cpp
    ../../source/config/helper.cpp
    ../../source/direct/DirectChannel.cpp
    ../../source/direct/DirectLink.cpp
    ../../source/direct/DirectLinkProfileParser.cpp
    ../../source/utils/base64.cpp
    ../../source/utils/log.cpp
    ../../source/utils/PortAllocator.cpp
    ../../source/whiteboard/IndirectChannel.cpp
    ../../source/whiteboard/TwosixWhiteboardLink.cpp
    ../../source/whiteboard/TwosixWhiteboardLinkProfileParser.cpp
    ../../source/PluginCommsTwoSixCpp.cpp

    base64.cpp
    BootstrapServer.cpp
    Channel.cpp
    ConfigHelper.cpp
    DirectLink.cpp
    Link.cpp
    LinkProfileParser.cpp
    main.cpp
    PluginCommsTwoSixCpp.cpp
    PortAllocator.cpp
)

# Disable compiling implementations for createPluginComms and destroyPluginComms
target_compile_definitions(unitTestPluginCommsCpp PUBLIC TESTBUILD)

set_target_properties(unitTestPluginCommsCpp PROPERTIES LINK_FLAGS "-Wl,-rpath,../../build")

find_package(GTest REQUIRED CONFIG)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

find_library(LIB_CPPREST cpprest)
find_library(LIB_CRYPTO crypto)
find_library(LIB_SSL ssl)

target_link_libraries(unitTestPluginCommsCpp
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
    target_link_libraries(unitTestPluginCommsCpp raceSdkTestMocks)
endif()

add_dependencies(build_plugin_comms_twosix_cpp_tests unitTestPluginCommsCpp)
add_test(plugin_comms_twosix_cpp ${CMAKE_CURRENT_BINARY_DIR}/unitTestPluginCommsCpp)
set_tests_properties(plugin_comms_twosix_cpp PROPERTIES LABELS "unit;plugin_comms_twosix_cpp")
setup_coverage_for_target(
    TARGET unitTestPluginCommsCpp
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../..
)
setup_valgrind_for_target(unitTestPluginCommsCpp)
setup_clang_format_for_target(unitTestPluginCommsCpp PARENT plugin_comms_twosix_cpp)
