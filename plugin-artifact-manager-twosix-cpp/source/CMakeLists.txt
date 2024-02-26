
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

###
# Setup build for shared library
###

add_library(PluginArtifactManagerTwoSixCpp SHARED
    PluginArtifactManagerTwoSixCpp.cpp
    log.cpp
)

file(GLOB PLUGIN_ARTIFACT_MANAGER_TWO_SIX_CPP_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
set_target_properties(PluginArtifactManagerTwoSixCpp PROPERTIES
    PRIVATE_HEADER "${PLUGIN_ARTIFACT_MANAGER_TWO_SIX_CPP_PRIVATE_HEADERS}"
)

target_compile_definitions(PluginArtifactManagerTwoSixCpp PUBLIC
    BUILD_VERSION="${BUILD_VERSION}"
)

# Disable global symbol visibility by default so that programs that link
# against the shared library don't see functions internal to the plugin.
# This prevents symbol clashes with other plugins or other parts of the project.
set_target_properties(PluginArtifactManagerTwoSixCpp PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    NO_SONAME ON
)

###
# Link external dependencies
###

find_library(LIB_CURL curl REQUIRED)

if(TARGET raceSdkCommon)
    set(LIB_SDK_COMMON raceSdkCommon)
else()
    find_library(LIB_SDK_COMMON raceSdkCommon REQUIRED)
endif()

target_link_libraries(PluginArtifactManagerTwoSixCpp
    ${LIB_SDK_COMMON}
    ${LIB_CURL}
)

###
# Setup style validation
###

# TODO enable performance-* and readability-* checks (needs fixes)
setup_clang_tidy_for_target(TARGET PluginArtifactManagerTwoSixCpp)
setup_cppcheck_for_target(
    TARGET PluginArtifactManagerTwoSixCpp
    CHECKS all
    SUPPRESS unmatchedSuppression unusedFunction missingIncludeSystem
)
setup_clang_format_for_target(PluginArtifactManagerTwoSixCpp PARENT plugin_artifact_manager_twosix_cpp)

###
# Install plugin artifacts
###

install(
    TARGETS PluginArtifactManagerTwoSixCpp
    LIBRARY DESTINATION lib/race/artifact-manager/PluginArtifactManagerTwoSixCpp
)
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json
    DESTINATION lib/race/artifact-manager/PluginArtifactManagerTwoSixCpp
)

if(ANDROID)
    if ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_arm64-v8a")
        set(NODE_TYPE android-arm64-v8a-client)
    else()
        set(NODE_TYPE android-x86_64-client)
    endif()

    add_custom_command(TARGET PluginArtifactManagerTwoSixCpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../race-python-utils ${PROJECT_SOURCE_DIR}/kit/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}/PluginArtifactManagerTwoSixCpp
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginArtifactManagerTwoSixCpp> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}/PluginArtifactManagerTwoSixCpp/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}/PluginArtifactManagerTwoSixCpp/
    )
    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}/
    )
else()
    if ("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_arm64-v8a")
        set(NODE_TYPE linux-arm64-v8a)
    else()
        set(NODE_TYPE linux-x86_64)
    endif()

    add_custom_command(TARGET PluginArtifactManagerTwoSixCpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../race-python-utils ${PROJECT_SOURCE_DIR}/kit/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/PluginArtifactManagerTwoSixCpp
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/PluginArtifactManagerTwoSixCpp
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginArtifactManagerTwoSixCpp> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/PluginArtifactManagerTwoSixCpp/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/PluginArtifactManagerTwoSixCpp/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginArtifactManagerTwoSixCpp> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/PluginArtifactManagerTwoSixCpp/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/PluginArtifactManagerTwoSixCpp/
    )
    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/
    )
endif()
