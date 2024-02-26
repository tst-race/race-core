
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

include(race/local-targets)

function(setup_java_format_for_target TARGET_NAME TARGET_SOURCES)
    cmake_parse_arguments(JAVA_FORMAT "ADD_TO_LOCAL_TARGET" "PARENT" "" ${ARGN})

    # It should be noted that there are Maven plugins to run this, *but* google-java-format
    # requires Java 11 and Maven is running under Java 8
    set(GOOGLE_JAVA_FORMAT_JAR google-java-format-1.8-all-deps.jar)
    add_custom_command(
        COMMAND wget -q https://github.com/google/google-java-format/releases/download/google-java-format-1.8/${GOOGLE_JAVA_FORMAT_JAR}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        OUTPUT ${CMAKE_BINARY_DIR}/${GOOGLE_JAVA_FORMAT_JAR}
        COMMENT "Downloading google-java-format"
    )

    if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
        set(JAVA_11_TOOL /usr/lib/jvm/java-11-openjdk-arm64/bin/java)
    else()
        set(JAVA_11_TOOL /usr/lib/jvm/java-11-openjdk-amd64/bin/java)
    endif()
    add_custom_target(format_java_${TARGET_NAME}
        COMMAND ${JAVA_11_TOOL} -jar ${CMAKE_BINARY_DIR}/${GOOGLE_JAVA_FORMAT_JAR}
            --aosp --replace ${TARGET_SOURCES}
        DEPENDS ${CMAKE_BINARY_DIR}/${GOOGLE_JAVA_FORMAT_JAR}
        COMMENT "Formatting ${TARGET_NAME} files..."
    )
    if (${JAVA_FORMAT_ADD_TO_LOCAL_TARGET})
        add_dependencies(format format_java_${TARGET_NAME})
    else()
        add_local_target(format DEPENDS format_java_${TARGET_NAME})
    endif()

    add_custom_target(check_format_java_${TARGET_NAME}
        COMMAND ${JAVA_11_TOOL} -jar ${CMAKE_BINARY_DIR}/${GOOGLE_JAVA_FORMAT_JAR}
            --aosp --dry-run --set-exit-if-changed ${TARGET_SOURCES}
        DEPENDS ${CMAKE_BINARY_DIR}/${GOOGLE_JAVA_FORMAT_JAR}
        COMMENT "Checking format for ${TARGET_NAME} files..."
    )
    if (${JAVA_FORMAT_ADD_TO_LOCAL_TARGET})
        add_dependencies(check_format check_format_java_${TARGET_NAME})
    else()
        add_local_target(check_format DEPENDS check_format_java_${TARGET_NAME})
    endif()

    if (JAVA_FORMAT_PARENT)
        add_dependencies(format_${JAVA_FORMAT_PARENT} format_java_${TARGET_NAME})
        add_dependencies(check_format_${JAVA_FORMAT_PARENT} check_format_java_${TARGET_NAME})
    endif()
endfunction()
