
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

cmake_minimum_required(VERSION 3.20.0)
project(race-node-daemon)

file(GLOB_RECURSE DAEMON_SDK_SRC race-daemon-sdk/src/**/*)

###
# Android daemon app
###

set(BUILD_ANDROID_DAEMON OFF CACHE BOOL "Build Android daemon app")

if (BUILD_ANDROID_DAEMON)
    file(GLOB_RECURSE DAEMON_ANDROID_SRC race-daemon-android/src/**/*)
    set(ANDROID_DAEMON_APP_NAME race-daemon-android-debug.apk)
    set(ANDROID_DAEMON_APP ${CMAKE_CURRENT_SOURCE_DIR}/race-daemon-android/build/outputs/apk/debug/${ANDROID_DAEMON_APP_NAME})
    set(ANDROID_x86_64_ARTIFACTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kit/artifacts/android-x86_64-client/race-daemon/)
    set(ANDROID_arm64_v8a_ARTIFACTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kit/artifacts/android-arm64-v8a-client/race-daemon/)

    add_custom_command(
        COMMAND ./gradlew --no-daemon assembleDebug

        # Copy apk to app artifact dirs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ANDROID_x86_64_ARTIFACTS_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${ANDROID_DAEMON_APP} ${ANDROID_x86_64_ARTIFACTS_DIR}/${ANDROID_DAEMON_APP_NAME}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ANDROID_arm64_v8a_ARTIFACTS_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${ANDROID_DAEMON_APP} ${ANDROID_arm64_v8a_ARTIFACTS_DIR}/${ANDROID_DAEMON_APP_NAME}

        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS
            build.gradle
            race-daemon-android/build.gradle
            ${DAEMON_ANDROID_SRC}
            race-daemon-sdk/build.gradle
            ${DAEMON_SDK_SRC}
        OUTPUT
            ${ANDROID_DAEMON_APP}
            ${CMAKE_CURRENT_SOURCE_DIR}/race-daemon-android/.gradle/
            ${CMAKE_CURRENT_SOURCE_DIR}/race-daemon-android/build/
            ${CMAKE_CURRENT_SOURCE_DIR}/race-daemon-sdk/.gradle/
            ${CMAKE_CURRENT_SOURCE_DIR}/race-daemon-sdk/build/
            ${CMAKE_CURRENT_SOURCE_DIR}/.gradle/
            ${ANDROID_x86_64_ARTIFACTS_DIR}
            ${ANDROID_arm64_v8a_ARTIFACTS_DIR}
        COMMENT "Building Android RACE daemon app..."
    )

    add_custom_target(RaceDaemonAndroid ALL DEPENDS ${ANDROID_DAEMON_APP})

    install(
        FILES ${ANDROID_DAEMON_APP}
        DESTINATION .
    )
endif()

###
# Linux daemon app
###

set(BUILD_LINUX_DAEMON OFF CACHE BOOL "Build Linux daemon app")

if (BUILD_LINUX_DAEMON)
    file(GLOB_RECURSE DAEMON_LINUX_SRC race-daemon-linux/src/**/*)
    set(LINUX_DAEMON_APP ${CMAKE_CURRENT_SOURCE_DIR}/race-daemon-linux/build/distributions/race-daemon-linux.tar)
    set(LINUX_x86_64_CLIENT_ARTIFACTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kit/artifacts/linux-x86_64-client/race-daemon/)
    set(LINUX_x86_64_SERVER_ARTIFACTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kit/artifacts/linux-x86_64-server/race-daemon/)
    set(LINUX_arm64_v8a_CLIENT_ARTIFACTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kit/artifacts/linux-arm64-v8a-client/race-daemon/)
    set(LINUX_arm64_v8a_SERVER_ARTIFACTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kit/artifacts/linux-arm64-v8a-server/race-daemon/)

    add_custom_command(
        COMMAND ./gradlew assembleDist
        COMMAND tar -xf ${LINUX_DAEMON_APP} -C ${CMAKE_CURRENT_BINARY_DIR}/

        # Copy to app artifact dirs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LINUX_x86_64_CLIENT_ARTIFACTS_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/race-daemon-linux/ ${LINUX_x86_64_CLIENT_ARTIFACTS_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LINUX_arm64_v8a_CLIENT_ARTIFACTS_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/race-daemon-linux/ ${LINUX_arm64_v8a_CLIENT_ARTIFACTS_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LINUX_x86_64_SERVER_ARTIFACTS_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/race-daemon-linux/ ${LINUX_x86_64_SERVER_ARTIFACTS_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LINUX_arm64_v8a_SERVER_ARTIFACTS_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/race-daemon-linux/ ${LINUX_arm64_v8a_SERVER_ARTIFACTS_DIR}

        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS
            build.gradle
            race-daemon-linux/build.gradle
            ${DAEMON_LINUX_SRC}
            race-daemon-sdk/build.gradle
            ${DAEMON_SDK_SRC}
        OUTPUT
            ${LINUX_DAEMON_APP}
            ${CMAKE_CURRENT_SOURCE_DIR}/race-daemon-linux/.gradle/
            ${CMAKE_CURRENT_SOURCE_DIR}/race-daemon-linux/build/
            ${CMAKE_CURRENT_SOURCE_DIR}/race-daemon-sdk/.gradle/
            ${CMAKE_CURRENT_SOURCE_DIR}/race-daemon-sdk/build/
            ${CMAKE_CURRENT_SOURCE_DIR}/.gradle/
            ${LINUX_x86_64_CLIENT_ARTIFACTS_DIR}
            ${LINUX_x86_64_SERVER_ARTIFACTS_DIR}
            ${LINUX_arm64_v8a_CLIENT_ARTIFACTS_DIR}
            ${LINUX_arm64_v8a_SERVER_ARTIFACTS_DIR}
        COMMENT "Building Linux RACE daemon app..."
    )

    add_custom_target(RaceDaemonLinux ALL DEPENDS ${LINUX_DAEMON_APP})

    install(
        DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/race-daemon-linux/
        DESTINATION .
        USE_SOURCE_PERMISSIONS
    )
endif()

include(race/java-format)
setup_java_format_for_target(RaceNodeDaemon
    "${DAEMON_ANDROID_SRC};${DAEMON_LINUX_SRC};${DAEMON_SDK_SRC}"
)
