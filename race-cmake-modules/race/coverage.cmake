
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

option(ENABLE_CODE_COVERAGE "Enable code coverage instrumentation and collection" OFF)

if(ENABLE_CODE_COVERAGE)
    message("-- Enabling code coverage instrumentation")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -fprofile-arcs -ftest-coverage")

    find_program(LCOV_TOOL lcov REQUIRED)
    find_program(GENHTML_TOOL genhtml REQUIRED)

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        find_program(GCOV_TOOL gcov REQUIRED)
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(GCOV_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/scripts)
        foreach(MODULE_PATH ${CMAKE_MODULE_PATH})
            list(APPEND GCOV_SEARCH_PATHS ${MODULE_PATH}/scripts)
        endforeach()
        find_program(GCOV_TOOL llvm-gcov.sh PATHS ${GCOV_SEARCH_PATHS} REQUIRED)
    else()
        message(FATAL_ERROR "Unrecognized compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()

    function(setup_coverage_for_target)
        set(OPTIONS "")
        set(ONE_VALUE_ARGS BASEDIR EXECUTABLE PROJECT SOURCE_DIRECTORY TARGET)
        set(MULTI_VALUE_ARGS DEPENDS EXCLUDE EXECUTABLE_ARGS)
        cmake_parse_arguments(COV "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

        if(NOT DEFINED COV_BASEDIR)
            set(COV_BASEDIR "${PROJECT_SOURCE_DIR}")
        endif()

        if(NOT DEFINED COV_EXECUTABLE)
            set(COV_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/${COV_TARGET}")
        endif()

        if(NOT DEFINED COV_PROJECT)
            set(COV_PROJECT "${PROJECT_NAME}")
        endif()

        if(NOT DEFINED COV_SOURCE_DIRECTORY)
            set(COV_SOURCE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
        endif()

        set(LCOV_EXCLUDES "*/build/*" "*/test/*")
        foreach(EXCLUDE ${COV_EXCLUDE})
            get_filename_component(EXCLUDE ${EXCLUDE} ABSOLUTE BASE_DIR ${COV_BASEDIR})
            list(APPEND LCOV_EXCLUDES "${EXCLUDE}")
        endforeach()

        set(LCOV_CLEAN_CMD
            ${LCOV_TOOL} --gcov-tool ${GCOV_TOOL}
            --directory ${COV_SOURCE_DIRECTORY}
            --base-directory ${COV_BASEDIR}
            --zerocounters
        )
        set(LCOV_BASELINE_CMD
            ${LCOV_TOOL} --gcov-tool ${GCOV_TOOL}
            --directory ${COV_SOURCE_DIRECTORY}
            --base-directory ${COV_BASEDIR}
            --capture --initial
            --no-external
            --output-file ${COV_TARGET}.baseline
        )
        set(LCOV_EXEC_TESTS_CMD
            ${COV_EXECUTABLE} ${COV_EXECUTABLE_ARGS}
        )
        set(LCOV_CAPTURE_CMD
            ${LCOV_TOOL} --gcov-tool ${GCOV_TOOL}
            --directory ${COV_SOURCE_DIRECTORY}
            --base-directory ${COV_BASEDIR}
            --capture --no-external
            --output-file ${COV_TARGET}.capture
        )
        set(LCOV_BASELINE_COUNT_CMD
            ${LCOV_TOOL} --gcov-tool ${GCOV_TOOL}
            --add-tracefile ${COV_TARGET}.baseline
            --add-tracefile ${COV_TARGET}.capture
            --output-file ${COV_TARGET}.combined
        )
        set(LCOV_FILTER_CMD
            ${LCOV_TOOL} --gcov-tool ${GCOV_TOOL}
            --remove ${COV_TARGET}.combined ${LCOV_EXCLUDES}
            --output-file ${COV_TARGET}.info
        )
        set(LCOV_GEN_HTML_CMD
            ${GENHTML_TOOL}
            --prefix ${COV_BASEDIR}
            --output-directory coverage
            ${COV_TARGET}.info
        )

        message("-- Creating target lcov_${COV_TARGET}")
        add_custom_target(lcov_${COV_TARGET}
            COMMAND echo "=================== LCOV ${COV_TARGET} ===================="
            COMMAND ${LCOV_CLEAN_CMD}
            COMMAND ${LCOV_BASELINE_CMD}
            COMMAND ${LCOV_EXEC_TESTS_CMD}
            COMMAND ${LCOV_CAPTURE_CMD}
            COMMAND ${LCOV_BASELINE_COUNT_CMD}
            COMMAND ${LCOV_FILTER_CMD}
            COMMAND ${LCOV_GEN_HTML_CMD}

            BYPRODUCTS
                ${COV_TARGET}.baseline
                ${COV_TARGET}.capture
                ${COV_TARGET}.combined
                ${COV_TARGET}.info
                ${COV_TARGET}/index.html
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${COV_TARGET} ${COV_DEPENDS}
            VERBATIM
            COMMENT "Capturing code coverage and generating coverage report"
        )
        add_local_target(lcov lcov_${COV_TARGET})

        list(APPEND ${COV_PROJECT}_LCOV_TARGETS lcov_${COV_TARGET})
        list(REMOVE_DUPLICATES ${COV_PROJECT}_LCOV_TARGETS)
        set(${COV_PROJECT}_LCOV_TARGETS "${${COV_PROJECT}_LCOV_TARGETS}" CACHE INTERNAL "LCOV targets")

        list(APPEND ${COV_PROJECT}_COV_INFO_FILES ${CMAKE_CURRENT_BINARY_DIR}/${COV_TARGET}.info)
        list(REMOVE_DUPLICATES ${COV_PROJECT}_COV_INFO_FILES)
        set(${COV_PROJECT}_COV_INFO_FILES "${${COV_PROJECT}_COV_INFO_FILES}" CACHE INTERNAL "Coverage info files")
    endfunction(setup_coverage_for_target)

    function(setup_project_coverage_target)
        set(OPTIONS "")
        set(ONE_VALUE_ARGS BASEDIR PROJECT SOURCE_DIRECTORY TARGET)
        set(MULTI_VALUE_ARGS DEPENDS EXCLUDE EXECUTABLE_ARGS)
        cmake_parse_arguments(COV "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

        if(NOT DEFINED COV_BASEDIR)
            set(COV_BASEDIR "${PROJECT_SOURCE_DIR}")
        endif()

        if(NOT DEFINED COV_PROJECT)
            set(COV_PROJECT "${PROJECT_NAME}")
        endif()

        if(NOT DEFINED ${COV_PROJECT}_COV_INFO_FILES)
            message(FATAL_ERROR "No targets have been setup for coverage collection, the project coverage target must be defined after all other coverage targets have been setup")
        endif()

        set(LCOV_COMBINE_CMD
            ${LCOV_TOOL} --gcov-tool ${GCOV_TOOL}
            --output-file coverage.info
        )
        foreach(COV_INFO_FILE ${${COV_PROJECT}_COV_INFO_FILES})
            list(APPEND LCOV_COMBINE_CMD "--add-tracefile" "${COV_INFO_FILE}")
        endforeach()

        set(LCOV_GEN_HTML_CMD
            ${GENHTML_TOOL}
            --prefix ${COV_BASEDIR}
            --output-directory coverage
            coverage.info
        )

        # Create custom target to combine test coverage
        add_custom_target(coverage_${COV_PROJECT}
            COMMAND ${LCOV_COMBINE_CMD}
            COMMAND ${LCOV_GEN_HTML_CMD}

            DEPENDS ${${COV_PROJECT}_LCOV_TARGETS}
            BYPRODUCTS
                coverage.info
                coverage/index.html
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            VERBATIM
            COMMENT "Combining code coverage and generating coverage report"
        )
        add_local_target(coverage DEPENDS coverage_${COV_PROJECT})
    endfunction(setup_project_coverage_target)

else()
    function(setup_coverage_for_target)
        # Do nothing
    endfunction(setup_coverage_for_target)

    function(setup_project_coverage_target)
        # Do nothing
    endfunction(setup_project_coverage_target)

endif()