
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
add_library(RaceJavaShims SHARED
    JavaIds.cpp
    JavaShimUtils.cpp
    PluginNMJavaWrapper.cpp
    PluginCommsJavaWrapper.cpp
    RaceAppWrapper.cpp
    ShimsJava_AppConfig.cpp
    ShimsJava_Helpers.cpp
    ShimsJava_JRaceSdkNM.cpp
    ShimsJava_JRaceSdkComms.cpp
    ShimsJava_RaceApp.cpp
    ShimsJava_RaceLog.cpp
    ShimsJava_RaceSdkApp.cpp
    ShimsJava_RaceTestApp.cpp
)

# Add the headers to the shared library
target_include_directories(RaceJavaShims
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)
set_property(TARGET RaceJavaShims PROPERTY PUBLIC_HEADER
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/JavaShimUtils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/PluginNMJavaWrapper.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/PluginCommsJavaWrapper.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/RaceAppWrapper.h
)
set_property(TARGET RaceJavaShims PROPERTY PRIVATE_HEADER
  ${CMAKE_CURRENT_SOURCE_DIR}/JavaIds.h
)

target_link_options(RaceJavaShims PRIVATE --no-undefined)

# Link external dependencies
find_library(LIB_CRYPTO crypto REQUIRED)
add_dependencies(RaceJavaShims RaceSdkJavaShimsJniHeaders)

if(ANDROID)
  target_link_libraries(RaceJavaShims
    PUBLIC
      raceSdkCommon
      raceSdkStorageEncryption
      raceSdkCore
      raceTestAppShared
      ${LIB_CRYPTO}
    PRIVATE
      RaceSdkJavaShimsJniHeaders
  )
else()
  find_package(JNI 1.8 REQUIRED)
  target_include_directories(RaceJavaShims SYSTEM PUBLIC ${JNI_INCLUDE_DIRS})
  target_link_libraries(RaceJavaShims
    PUBLIC
      raceSdkCommon
      raceSdkStorageEncryption
      raceSdkCore
      raceTestAppShared
      ${JAVA_JVM_LIBRARY}
      ${LIB_CRYPTO}
      stdc++fs
    PRIVATE
      RaceSdkJavaShimsJniHeaders
  )
endif()

# TODO enable performance-* and readability-* checks (needs fixes)
setup_clang_tidy_for_target(TARGET RaceJavaShims)
setup_cppcheck_for_target(TARGET RaceJavaShims CHECKS warning style)
setup_clang_format_for_target(RaceJavaShims PARENT racesdk)
