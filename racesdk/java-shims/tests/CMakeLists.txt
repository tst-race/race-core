
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
# RaceSdkJavaStubPlugin
################################################################################

set(STUB_PLUGIN_SOURCES
    stub/pom.xml
    stub/src/main/java/com/twosix/race/StubLibraryLoader.java
    stub/src/main/java/com/twosix/race/StubCommsPlugin.java
    stub/src/main/java/com/twosix/race/StubNMPlugin.java
)

set(STUB_PLUGIN_JAR stub-plugins-1.jar)

get_target_property(RACE_JAVA_SHIMS_JAR_BINARY_DIR RaceSdkJavaShimsJar BINARY_DIR)
set(MVN_ARGS -Dracesdk.java.shims.jar=${RACE_JAVA_SHIMS_JAR_BINARY_DIR}/racesdk-java-shims-1.jar)

add_custom_command(
    COMMAND mkdir -p ./stub
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/stub/pom.xml ./stub/
    COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/stub/src ./stub/
    COMMAND cd stub && mvn ${MVN_ARGS} compile package
    COMMAND cp stub/target/${STUB_PLUGIN_JAR} ${CMAKE_CURRENT_BINARY_DIR}/${STUB_PLUGIN_JAR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/${STUB_PLUGIN_JAR}
        stub/
    DEPENDS ${STUB_PLUGIN_SOURCES}
    COMMENT "Generating Stub Plugin Jar..."
)

add_custom_target(RaceSdkJavaStubPlugin ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${STUB_PLUGIN_JAR} RaceSdkJavaShimsJar)

################################################################################
# Copy RaceJavaShimsJar
################################################################################

add_custom_command(
    COMMAND cp ${RACE_JAVA_SHIMS_JAR_BINARY_DIR}/racesdk-java-shims-1.jar ${CMAKE_CURRENT_BINARY_DIR}/racesdk-java-shims-1.jar
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/racesdk-java-shims-1.jar
    DEPENDS
        RaceSdkJavaShimsJar
        ${RACE_JAVA_SHIMS_JAR_BINARY_DIR}/racesdk-java-shims-1.jar
    COMMENT "Copying Java Shims Jar..."
)

add_custom_target(CopyRaceJavaShimsJar ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/racesdk-java-shims-1.jar)

################################################################################
# Copy RaceJavaShims
################################################################################

add_custom_command(
    COMMAND cp $<TARGET_FILE:RaceJavaShims> ${CMAKE_CURRENT_BINARY_DIR}/libRaceJavaShims.so
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libRaceJavaShims.so
    DEPENDS RaceJavaShims
    COMMENT "Copying RaceJavaShims library..."
)

add_custom_target(CopyRaceJavaShimsLib ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libRaceJavaShims.so)

################################################################################
# unitTestRaceJavaShims
################################################################################

# Add the source to the test executable
add_executable(unitTestRaceJavaShims
    main.cpp
    JRaceSdkNMTest.cpp
    JRaceSdkCommsTest.cpp
    NMPluginTest.cpp
    CommsPluginTest.cpp
    TestShimUtils.cpp
)

# Link external dependencies
find_package(GTest REQUIRED CONFIG)
find_package(Threads REQUIRED)

target_link_libraries(unitTestRaceJavaShims
    GTest::gmock
    Threads::Threads
    RaceJavaShims
    raceSdkTestMocks
)
add_dependencies(unitTestRaceJavaShims
    RaceSdkJavaStubPlugin CopyRaceJavaShimsLib CopyRaceJavaShimsJar)

add_dependencies(build_racesdk_tests unitTestRaceJavaShims)
add_test(java-shims ${CMAKE_CURRENT_BINARY_DIR}/unitTestRaceJavaShims)
set_tests_properties(java-shims PROPERTIES LABELS "unit;racesdk")
setup_coverage_for_target(
    TARGET unitTestRaceJavaShims
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
    EXCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/*"
)
setup_valgrind_for_target(unitTestRaceJavaShims
    # Temporarily omitted because of pre-existing errors
    EXCLUDE_FROM_ALL
)

# Setup style validation
setup_clang_format_for_target(unitTestRaceJavaShims PARENT racesdk)

file(GLOB_RECURSE STUB_PLUGIN_JAVA_SOURCES CONFIGURE_DEPENDS stub/src/*.java)
setup_java_format_for_target(unitTestRaceJavaShims "${STUB_PLUGIN_JAVA_SOURCES}"
    ADD_TO_LOCAL_TARGET PARENT racesdk)
