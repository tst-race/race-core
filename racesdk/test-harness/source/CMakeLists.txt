
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

# Will fail to link with racesdk-core without setting this.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(PluginNMTwoSixTestHarness STATIC
    helpers.cpp
    MessageSerializer/helpers.cpp
    MessageSerializer/MessageSerializer.cpp
    PluginNMTestHarness.cpp
)

file(GLOB TEST_HARNESS_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/*.h
)
set_target_properties(PluginNMTwoSixTestHarness PROPERTIES
    PUBLIC_HEADER "${TEST_HARNESS_PUBLIC_HEADERS}"
)
target_include_directories(PluginNMTwoSixTestHarness PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

file(GLOB TEST_HARNESS_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
set_target_properties(PluginNMTwoSixTestHarness PROPERTIES
    PRIVATE_HEADER "${TEST_HARNESS_PRIVATE_HEADERS}"
)

target_link_libraries(PluginNMTwoSixTestHarness PRIVATE raceSdkCommon)

target_compile_definitions(PluginNMTwoSixTestHarness PUBLIC
    BUILD_VERSION="${BUILD_VERSION}"
)
target_link_options(PluginNMTwoSixTestHarness PRIVATE --no-undefined)

# TODO enable performance-* and readability-* checks (needs fixes)
setup_clang_tidy_for_target(TARGET PluginNMTwoSixTestHarness)
setup_cppcheck_for_target(TARGET PluginNMTwoSixTestHarness CHECKS warning style)
setup_clang_format_for_target(PluginNMTwoSixTestHarness PARENT racesdk)
