
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

################################################################################
# User Model component
################################################################################

setup_component_target(
    TARGET PluginCommsTwoSixStubUserModelFile
    SOURCES
        PluginCommsTwoSixStubUserModelFile.cpp
        ../common/log.cpp
)

if(ANDROID)
    if ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_arm64-v8a")
        set(NODE_TYPE android-arm64-v8a-client)
    else()
        set(NODE_TYPE android-x86_64-client)
    endif()

    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PLUGIN_BASE_DIR}/artifacts/${NODE_TYPE}/PluginCommsTwoSixStubDecomposed
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/actions.json ${PLUGIN_BASE_DIR}/artifacts/${NODE_TYPE}/PluginCommsTwoSixStubDecomposed/
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/actions.json
        OUTPUT ${PLUGIN_BASE_DIR}/artifacts/${NODE_TYPE}/PluginCommsTwoSixStubDecomposed/actions.json
    )
    add_custom_target(PluginCommsTwoSixStubManifest ALL
        DEPENDS ${PLUGIN_BASE_DIR}/artifacts/${NODE_TYPE}/PluginCommsTwoSixStubDecomposed/actions.json
    )
else()
    if ("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_arm64-v8a")
        set(NODE_TYPE linux-arm64-v8a)
    else()
        set(NODE_TYPE linux-x86_64)
    endif()
    
    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PLUGIN_BASE_DIR}/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixStubDecomposed
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PLUGIN_BASE_DIR}/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixStubDecomposed
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/actions.json ${PLUGIN_BASE_DIR}/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixStubDecomposed/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/actions.json ${PLUGIN_BASE_DIR}/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixStubDecomposed/
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/actions.json
        OUTPUT
            ${PLUGIN_BASE_DIR}/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixStubDecomposed/actions.json
            ${PLUGIN_BASE_DIR}/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixStubDecomposed/actions.json
    )
    add_custom_target(PluginCommsTwoSixStubManifest ALL DEPENDS
        ${PLUGIN_BASE_DIR}/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixStubDecomposed/actions.json
        ${PLUGIN_BASE_DIR}/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixStubDecomposed/actions.json
    )
endif()