
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

add_executable(unitTestPluginCommsGolang main.cpp)

find_package(GTest REQUIRED CONFIG)
find_package(Threads REQUIRED)

target_link_libraries(unitTestPluginCommsGolang
    GTest::gmock
    GTest::gtest
    Threads::Threads
    raceSdkCommon
)

if (TARGET raceSdkTestMocks)
    target_link_libraries(unitTestPluginCommsGolang raceSdkTestMocks)
endif()

if (TARGET PluginCommsTwoSixGolang)
    get_target_property(PLUGIN_BINARY_DIR PluginCommsTwoSixGolang BINARY_DIR)
    target_link_libraries(unitTestPluginCommsGolang
        ${PLUGIN_BINARY_DIR}/libPluginCommsTwoSixGolang.so
    )
    add_dependencies(unitTestPluginCommsGolang PluginCommsTwoSixGolang)
else()
    target_link_libraries(unitTestPluginCommsGolang PluginCommsTwoSixGolang)
endif()

if (TARGET commsPluginBindingsGolang)
    target_link_libraries(unitTestPluginCommsGolang commsPluginBindingsGolang)
else()
    target_link_libraries(unitTestPluginCommsGolang
        # Have to use full path since it doesn't have the lib prefix
        /usr/local/lib/commsPluginBindingsGolang.so
    )
endif()

add_dependencies(build_plugin_comms_twosix_golang_tests unitTestPluginCommsGolang)
add_test(plugin_comms_twosix_golang ${CMAKE_CURRENT_BINARY_DIR}/unitTestPluginCommsGolang)
set_tests_properties(plugin_comms_twosix_golang PROPERTIES LABELS "unit;plugin_comms_twosix_golang")
setup_valgrind_for_target(unitTestPluginCommsGolang)
setup_clang_format_for_target(unitTestPluginCommsGolang PARENT plugin_comms_twosix_golang)
