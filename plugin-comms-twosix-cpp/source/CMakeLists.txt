
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
# plugin
################################################################################

# Setup Packages

# Add the source to the shared library
add_library(PluginCommsTwoSixStub SHARED
    base/Channel.cpp
    base/Connection.cpp
    base/Link.cpp
    base/LinkProfileParser.cpp
    bootstrap/BootstrapChannel.cpp
    bootstrap/BootstrapLink.cpp
    bootstrap/BootstrapServer.cpp
    bootstrap-indirect/IndirectBootstrapChannel.cpp
    bootstrap-indirect/IndirectBootstrapLink.cpp
    bootstrap-file/BootstrapFileChannel.cpp
    bootstrap-file/BootstrapFileLink.cpp
    bootstrap-file/BootstrapFileLinkProfileParser.cpp
    bootstrap-file/BootstrapFileWatcher.cpp
    config/helper.cpp
    direct/DirectChannel.cpp
    direct/DirectLink.cpp
    direct/DirectLinkProfileParser.cpp
    utils/base64.cpp
    utils/log.cpp
    utils/PortAllocator.cpp
    whiteboard/IndirectChannel.cpp
    whiteboard/TwosixWhiteboardLink.cpp
    whiteboard/TwosixWhiteboardLinkProfileParser.cpp
    PluginCommsTwoSixCpp.cpp
)

# Add the headers to the shared library
file(GLOB_RECURSE COMMS_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
set_target_properties(PluginCommsTwoSixStub PROPERTIES
    PRIVATE_HEADER "${COMMS_HEADERS}"
)

target_compile_definitions(PluginCommsTwoSixStub PUBLIC
    BUILD_VERSION="${BUILD_VERSION}"
)

# Disable global symbol visibility by default so that programs that link
# against the shared library don't see functions internal to the plugin.
# This prevents symbol clashes with other plugins or other parts of the project.
set_target_properties(PluginCommsTwoSixStub PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    NO_SONAME ON
)

if(ANDROID)
    find_library(LIB_CURL curl)
    find_package(Boost COMPONENTS filesystem system REQUIRED) 
    find_library(LIB_CPPREST cpprest)
    find_library(LIB_LOG log)

    if(TARGET raceSdkCommon)
      set(RACE_JAVA_SHIMS RaceJavaShims)
    else()
      find_library(RACE_JAVA_SHIMS RaceJavaShims)
    endif()

    if(TARGET raceSdkCommon)
        # If this is a global build, use the targets directly so that cmake can do its dependency magic
        set(LIB_SDK_COMMON raceSdkCommon)
    else()
        # Otherwise, just treat them as any other library
        find_library(LIB_SDK_COMMON raceSdkCommon)
    endif()

    target_link_libraries(PluginCommsTwoSixStub
        ${LIB_SDK_COMMON}
        ${LIB_CURL}
        Boost::system
        Boost::filesystem
        ${LIB_CPPREST}
        ${LIB_LOG}
        ${RACE_JAVA_SHIMS}
    )

else()
    ## Default to Linux
    find_library(LIB_CURL curl)
    find_library(LIB_CPPREST cpprest)
    find_library(LIB_CRYPTO crypto)
    find_library(LIB_SSL ssl)

    target_link_libraries(PluginCommsTwoSixStub
        stdc++fs
        raceSdkCommon
        ${LIB_CURL}
        ${LIB_CPPREST}
        ${LIB_CRYPTO}
        ${LIB_SSL}
    )

endif()

# TODO enable performance-* and readability-* checks (needs fixes)
setup_clang_tidy_for_target(TARGET PluginCommsTwoSixStub)
setup_cppcheck_for_target(
    TARGET PluginCommsTwoSixStub
    CHECKS all
    SUPPRESS unmatchedSuppression unusedFunction missingIncludeSystem
)
setup_clang_format_for_target(PluginCommsTwoSixStub PARENT plugin_comms_twosix_cpp)

################################################################################
# install
################################################################################

install(
    TARGETS PluginCommsTwoSixStub
    LIBRARY
        DESTINATION lib/race/comms/PluginCommsTwoSixStub
)
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json
        DESTINATION lib/race/comms/PluginCommsTwoSixStub
)

if(ANDROID)
    if ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_arm64-v8a")
        set(NODE_TYPE android-arm64-v8a-client)
    else()
        set(NODE_TYPE android-x86_64-client)
    endif()

    add_custom_command(TARGET PluginCommsTwoSixStub POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/channels/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}/PluginCommsTwoSixStub
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginCommsTwoSixStub> ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}/PluginCommsTwoSixStub/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}/PluginCommsTwoSixStub/
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../../source/aux-data ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}/PluginCommsTwoSixStub/aux-data
        BYPRODUCTS 
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/channels/race-python-utils
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}/PluginCommsTwoSixStub/
    )
    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}/
    )
else()
    if ("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_arm64-v8a")
        set(NODE_TYPE linux-arm64-v8a)
    else()
        set(NODE_TYPE linux-x86_64)
    endif()

    add_custom_command(TARGET PluginCommsTwoSixStub POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/channels/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixStub
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixStub
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginCommsTwoSixStub> ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixStub/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixStub/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginCommsTwoSixStub> ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixStub/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixStub/
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../../source/aux-data ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixStub/aux-data
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../../source/aux-data ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixStub/aux-data
        BYPRODUCTS 
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/channels/race-python-utils
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixStub/
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixStub/
    )
    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/
        ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/
    )
endif()
