
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
# RaceTestApp
################################################################################

add_executable(racetestapp
    createPidFile.cpp
    input/RaceTestAppInputFifo.cpp
    input/RaceTestAppInputStdin.cpp
    main.cpp
    RaceLinuxApp.cpp
    output/NodeDaemonPublisher.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(racetestapp
    raceSdkCommon
    raceSdkCore
    raceTestAppShared
    crypto
    stdc++fs
    Threads::Threads
)

set_target_properties(racetestapp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    BUILD_RPATH "\${ORIGIN}/../lib"
    INSTALL_RPATH "\${ORIGIN}/../lib"
)

# TODO enable performance-* and readability-* checks (needs fixes)
setup_clang_tidy_for_target(TARGET racetestapp)
setup_cppcheck_for_target(
    TARGET racetestapp
    CHECKS all
    SUPPRESS unmatchedSuppression unusedFunction missingIncludeSystem useStlAlgorithm
)
setup_clang_format_for_target(racetestapp PARENT racetestapp_linux)

################################################################################
# Install
################################################################################

install(
    TARGETS racetestapp
    DESTINATION bin)


###
# SDK libraries
###
if ("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_arm64-v8a")
    set(NODE_TYPE linux-arm64-v8a)
else()
    set(NODE_TYPE linux-x86_64)
endif()

if(TARGET raceSdkCommon)
    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:raceSdkCommon> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:raceSdkCommon> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:raceSdkStorageEncryption> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:raceSdkStorageEncryption> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:raceSdkCore> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:raceSdkCore> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:raceTestAppShared> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:raceTestAppShared> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:RaceJavaShims> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:RaceJavaShims> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:commsPluginBindingsGolang> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:commsPluginBindingsGolang> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        OUTPUT
            ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
            ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        DEPENDS
            raceSdkCommon
            raceSdkStorageEncryption
            raceSdkCore
            raceTestAppShared
            RaceJavaShims
            commsPluginBindingsGolang
            ${RACE_JAVA_SHIMS_JAR}

        COMMENT "Copying SDK artifacts..."
    )

    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/bin/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/bin/

        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:racesecli> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/bin/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:racesecli> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/bin/

        OUTPUT
            ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/bin/racesecli
            ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/bin/racesecli

        DEPENDS
            racesecli

        COMMENT "Copying racesecli"
    )
else()
    find_library(LIB_SDK_COMMON raceSdkCommon REQUIRED)
    find_library(LIB_SDK_STORAGE_ENCRYPTION raceSdkStorageEncryption REQUIRED)
    find_library(LIB_SDK_CORE raceSdkCore REQUIRED)
    find_library(LIB_TEST_APP_SHARED raceTestAppShared REQUIRED)
    find_library(LIB_JAVA_SHIMS RaceJavaShims REQUIRED)
    find_library(LIB_COMMS_BINDINGS_GOLANG commsPluginBindingsGolang.so REQUIRED)
    find_program(BIN_RACE_SE_CLI racesecli REQUIRED)

    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_SDK_COMMON} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_SDK_COMMON} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_SDK_STORAGE_ENCRYPTION} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_SDK_STORAGE_ENCRYPTION} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_SDK_CORE} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_SDK_CORE} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_TEST_APP_SHARED} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_TEST_APP_SHARED} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_JAVA_SHIMS} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_JAVA_SHIMS} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_COMMS_BINDINGS_GOLANG} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        COMMAND ${CMAKE_COMMAND} -E copy ${LIB_COMMS_BINDINGS_GOLANG} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        OUTPUT
            ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
            ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/

        DEPENDS
            ${LIB_SDK_COMMON}
            ${LIB_SDK_STORAGE_ENCRYPTION}
            ${LIB_SDK_CORE}
            ${LIB_TEST_APP_SHARED}
            ${LIB_JAVA_SHIMS}
            ${LIB_COMMS_BINDINGS_GOLANG}

        COMMENT "Copying SDK artifacts..."
    )

    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/bin/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/bin/

        COMMAND ${CMAKE_COMMAND} -E copy ${BIN_RACE_SE_CLI} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${BIN_RACE_SE_CLI} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/bin/

        OUTPUT
            ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/bin/racesecli
            ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/bin/racesecli

        DEPENDS
            ${BIN_RACE_SE_CLI}

        COMMENT "Copying racesecli"
    )
endif()


###
# Java Shim .jar
###

if (TARGET RaceSdkJavaShimsJar)
    get_target_property(RACE_JAVA_SHIMS_JAR_BINARY_DIR RaceSdkJavaShimsJar BINARY_DIR)
else()
    set(RACE_JAVA_SHIMS_JAR_BINARY_DIR ${CMAKE_INSTALL_PREFIX}/lib/race/java)
endif()
set(RACE_JAVA_SHIMS_JAR ${RACE_JAVA_SHIMS_JAR_BINARY_DIR}/racesdk-java-shims-1.jar)

add_custom_command(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/java
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/java

    COMMAND ${CMAKE_COMMAND} -E copy ${RACE_JAVA_SHIMS_JAR} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/java
    COMMAND ${CMAKE_COMMAND} -E copy ${RACE_JAVA_SHIMS_JAR} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/java

    OUTPUT
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/java/
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/java/

    DEPENDS
        ${RACE_JAVA_SHIMS_JAR}

    COMMENT "Copying Java shims jar..."
)

###
# Python Language Bindings
###

if (TARGET networkManagerPluginBindings)
    get_target_property(RACE_NETWORK_MANAGER_PYTHON_SHIMS_BINARY_DIR networkManagerPluginBindings BINARY_DIR)
    set(RACE_NETWORK_MANAGER_PYTHON_SHIMS_INCLUDE_DIR ${RACE_NETWORK_MANAGER_PYTHON_SHIMS_BINARY_DIR}/include)
else()
    set(RACE_NETWORK_MANAGER_PYTHON_SHIMS_BINARY_DIR ${CMAKE_INSTALL_PREFIX}/lib/race/python)
    set(RACE_NETWORK_MANAGER_PYTHON_SHIMS_INCLUDE_DIR ${RACE_NETWORK_MANAGER_PYTHON_SHIMS_BINARY_DIR})
endif()
set(RACE_NETWORK_MANAGER_PYTHON_SHIMS ${RACE_NETWORK_MANAGER_PYTHON_SHIMS_BINARY_DIR}/_networkManagerPluginBindings.so ${RACE_NETWORK_MANAGER_PYTHON_SHIMS_INCLUDE_DIR}/networkManagerPluginBindings.py)
set(RACE_NETWORK_MANAGER_COPY_PYTHON_COMMAND COMMAND cp -f ${RACE_NETWORK_MANAGER_PYTHON_SHIMS} app/src/main/assets/race/python/)

if (TARGET commsPluginBindings)
    get_target_property(RACE_COMMS_PYTHON_SHIMS_BINARY_DIR commsPluginBindings BINARY_DIR)
    set(RACE_COMMS_PYTHON_SHIMS_INCLUDE_DIR ${RACE_COMMS_PYTHON_SHIMS_BINARY_DIR}/include)
else()
    set(RACE_COMMS_PYTHON_SHIMS_BINARY_DIR ${CMAKE_INSTALL_PREFIX}/lib/race/python)
    set(RACE_COMMS_PYTHON_SHIMS_INCLUDE_DIR ${RACE_COMMS_PYTHON_SHIMS_BINARY_DIR})
endif()
set(RACE_COMMS_PYTHON_SHIMS ${RACE_COMMS_PYTHON_SHIMS_BINARY_DIR}/_commsPluginBindings.so ${RACE_COMMS_PYTHON_SHIMS_INCLUDE_DIR}/commsPluginBindings.py)
set(RACE_COMMS_COPY_PYTHON_COMMAND COMMAND cp -f ${RACE_COMMS_PYTHON_SHIMS} app/src/main/assets/race/python/)

add_custom_command(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/python
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/python

    COMMAND ${CMAKE_COMMAND} -E copy ${RACE_NETWORK_MANAGER_PYTHON_SHIMS} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/python
    COMMAND ${CMAKE_COMMAND} -E copy ${RACE_NETWORK_MANAGER_PYTHON_SHIMS} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/python

    COMMAND ${CMAKE_COMMAND} -E copy ${RACE_COMMS_PYTHON_SHIMS} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/python
    COMMAND ${CMAKE_COMMAND} -E copy ${RACE_COMMS_PYTHON_SHIMS} ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/python

    OUTPUT
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/python
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/python

    DEPENDS
        ${RACE_NETWORK_MANAGER_PYTHON_SHIMS}
        ${RACE_COMMS_PYTHON_SHIMS}

    COMMENT "Copying Python shims..."
)

###
# RACE Test App Executable
###

add_custom_command(TARGET racetestapp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/bin/
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/bin/

    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:racetestapp> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/bin/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:racetestapp> ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/bin/
)

add_custom_command(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/bin/
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/bin/

    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/install.sh ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/install.sh ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/

    OUTPUT
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/install.sh
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/install.sh

    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/install.sh

    COMMENT "Copying install.sh..."
)

###
# App artifacts target
###

add_custom_target(RaceTestAppArtifacts ALL
    DEPENDS
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/bin/racesecli
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/bin/racesecli
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/java/
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/java/
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/lib/python
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/lib/python
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/race/install.sh
        ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/race/install.sh
)

set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/
    ${PROJECT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/
)
