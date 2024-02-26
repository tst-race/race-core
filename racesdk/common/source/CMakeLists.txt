
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
add_library(raceSdkCommon SHARED
    AppConfig.cpp
    base64.cpp
    BootstrapState.cpp
    ChannelProperties.cpp
    ChannelRole.cpp
    ChannelStatus.cpp
    ClrMsg.cpp
    ComponentTypes.cpp
    ConnectionStatus.cpp
    ConnectionType.cpp
    EncPkg.cpp
    LinkProperties.cpp
    LinkPropertyPair.cpp
    LinkPropertySet.cpp
    LinkStatus.cpp
    LinkType.cpp
    MessageStatus.cpp
    OpenTracingHelpers.cpp
    PackageStatus.cpp
    PackageType.cpp
    PluginResponse.cpp
    PluginStatus.cpp
    RaceEnums.cpp
    RaceException.cpp
    RaceLog.cpp
    SdkResponse.cpp
    SendType.cpp
    TransmissionType.cpp
)

# Add the headers to the shared library
file(GLOB SDK_COMMON_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/*.h
)
set_target_properties(raceSdkCommon PROPERTIES
    PUBLIC_HEADER "${SDK_COMMON_HEADERS}"
)
target_include_directories(raceSdkCommon PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(raceSdkCommon PUBLIC "SWIG_TYPE_TABLE=RaceSdk")
target_link_options(raceSdkCommon PRIVATE --no-undefined)

# Link external dependencies
find_library(LIB_JAEGERTRACING libjaegertracing.a) # Has to be a static library
find_library(LIB_OPENTRACING libopentracing.a)
find_library(LIB_THRIFT thrift)
find_library(LIB_YAML yaml-cpp)
find_library(LIB_SSL ssl)
find_library(LIB_CRYPTO crypto)
find_package(Threads REQUIRED)


if(ANDROID)
    # Library for logging messages to logcat on Android.
    find_library(LIB_LOG log REQUIRED)

    target_link_libraries(raceSdkCommon
    PUBLIC
        ${LIB_THRIFT}
        ${LIB_YAML}
        ${LIB_SSL}
        ${LIB_CRYPTO}
        Threads::Threads
        ${LIB_LOG}
    PRIVATE
        ${LIB_JAEGERTRACING}
        ${LIB_OPENTRACING}
    )
else()
    # Default to Linux
    target_link_libraries(raceSdkCommon
    PUBLIC
        ${LIB_THRIFT}
        ${LIB_YAML}
        ${LIB_SSL}
        ${LIB_CRYPTO}
        Threads::Threads
    PRIVATE
        ${LIB_JAEGERTRACING}
        ${LIB_OPENTRACING}
    )
endif()


# TODO enable performance-* and readability-* checks (needs fixes)
setup_clang_tidy_for_target(TARGET raceSdkCommon)
setup_cppcheck_for_target(TARGET raceSdkCommon CHECKS warning style)
setup_clang_format_for_target(raceSdkCommon PARENT racesdk)
