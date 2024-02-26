
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

file(GLOB NETWORK_MANAGER_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

################################################################################
# Client plugin
################################################################################

# Add the source to the shared library
add_library(PluginNMClientTwoSixStub SHARED
    BootstrapManager.cpp
    ClearMessagePackageTracker.cpp
    ConfigPersonas.cpp
    ConfigStaticLinks.cpp
    ConfigNMTwoSix.cpp
    ExtClrMsg.cpp
    helper.cpp
    LinkManager.cpp
    LinkProfile.cpp
    LinkWizard.cpp
    Log.cpp
    Persona.cpp
    PluginNMTwoSix.cpp
    PluginNMTwoSixClientCpp.cpp
    RaceCrypto.cpp
)

# Add the headers to the shared library
set_target_properties(PluginNMClientTwoSixStub PROPERTIES
    PUBLIC_HEADER "${NETWORK_MANAGER_HEADERS}"
)
target_include_directories(PluginNMClientTwoSixStub PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Set project name for logging
target_compile_definitions(PluginNMClientTwoSixStub PUBLIC
    NETWORK_MANAGER_PLUGIN_LOGGING_NAME="PluginNMClientTwoSixStub"
    BUILD_VERSION="${BUILD_VERSION}"
)

# Disable global symbol visibility by default so that programs that link
# against the shared library don't see functions internal to the plugin.
# This prevents symbol clashes with other plugins or other parts of the project.
set_target_properties(PluginNMClientTwoSixStub PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    NO_SONAME ON
)

################################################################################
# Server plugin
################################################################################

# Add the source to the shared library
add_library(PluginNMServerTwoSixStub SHARED
    BootstrapManager.cpp
    ConfigPersonas.cpp
    ConfigStaticLinks.cpp
    ConfigNMTwoSix.cpp
    ExtClrMsg.cpp
    helper.cpp
    LinkManager.cpp
    LinkProfile.cpp
    LinkWizard.cpp
    Log.cpp
    Persona.cpp
    PluginNMTwoSix.cpp
    PluginNMTwoSixServerCpp.cpp
    RaceCrypto.cpp
)

# Add the headers to the shared library
set_target_properties(PluginNMServerTwoSixStub PROPERTIES
    PUBLIC_HEADER "${NETWORK_MANAGER_HEADERS}"
)
target_include_directories(PluginNMServerTwoSixStub PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Set project name for logging
target_compile_definitions(PluginNMServerTwoSixStub PUBLIC
    NETWORK_MANAGER_PLUGIN_LOGGING_NAME="PluginNMServerTwoSixStub"
    BUILD_VERSION="${BUILD_VERSION}"
)

# Disable global symbol visibility by default so that programs that link
# against the shared library don't see functions internal to the plugin.
# This prevents symbol clashes with other plugins or other parts of the project.
set_target_properties(PluginNMServerTwoSixStub PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    NO_SONAME ON
)

if(ANDROID)
    find_library(LIB_CRYPTO crypto)
    find_package(Boost COMPONENTS filesystem system REQUIRED) 

    if(TARGET raceSdkCommon)
        # If this is a global build, use the targets directly so that cmake can do its dependency magic
        set(LIB_SDK_COMMON raceSdkCommon)
    else()
        # Otherwise, just treat them as any other library
        find_library(LIB_SDK_COMMON raceSdkCommon)
    endif()

    target_link_libraries(PluginNMClientTwoSixStub
        ${LIB_SDK_COMMON}
        ${LIB_CRYPTO}
        Boost::system
        Boost::filesystem
    )
    target_link_libraries(PluginNMServerTwoSixStub
        ${LIB_SDK_COMMON}
        ${LIB_CRYPTO}
        Boost::system
        Boost::filesystem
    )

else()
    ## Default to Linux
    find_library(LIB_CRYPTO crypto)

    target_link_libraries(PluginNMClientTwoSixStub
        raceSdkCommon
        ${LIB_CRYPTO}
        stdc++fs
    )
    target_link_libraries(PluginNMServerTwoSixStub
        raceSdkCommon
        ${LIB_CRYPTO}
        stdc++fs
    )

endif()

install(
    TARGETS PluginNMClientTwoSixStub PluginNMServerTwoSixStub
    LIBRARY
        DESTINATION lib/race/network-manager/PluginNMTwoSixStub
)

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json
        DESTINATION lib/race/network-manager/PluginNMTwoSixStub
)

if(ANDROID)
    if ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_arm64-v8a")
        set(NODE_TYPE android-arm64-v8a-client)
    else()
        set(NODE_TYPE android-x86_64-client)
    endif()

    add_custom_command(TARGET PluginNMClientTwoSixStub POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}/PluginNMTwoSixStub
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginNMClientTwoSixStub> ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}/PluginNMTwoSixStub/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}/PluginNMTwoSixStub/
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/manifest.json
        BYPRODUCTS
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/race-python-utils
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}/PluginNMTwoSixStub/
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

    add_custom_command(TARGET PluginNMClientTwoSixStub POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginNMTwoSixStub
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginNMClientTwoSixStub> ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginNMTwoSixStub/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginNMTwoSixStub/
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/manifest.json
        BYPRODUCTS
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/race-python-utils
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginNMTwoSixStub/
    )
    add_custom_command(TARGET PluginNMServerTwoSixStub POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginNMTwoSixStub
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginNMServerTwoSixStub> ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginNMTwoSixStub/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginNMTwoSixStub/
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/manifest.json
        BYPRODUCTS
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/race-python-utils
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginNMTwoSixStub/
    )
    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/
        ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/
    )
endif()

# TODO enable performance-* and readability-* checks (needs fixes)
setup_clang_tidy_for_target(TARGET PluginNMClientTwoSixStub)
setup_clang_tidy_for_target(TARGET PluginNMServerTwoSixStub)

setup_cppcheck_for_target(
    TARGET PluginNMClientTwoSixStub
    CHECKS all
    SUPPRESS unmatchedSuppression unusedFunction missingIncludeSystem useStlAlgorithm
)
setup_cppcheck_for_target(
    TARGET PluginNMServerTwoSixStub
    CHECKS all
    SUPPRESS unmatchedSuppression unusedFunction missingIncludeSystem useStlAlgorithm
)

setup_clang_format_for_target(PluginNMClientTwoSixStub PARENT plugin_network_manager_twosix_cpp)
setup_clang_format_for_target(PluginNMServerTwoSixStub PARENT plugin_network_manager_twosix_cpp ADD_TO_LOCAL_TARGET)
