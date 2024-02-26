
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

link_directories("/usr/local/lib/race/comms/")

add_executable(unitTestPluginCommsJava
    main.cpp
)
target_compile_definitions(unitTestPluginCommsJava PRIVATE
    TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/"
)

find_package(GTest REQUIRED CONFIG)
find_package(Threads REQUIRED)

target_link_libraries(unitTestPluginCommsJava
    PluginCommsTwoSixJava
    raceSdkCommon
    GTest::gmock
    GTest::gtest
    Threads::Threads
    dl
)

if (TARGET raceSdkTestMocks)
    target_link_libraries(unitTestPluginCommsJava raceSdkTestMocks)
endif()

get_target_property(PLUGIN_BINARY_DIR PluginCommsTwoSixJavaJar BINARY_DIR)
add_custom_command(
    COMMAND cp ${PLUGIN_BINARY_DIR}/target/plugin-comms-twosix-java-1.jar ${CMAKE_CURRENT_BINARY_DIR}/plugin-comms-twosix-java-1.jar
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/plugin-comms-twosix-java-1.jar
    DEPENDS PluginCommsTwoSixJavaJar
    COMMENT "Copying plugin jar..."
)
add_custom_target(CopyPluginJarForCommsJavaTest ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/plugin-comms-twosix-java-1.jar)
add_dependencies(unitTestPluginCommsJava CopyPluginJarForCommsJavaTest)


if (TARGET RaceSdkJavaShimsJar)
    get_target_property(RACE_JAVA_SHIMS_JAR_BINARY_DIR RaceSdkJavaShimsJar BINARY_DIR)
    add_custom_command(
        COMMAND cp ${RACE_JAVA_SHIMS_JAR_BINARY_DIR}/racesdk-java-shims-1.jar ${CMAKE_CURRENT_BINARY_DIR}/racesdk-java-shims-1.jar
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/racesdk-java-shims-1.jar
        DEPENDS
            RaceSdkJavaShimsJar
            ${RACE_JAVA_SHIMS_JAR_BINARY_DIR}/racesdk-java-shims-1.jar
        COMMENT "Copying Java Shims Jar..."
    )
    add_custom_target(CopyRaceJavaShimsJarForCommsJavaTest ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/racesdk-java-shims-1.jar)
    add_dependencies(unitTestPluginCommsJava CopyRaceJavaShimsJarForCommsJavaTest)
endif()


if (TARGET RaceJavaShims)
    add_custom_command(
        COMMAND cp $<TARGET_FILE:RaceJavaShims> ${CMAKE_CURRENT_BINARY_DIR}/libRaceJavaShims.so
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libRaceJavaShims.so
        DEPENDS RaceJavaShims
        COMMENT "Copying RaceJavaShims library..."
    )
    add_custom_target(CopyRaceJavaShimsLibForCommsJavaTest ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libRaceJavaShims.so)
    add_dependencies(unitTestPluginCommsJava CopyRaceJavaShimsLibForCommsJavaTest)
endif()

add_dependencies(build_plugin_comms_twosix_java_tests unitTestPluginCommsJava)
add_test(plugin_comms_twosix_java ${CMAKE_CURRENT_BINARY_DIR}/unitTestPluginCommsJava)
set_tests_properties(plugin_comms_twosix_java PROPERTIES LABELS "unit;plugin_comms_twosix_java")
setup_coverage_for_target(
    TARGET unitTestPluginCommsJava
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../..
)
setup_valgrind_for_target(unitTestPluginCommsJava)
setup_clang_format_for_target(unitTestPluginCommsJava PARENT plugin_comms_twosix_java)
