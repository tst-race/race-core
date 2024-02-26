
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
# Java plugin jar
################################################################################

if(ANDROID)
    set(JAR_FILE ${CMAKE_CURRENT_BINARY_DIR}/build/outputs/aar/classes.jar)
    set(DEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/build/outputs/aar/PluginCommsTwoSixJava.dex)
else()
    set(JAR_FILE ${CMAKE_CURRENT_BINARY_DIR}/target/plugin-comms-twosix-java-1.jar)
endif()
file(GLOB_RECURSE PLUGIN_SOURCES CONFIGURE_DEPENDS src/*.java)

if(TARGET RaceSdkJavaShimsJar)
    get_target_property(RACE_JAVA_SHIMS_JAR_BINARY_DIR RaceSdkJavaShimsJar BINARY_DIR)
    get_target_property(RACE_JAVA_SHIMS_BINARY_DIR     RaceJavaShims       BINARY_DIR)
    set(RACE_JAVA_SHIMS_JAR_DEPEND RaceSdkJavaShimsJar)
    set(RACE_JAVA_SHIMS_DEPEND     RaceJavaShims)
else()
    set(RACE_JAVA_SHIMS_JAR_BINARY_DIR ${CMAKE_INSTALL_PREFIX}/lib/race/java)
    set(RACE_JAVA_SHIMS_BINARY_DIR     ${CMAKE_INSTALL_PREFIX}/lib)
    set(RACE_JAVA_SHIMS_JAR_DEPEND ${RACE_JAVA_SHIMS_JAR_BINARY_DIR}/racesdk-java-shims-1.jar)
    set(RACE_JAVA_SHIMS_DEPEND     ${RACE_JAVA_SHIMS_BINARY_DIR}/libRaceJavaShims.so)
endif()

list(APPEND MVN_ARGS
    -Dsystem.java.dir=${CMAKE_INSTALL_PREFIX}/lib/race/java
    -Dracesdk.java.shims.jar=${RACE_JAVA_SHIMS_JAR_BINARY_DIR}/racesdk-java-shims-1.jar
    -Djava.library.path=${RACE_JAVA_SHIMS_BINARY_DIR}
)

if(CMAKE_CROSSCOMPILING)
    list(APPEND MVN_ARGS -DskipTests)
endif()

if(ANDROID)
    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/gradle ./gradle
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/gradlew ./gradlew
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/build.gradle ./build.gradle
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/settings.gradle ./settings.gradle
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/gradle.properties ./gradle.properties
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src ./src
        COMMAND ./gradlew --no-daemon build -x test -Drace.java.dependencies=${RACE_JAVA_SHIMS_JAR_BINARY_DIR}/
        DEPENDS ${PLUGIN_SOURCES} build.gradle ${RACE_JAVA_SHIMS_JAR_DEPEND} ${RACE_JAVA_SHIMS_DEPEND}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        OUTPUT  ${CMAKE_CURRENT_SOURCE_DIR}/build/outputs/aar/plugin-comms-twosix-java-debug.aar
    )
    add_custom_command(
        COMMAND unzip -o plugin-comms-twosix-java-debug.aar
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/build/outputs/aar/plugin-comms-twosix-java-debug.aar
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build/outputs/aar/
        OUTPUT  ${JAR_FILE}
    )
    add_custom_target(PluginCommsTwoSixJavaJar ALL DEPENDS ${JAR_FILE})
    set_property(TARGET PluginCommsTwoSixJavaJar PROPERTY ADDITIONAL_CLEAN_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/build.gradle
        ${CMAKE_CURRENT_BINARY_DIR}/gradle.properties
        ${CMAKE_CURRENT_BINARY_DIR}/settings.gradle
        ${CMAKE_CURRENT_BINARY_DIR}/src/
        ${CMAKE_CURRENT_BINARY_DIR}/build/
    )  
else()
    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/pom.xml ./pom.xml
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src ./src
        COMMAND mvn ${MVN_ARGS} compile package
        DEPENDS ${PLUGIN_SOURCES} pom.xml ${RACE_JAVA_SHIMS_JAR_DEPEND} ${RACE_JAVA_SHIMS_DEPEND}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        OUTPUT ${JAR_FILE}
    )
    add_custom_target(PluginCommsTwoSixJavaJar ALL DEPENDS ${JAR_FILE})
    set_property(TARGET PluginCommsTwoSixJavaJar PROPERTY ADDITIONAL_CLEAN_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/pom.xml
        ${CMAKE_CURRENT_BINARY_DIR}/src/
        ${CMAKE_CURRENT_BINARY_DIR}/target/
    )
endif()


if(ANDROID)
    add_custom_command(
        COMMAND /opt/android/build-tools/default/d8 --min-api ${CMAKE_SYSTEM_VERSION}  --output ${CMAKE_CURRENT_BINARY_DIR}/build/ classes.jar
        COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/build/classes.dex ${DEX_FILE}
        DEPENDS PluginCommsTwoSixJavaJar
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build/outputs/aar/
        OUTPUT  ${DEX_FILE}
    )

    add_custom_target(PluginCommsTwoSixJavaDex ALL DEPENDS ${DEX_FILE})
    install(FILES ${DEX_FILE} DESTINATION lib/race/comms/PluginCommsTwoSixJava)

    if ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_x86_64")
        add_custom_command(TARGET PluginCommsTwoSixJavaDex POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/channels/race-python-utils
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-x86_64-client/PluginCommsTwoSixJava/
            COMMAND ${CMAKE_COMMAND} -E copy ${DEX_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-x86_64-client/PluginCommsTwoSixJava/
        )
        set_property(TARGET PluginCommsTwoSixJavaDex PROPERTY ADDITIONAL_CLEAN_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-x86_64-client/
        )
    else()
        add_custom_command(TARGET PluginCommsTwoSixJavaDex POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/channels/race-python-utils
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-arm64-v8a-client/PluginCommsTwoSixJava/
            COMMAND ${CMAKE_COMMAND} -E copy ${DEX_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-arm64-v8a-client/PluginCommsTwoSixJava/
        )
        set_property(TARGET PluginCommsTwoSixJavaDex PROPERTY ADDITIONAL_CLEAN_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-arm64-v8a-client/
        )
    endif()
else()
    if ("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_arm64-v8a")
        set(NODE_TYPE linux-arm64-v8a)
    else()
        set(NODE_TYPE linux-x86_64)
    endif()

    install(FILES ${JAR_FILE} DESTINATION lib/race/comms/PluginCommsTwoSixJava)

    add_custom_command(TARGET PluginCommsTwoSixJavaJar POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/channels/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixJava/
        COMMAND ${CMAKE_COMMAND} -E copy ${JAR_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixJava/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixJava/
        COMMAND ${CMAKE_COMMAND} -E copy ${JAR_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixJava/
    )
    set_property(TARGET PluginCommsTwoSixJavaJar APPEND PROPERTY ADDITIONAL_CLEAN_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/
        ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/
    )
endif()

setup_java_format_for_target(PluginCommsTwoSixJava ${PLUGIN_SOURCES} PARENT plugin_comms_twosix_java)
