
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
# Java Shims Jar
################################################################################

file(GLOB_RECURSE JAVA_SOURCES CONFIGURE_DEPENDS src/*.java)
set(JAVA_DEPENDS pom.xml ${JAVA_SOURCES})
set(JAVA_SHIMS_JAR ${CMAKE_CURRENT_BINARY_DIR}/racesdk-java-shims-1.jar)

add_custom_command(
    COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/* .
    COMMAND mvn compile package
    COMMAND cp target/racesdk-java-shims-1.jar ${JAVA_SHIMS_JAR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    OUTPUT ${JAVA_SHIMS_JAR}
    DEPENDS ${JAVA_DEPENDS}
    COMMENT "Generating Java Shims Jar..."
)

add_custom_target(RaceSdkJavaShimsJar ALL DEPENDS ${JAVA_SHIMS_JAR})

install(
    FILES ${JAVA_SHIMS_JAR}
    DESTINATION lib/race/java
    COMPONENT sdk
)

################################################################################
# Java Shims JNI Headers
################################################################################

set(JNI_SOURCES
        src/main/java/ShimsJava/AppConfig.java
        src/main/java/ShimsJava/Helpers.java
        src/main/java/ShimsJava/JRaceSdkComms.java
        src/main/java/ShimsJava/JRaceSdkNM.java
        src/main/java/ShimsJava/RaceApp.java
        src/main/java/ShimsJava/RaceLog.java
        src/main/java/ShimsJava/RaceSdkApp.java
        src/main/java/ShimsJava/RaceTestApp.java
)

set(JNI_CLASSES
        ShimsJava.AppConfig
        ShimsJava.Helpers
        ShimsJava.JRaceSdkComms
        ShimsJava.JRaceSdkNM
        ShimsJava.RaceApp
        ShimsJava.RaceLog
        ShimsJava.RaceSdkApp
        ShimsJava.RaceTestApp
)

set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(GENERATED_HEADERS
        ${GENERATED_HEADERS_DIR}/ShimsJava_AppConfig.h
        ${GENERATED_HEADERS_DIR}/ShimsJava_Helpers.h
        ${GENERATED_HEADERS_DIR}/ShimsJava_JRaceSdkNM.h
        ${GENERATED_HEADERS_DIR}/ShimsJava_JRaceSdkComms.h
        ${GENERATED_HEADERS_DIR}/ShimsJava_RaceApp.h
        ${GENERATED_HEADERS_DIR}/ShimsJava_RaceLog.h
        ${GENERATED_HEADERS_DIR}/ShimsJava_RaceSdkApp.h
        ${GENERATED_HEADERS_DIR}/ShimsJava_RaceTestApp.h
)

add_custom_command(
    COMMAND javah -jni -classpath ${CMAKE_CURRENT_BINARY_DIR}/target/classes -d ${GENERATED_HEADERS_DIR}/ ${JNI_CLASSES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT ${GENERATED_HEADERS}
    DEPENDS ${JNI_SOURCES}
    COMMENT "Generating Java Shims JNI Headers..."
)

# Need this directory to exist at the time we define the imported target,
# even though it won't be populated until a build occurs
file(MAKE_DIRECTORY ${GENERATED_HEADERS_DIR})

add_custom_target(generate_java_shims_jni_headers ALL
    DEPENDS ${GENERATED_HEADERS} RaceSdkJavaShimsJar
)

add_library(RaceSdkJavaShimsJniHeaders INTERFACE IMPORTED GLOBAL)
set_target_properties(RaceSdkJavaShimsJniHeaders PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${GENERATED_HEADERS_DIR}
)
add_dependencies(RaceSdkJavaShimsJniHeaders generate_java_shims_jni_headers)

set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/target/
    ${GENERATED_HEADERS_DIR}/
)

################################################################################
# Formatting
################################################################################

setup_java_format_for_target(RaceSdkJavaShimsJar "${JAVA_SOURCES}" PARENT racesdk)
