
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

# Add the source to the shared library
add_library(raceSdkCore SHARED
    AppWrapper.cpp
    ArtifactManager.cpp
    ArtifactManagerWrapper.cpp
    BootstrapManager.cpp
    BootstrapThread.cpp
    Composition.cpp
    ConfigLogging.cpp
    FileSystemHelper.cpp
    Handler.cpp
    helper.cpp
    JsonConfig.cpp
    PluginDef.cpp
    RaceChannels.cpp
    RaceConfig.cpp
    RaceLinks.cpp
    RaceSdk.cpp
    NMWrapper.cpp
    CommsWrapper.cpp
    TestHarnessWrapper.cpp
    VoaConfig.cpp
    VoaThread.cpp
    decomposed-comms/ComponentActionManager.cpp
    decomposed-comms/ComponentConnectionManager.cpp
    decomposed-comms/ComponentLifetimeManager.cpp
    decomposed-comms/ComponentLinkManager.cpp
    decomposed-comms/ComponentManager.cpp
    decomposed-comms/ComponentManagerTypes.cpp
    decomposed-comms/ComponentPackageManager.cpp
    decomposed-comms/ComponentReceivePackageManager.cpp
    decomposed-comms/ComponentWrappers.cpp
    decomposed-comms/MimeTypes.cpp
    decomposed-comms/SdkWrappers.cpp
    plugin-loading/ComponentPlugin.cpp
    plugin-loading/DecomposedPluginLoader.cpp
    plugin-loading/DynamicLibrary.cpp
    plugin-loading/PluginLoader.cpp
    plugin-loading/PythonComponentPlugin.cpp
    plugin-loading/PythonLoaderWrapper.cpp
)

# Add the headers to the shared library
file(GLOB_RECURSE SDK_CORE_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/*.h
)
set_target_properties(raceSdkCore PROPERTIES
    PUBLIC_HEADER "${SDK_CORE_PUBLIC_HEADERS}"
)
target_include_directories(raceSdkCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

file(GLOB_RECURSE SDK_CORE_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
set_target_properties(raceSdkCore PROPERTIES
    PRIVATE_HEADER "${SDK_CORE_PRIVATE_HEADERS}"
)

target_compile_definitions(raceSdkCore PUBLIC SWIG_TYPE_TABLE=RaceSdk)
target_link_options(raceSdkCore PRIVATE --no-undefined)

# Link external dependencies
find_package(PythonLibs 3.7 EXACT REQUIRED)
find_library(LIB_ARCHIVE archive REQUIRED)

if(ANDROID)
    find_package(Boost COMPONENTS filesystem system REQUIRED)
    find_library(LIB_CRYPTO crypto REQUIRED)
    find_library(LIB_SSL ssl REQUIRED)
    find_library(LIB_ZIP zip REQUIRED)

    target_link_libraries(raceSdkCore
        PRIVATE
            raceSdkCommon
            raceSdkStorageEncryption
            PluginNMTwoSixTestHarness
            language-shims-headers
            Boost::system
            Boost::filesystem
            ${LIB_CRYPTO}
            ${PYTHON_LIBRARIES}
            ${LIB_SSL}
            ${LIB_ZIP}
            ${CMAKE_DL_LIBS} # Contains dlopen and dlcose for dynamically loading plugins.
            ${LIB_ARCHIVE}
    )
    target_include_directories(raceSdkCore SYSTEM PRIVATE ${PYTHON_INCLUDE_PATH})

else()
    find_library(LIB_ZIP zip REQUIRED)

    # Default to Linux
    target_link_libraries(raceSdkCore
        PUBLIC
            raceSdkCommon
            raceSdkStorageEncryption
            language-shims-headers
            ${LIB_ZIP}
            stdc++fs
            crypto
            ${CMAKE_DL_LIBS} # Contains dlopen and dlcose for dynamically loading plugins.
            ${PYTHON_LIBRARIES}
            ${LIB_ARCHIVE}

        PRIVATE
            PluginNMTwoSixTestHarness
    )
    target_include_directories(raceSdkCore SYSTEM PRIVATE ${PYTHON_INCLUDE_PATH})

endif()


# TODO enable performance-* and readability-* checks (needs fixes)
setup_clang_tidy_for_target(TARGET raceSdkCore)
setup_cppcheck_for_target(
    TARGET raceSdkCore
    CHECKS all
    SUPPRESS unmatchedSuppression unusedFunction missingIncludeSystem useStlAlgorithm
)
setup_clang_format_for_target(raceSdkCore PARENT racesdk)
