
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

if (${CMAKE_HOST_SYSTEM} MATCHES "Darwin")
    find_program(CLANG_FORMAT_TOOL clang-format REQUIRED)
else()
    find_program(CLANG_FORMAT_TOOL clang-format-10 REQUIRED)
endif()

function(setup_clang_format_for_target TARGET_NAME)
    get_target_property(TARGET_PUBLIC_HEADERS ${TARGET_NAME} PUBLIC_HEADER)
    get_target_property(TARGET_PRIVATE_HEADERS ${TARGET_NAME} PRIVATE_HEADER)
    get_target_property(TARGET_SOURCES ${TARGET_NAME} SOURCES)

    if(TARGET_PUBLIC_HEADERS)
        foreach(header ${TARGET_PUBLIC_HEADERS})
            get_filename_component(header ${header} ABSOLUTE)
            list(APPEND FORMAT_FILES ${header})
        endforeach()
    endif()

    if(TARGET_PRIVATE_HEADERS)
        foreach(header ${TARGET_PRIVATE_HEADERS})
            get_filename_component(header ${header} ABSOLUTE)
            list(APPEND FORMAT_FILES ${header})
        endforeach()
    endif()

    if(TARGET_SOURCES)
        foreach(source ${TARGET_SOURCES})
            get_filename_component(source ${source} ABSOLUTE)
            list(APPEND FORMAT_FILES ${source})
        endforeach()
    endif()

    setup_clang_format_for_files(
        NAME ${TARGET_NAME}
        FILES ${FORMAT_FILES}
        ${ARGN}
    )
endfunction()

function(setup_clang_format_for_files)
    cmake_parse_arguments(FORMAT "ADD_TO_LOCAL_TARGET" "NAME;PARENT" "FILES" ${ARGN})

    add_custom_target(format_${FORMAT_NAME}
        COMMAND ${CLANG_FORMAT_TOOL} -style=file -i ${FORMAT_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Formatting ${FORMAT_NAME} files..."
    )
    if (${FORMAT_ADD_TO_LOCAL_TARGET})
        add_dependencies(format format_${FORMAT_NAME})
    else()
        add_local_target(format DEPENDS format_${FORMAT_NAME})
    endif()

    add_custom_target(check_format_${FORMAT_NAME}
        COMMAND ${CLANG_FORMAT_TOOL} -style=file --dry-run -Werror ${FORMAT_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Checking format for ${FORMAT_NAME} files..."
    )
    if (${FORMAT_ADD_TO_LOCAL_TARGET})
        add_dependencies(check_format check_format_${FORMAT_NAME})
    else()
        add_local_target(check_format DEPENDS check_format_${FORMAT_NAME})
    endif()

    if(FORMAT_PARENT)
        add_dependencies(format_${FORMAT_PARENT} format_${FORMAT_NAME})
        add_dependencies(check_format_${FORMAT_PARENT} check_format_${FORMAT_NAME})
    endif()
endfunction()

function(setup_project_format_targets project_name)
    add_custom_target(format_${project_name})
    add_local_target(format DEPENDS format_${project_name})

    add_custom_target(check_format_${project_name})
    add_local_target(check_format DEPENDS check_format_${project_name})
endfunction()
