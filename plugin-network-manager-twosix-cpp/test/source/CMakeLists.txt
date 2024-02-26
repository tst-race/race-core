
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
include_directories("../../source")
link_directories("../../build/")

# Add the source files to the project
add_executable(unitTestPluginNMCpp
    ../../source/BootstrapManager.cpp
    ../../source/ClearMessagePackageTracker.cpp
    ../../source/ConfigPersonas.cpp
    ../../source/ConfigStaticLinks.cpp
    ../../source/ConfigNMTwoSix.cpp
    ../../source/ExtClrMsg.cpp
    ../../source/helper.cpp
    ../../source/LinkWizard.cpp
    ../../source/LinkManager.cpp
    ../../source/LinkProfile.cpp
    ../../source/Log.cpp
    ../../source/Persona.cpp
    ../../source/PluginNMTwoSix.cpp
    ../../source/PluginNMTwoSixClientCpp.cpp
    ../../source/PluginNMTwoSixServerCpp.cpp
    ../../source/RaceCrypto.cpp
    ../../source/LinkWizard.cpp

    BootstrapManagerTests.cpp
    LinkWizard.cpp
    ConfigNMTwoSix.cpp
    ConfigPersonas.cpp
    ConfigStaticLinks.cpp
    LinkManagerTest.cpp
    PluginNMTwoSixClientCpp.cpp
    PluginNMTwoSixServerCpp.cpp
    RaceCrypto.cpp
    main.cpp
)

# Disable compiling implementations for createPluginNM and destroyPluginNM
target_compile_definitions(unitTestPluginNMCpp PUBLIC TESTBUILD JSON_DIAGNOSTICS=1)

# Find OpenSSL for SHA256
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(GTest REQUIRED CONFIG)

target_link_libraries(unitTestPluginNMCpp
    GTest::gmock
    GTest::gtest
    Threads::Threads
    raceSdkCommon
    OpenSSL::Crypto
    stdc++fs
)

if (TARGET raceSdkTestMocks)
    target_link_libraries(unitTestPluginNMCpp raceSdkTestMocks)
endif()

add_dependencies(build_plugin_network_manager_twosix_cpp_tests unitTestPluginNMCpp)
add_test(plugin_network_manager_twosix_cpp ${CMAKE_CURRENT_BINARY_DIR}/unitTestPluginNMCpp)
set_tests_properties(plugin_network_manager_twosix_cpp PROPERTIES LABELS "unit;plugin_network_manager_twosix_cpp")
setup_coverage_for_target(
    TARGET unitTestPluginNMCpp
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../..
)
setup_valgrind_for_target(unitTestPluginNMCpp)
setup_clang_format_for_target(unitTestPluginNMCpp PARENT plugin_network_manager_twosix_cpp)
