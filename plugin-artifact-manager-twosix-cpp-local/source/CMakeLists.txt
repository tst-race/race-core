
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

add_library(PluginArtifactManagerTwoSixCppLocal SHARED
    PluginArtifactManagerTwoSixCppLocal.cpp
    log.cpp
    zip.cpp
)

file(GLOB PLUGIN_ARTIFACT_MANAGER_TWO_SIX_CPP_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
set_target_properties(PluginArtifactManagerTwoSixCppLocal PROPERTIES
    PRIVATE_HEADER "${PLUGIN_ARTIFACT_MANAGER_TWO_SIX_CPP_PRIVATE_HEADERS}"
)

target_compile_definitions(PluginArtifactManagerTwoSixCppLocal PUBLIC
    BUILD_VERSION="${BUILD_VERSION}"
)

# Disable global symbol visibility by default so that programs that link
# against the shared library don't see functions internal to the plugin.
# This prevents symbol clashes with other plugins or other parts of the project.
set_target_properties(PluginArtifactManagerTwoSixCppLocal PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    NO_SONAME ON
)

###
# Link external dependencies
###

find_library(LIB_ARCHIVE archive REQUIRED)

if(TARGET raceSdkCommon)
    set(LIB_SDK_COMMON raceSdkCommon)
else()
    find_library(LIB_SDK_COMMON raceSdkCommon REQUIRED)
endif()

target_link_libraries(PluginArtifactManagerTwoSixCppLocal
    ${LIB_SDK_COMMON}
    ${LIB_ARCHIVE}
)

###
# Setup style validation
###

# TODO enable performance-* and readability-* checks (needs fixes)
setup_clang_tidy_for_target(TARGET PluginArtifactManagerTwoSixCppLocal)
setup_cppcheck_for_target(
    TARGET PluginArtifactManagerTwoSixCppLocal
    CHECKS all
    SUPPRESS unmatchedSuppression unusedFunction missingIncludeSystem
)
setup_clang_format_for_target(PluginArtifactManagerTwoSixCppLocal PARENT plugin_artifact_manager_twosix_cpp)

###
# Install plugin artifacts
###

install(
    TARGETS PluginArtifactManagerTwoSixCppLocal
    LIBRARY DESTINATION lib/race/artifact-manager/PluginArtifactManagerTwoSixCppLocal
)
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json
    DESTINATION lib/race/artifact-manager/PluginArtifactManagerTwoSixCppLocal
)

if(ANDROID)
    if ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_arm64-v8a")
        set(NODE_TYPE android-arm64-v8a-client)
    else()
        set(NODE_TYPE android-x86_64-client)
    endif()

    find_package(Boost COMPONENTS filesystem system REQUIRED)
    target_link_libraries(PluginArtifactManagerTwoSixCppLocal
        Boost::system
        Boost::filesystem
    )

    add_custom_command(TARGET PluginArtifactManagerTwoSixCppLocal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../race-python-utils ${PROJECT_SOURCE_DIR}/kit/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}/PluginArtifactManagerTwoSixCppLocal
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginArtifactManagerTwoSixCppLocal> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}/PluginArtifactManagerTwoSixCppLocal/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}/PluginArtifactManagerTwoSixCppLocal/
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

    add_custom_command(TARGET PluginArtifactManagerTwoSixCppLocal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../race-python-utils ${PROJECT_SOURCE_DIR}/kit/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/PluginArtifactManagerTwoSixCppLocal
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/PluginArtifactManagerTwoSixCppLocal
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginArtifactManagerTwoSixCppLocal> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/PluginArtifactManagerTwoSixCppLocal/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/PluginArtifactManagerTwoSixCppLocal/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginArtifactManagerTwoSixCppLocal> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/PluginArtifactManagerTwoSixCppLocal/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/PluginArtifactManagerTwoSixCppLocal/
    )
    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/
    )
endif()
