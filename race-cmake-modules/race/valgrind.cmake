
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

function(setup_valgrind_for_target TARGET_NAME)
    if (${CMAKE_HOST_SYSTEM} MATCHES "Darwin")
        return()
    endif()

    find_program(VALGRIND_TOOL valgrind REQUIRED)

    set(OPTIONS EXCLUDE_FROM_ALL)
    set(ONE_VALUE_ARGS "")
    set(MULTI_VALUE_ARGS DEPENDS EXECUTABLE_ARGS)
    cmake_parse_arguments(TEST "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    set(VALGRIND_OUT_FILE ${TARGET_NAME}.valgrind.txt)
    set(VALGRIND_CMD
        ${VALGRIND_TOOL}
        --leak-check=full
        --show-leak-kinds=all
        --track-origins=yes
        --verbose
        --log-file=${VALGRIND_OUT_FILE}
        --error-exitcode=1
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME} ${TEST_EXECUTABLE_ARGS}
    )

    add_custom_target(valgrind_${TARGET_NAME}
        COMMAND echo "=================== valgrind ===================="
        COMMAND echo ${VALGRIND_CMD}
        COMMAND ${VALGRIND_CMD}

        BYPRODUCTS ${VALGRIND_OUT_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${TARGET_NAME} ${TEST_DEPENDS}
        VERBATIM
        COMMENT "Running test through valgrind"
    )
    if(NOT ${TEST_EXCLUDE_FROM_ALL})
        add_local_target(valgrind DEPENDS valgrind_${TARGET_NAME})
    endif()
endfunction(setup_valgrind_for_target)
