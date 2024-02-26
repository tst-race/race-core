
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
# plugin loader C++ library
################################################################################

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(PluginCommsTwoSixRust SHARED
    ChannelPropertiesC.cpp
    ChannelRoleC.cpp
    LinkPropertiesC.cpp
    PluginCommsRustCppWrapper.cpp
    helper.cpp
    loader.cpp
    race_log.cpp
    sdk_c_wrapper.cpp
)

# Add the headers to the shared library (so they get formatted)
file(GLOB COMMS_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
set_target_properties(PluginCommsTwoSixRust PROPERTIES
    PRIVATE_HEADER "${COMMS_HEADERS}"
)

target_compile_definitions(PluginCommsTwoSixRust PUBLIC
    BUILD_VERSION="${BUILD_VERSION}"
)

if(TARGET raceSdkCommon)
    set(LIB_SDK_COMMON raceSdkCommon)
else()
    find_library(LIB_SDK_COMMON raceSdkCommon)
endif()

find_library(LIB_SSL ssl)
find_library(LIB_CRYPTO crypto)

target_link_libraries(PluginCommsTwoSixRust
    ${LIB_SDK_COMMON}
    PluginCommsTwoSixRustWrapper
    dl
    ${LIB_SSL}
    ${LIB_CRYPTO})

setup_clang_tidy_for_target(TARGET PluginCommsTwoSixRust)
setup_cppcheck_for_target(
    TARGET PluginCommsTwoSixRust
    CHECKS all
    SUPPRESS unmatchedSuppression unusedFunction missingIncludeSystem useStlAlgorithm
)
setup_clang_format_for_target(PluginCommsTwoSixRust PARENT plugin_comms_twosix_rust)

install(TARGETS PluginCommsTwoSixRust
    LIBRARY DESTINATION lib/race/comms/PluginCommsTwoSixRust)

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json
    DESTINATION lib/race/comms/PluginCommsTwoSixRust
)

if(ANDROID)
    if ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_x86_64")
        add_custom_command(TARGET PluginCommsTwoSixRust POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/channels/race-python-utils
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-x86_64-client/PluginCommsTwoSixRust
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginCommsTwoSixRust> ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-x86_64-client/PluginCommsTwoSixRust/
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-x86_64-client/PluginCommsTwoSixRust/
        )
        set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-x86_64-client/
        )
    else()
        add_custom_command(TARGET PluginCommsTwoSixRust POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/channels/race-python-utils
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-arm64-v8a-client/PluginCommsTwoSixRust
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginCommsTwoSixRust> ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-arm64-v8a-client/PluginCommsTwoSixRust/
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-arm64-v8a-client/PluginCommsTwoSixRust/
        )
        set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-arm64-v8a-client/
        )
    endif()
else()
    if ("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_arm64-v8a")
        set(NODE_TYPE linux-arm64-v8a)
    else()
        set(NODE_TYPE linux-x86_64)
    endif()

    add_custom_command(TARGET PluginCommsTwoSixRust POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/channels/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixRust
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixRust
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginCommsTwoSixRust> ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixRust/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixRust/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginCommsTwoSixRust> ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixRust/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixRust/
    )
    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/
        ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/
    )
endif()
