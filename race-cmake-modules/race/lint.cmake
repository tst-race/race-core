
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

option(ENABLE_CODE_LINTING "Enable code linting (clang-tidy, cppcheck)" ON)
# Currently disabled for Android on the assumption that the findings are the same
# for Linux and Android, and running this on both is just redundant. If substantial
# use of preprocessor-driven conditional logic is ever added to the code, then we
# may want to remove this and run linting by default for Android too.
if(ANDROID)
    set(ENABLE_CODE_LINTING OFF)
endif()

function(setup_clang_tidy_for_target)
    set(OPTIONS "")
    set(ONE_VALUE_ARGS TARGET)
    set(MULTI_VALUE_ARGS CHECKS)
    cmake_parse_arguments(CLANG_TIDY "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    if(ENABLE_CODE_LINTING)
        if(NOT DEFINED CLANG_TIDY_CHECKS)
            set(CLANG_TIDY_CHECKS "bugprone-*;clang-analyzer-*")
        endif()
        string(REPLACE ";" "," CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS}")

        set(CLANG_TIDY_CMD
            clang-tidy-10
            -checks=${CLANG_TIDY_CHECKS}
            -warnings-as-errors=*
            -header-filter=.*
        )
        set_target_properties(${CLANG_TIDY_TARGET} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_CMD}")
    endif()
endfunction()

function(setup_cppcheck_for_target)
    set(OPTIONS "")
    set(ONE_VALUE_ARGS TARGET)
    set(MULTI_VALUE_ARGS CHECKS SUPPRESS)
    cmake_parse_arguments(CPPCHECK "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    if(ENABLE_CODE_LINTING)
        if(NOT DEFINED CPPCHECK_CHECKS)
            set(CPPCHECK_CHECKS "all")
        endif()
        string(REPLACE ";" "," CPPCHECK_CHECKS "${CPPCHECK_CHECKS}")

        set(CPPCHECK_CMD
            cppcheck -q --force --enable=${CPPCHECK_CHECKS}
            --language=c++ --std=c++17 --inline-suppr
            --error-exitcode=22 -D__unix__ -D__aarch64__
        )
        if(DEFINED CPPCHECK_SUPPRESS)
            foreach(SUPPRESS ${CPPCHECK_SUPPRESS})
                list(APPEND CPPCHECK_CMD "--suppress=${SUPPRESS}")
            endforeach()
        endif()
        set_target_properties(${CPPCHECK_TARGET} PROPERTIES CXX_CPPCHECK "${CPPCHECK_CMD}")
    endif()
endfunction()
