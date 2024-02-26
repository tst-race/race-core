
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

if(NOT ANDROID)
  # Link JVM
  if ("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_arm64-v8a")
    include_directories("/usr/lib/jvm/java-1.8.0-openjdk-arm64/include")
    include_directories("/usr/lib/jvm/java-8-openjdk-arm64/include/linux/")
    link_directories("/usr/lib/jvm/java-1.8.0-openjdk-arm64/jre/lib/aarch64/server/")
  else()
    include_directories("/usr/lib/jvm/java-1.8.0-openjdk-amd64/include")
    include_directories("/usr/lib/jvm/java-8-openjdk-amd64/include/linux/")
    link_directories("/usr/lib/jvm/java-1.8.0-openjdk-amd64/jre/lib/amd64/server/")
  endif()
endif()

# Add the source to the shared library
add_library(PluginCommsTwoSixJava SHARED
    loader.cpp
)

if(TARGET raceSdkCommon)
  set(RACE_SDK_COMMON raceSdkCommon)
  set(RACE_JAVA_SHIMS RaceJavaShims)
else()
  find_library(RACE_SDK_COMMON raceSdkCommon)
  find_library(RACE_JAVA_SHIMS RaceJavaShims)
endif()


target_link_libraries(PluginCommsTwoSixJava
  ${RACE_SDK_COMMON}
  ${RACE_JAVA_SHIMS}
)

install(
  TARGETS PluginCommsTwoSixJava
    LIBRARY DESTINATION lib/race/comms/PluginCommsTwoSixJava
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json
    DESTINATION lib/race/comms/PluginCommsTwoSixJava
)

if(ANDROID)
    if ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_x86_64")
        add_custom_command(TARGET PluginCommsTwoSixJava POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/android-x86_64-client/PluginCommsTwoSixJava
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginCommsTwoSixJava> ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/android-x86_64-client/PluginCommsTwoSixJava/
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/android-x86_64-client/PluginCommsTwoSixJava/
        )
        set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/android-x86_64-client/
        )
    else()
        add_custom_command(TARGET PluginCommsTwoSixJava POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/android-arm64-v8a-client/PluginCommsTwoSixJava
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginCommsTwoSixJava> ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/android-arm64-v8a-client/PluginCommsTwoSixJava/
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/android-arm64-v8a-client/PluginCommsTwoSixJava/
        )
        set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/android-arm64-v8a-client/
        )
    endif()
else()
    if ("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_arm64-v8a")
        set(NODE_TYPE linux-arm64-v8a)
    else()
        set(NODE_TYPE linux-x86_64)
    endif()

    add_custom_command(TARGET PluginCommsTwoSixJava POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixJava
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixJava
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginCommsTwoSixJava> ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixJava/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixJava/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginCommsTwoSixJava> ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixJava/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixJava/
    )
    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/${NODE_TYPE}-client/
        ${CMAKE_CURRENT_SOURCE_DIR}/../../kit/artifacts/${NODE_TYPE}-server/
    )
endif()

setup_clang_tidy_for_target(TARGET PluginCommsTwoSixJava)
setup_cppcheck_for_target(
    TARGET PluginCommsTwoSixJava
    CHECKS all
    SUPPRESS unmatchedSuppression unusedFunction missingIncludeSystem
)
setup_clang_format_for_target(PluginCommsTwoSixJava PARENT plugin_comms_twosix_java)
