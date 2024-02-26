
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

enable_testing()

function(setup_project_test_targets project_name)
    cmake_parse_arguments(TEST "UNIT;INTEGRATION" "" "" ${ARGN})

    # Create custom build-tests target for the project
    add_custom_target(build_${project_name}_tests)
    add_local_target(build_tests DEPENDS build_${project_name}_tests)

    # Create custom run-tests target for the project
    add_custom_target(run_${project_name}_tests
        COMMAND ${CMAKE_CTEST_COMMAND} -L ${project_name} --output-on-failure \$\(ARGS\)
        DEPENDS build_${project_name}_tests
    )
    add_local_target(run_tests DEPENDS run_${project_name}_tests)

    # Create custom run-unit-tests target for the project, if enabled
    if (TEST_UNIT)
        add_custom_target(run_${project_name}_unit_tests
            COMMAND ${CMAKE_CTEST_COMMAND} -L unit -L ${project_name} --output-on-failure \$\(ARGS\)
            DEPENDS build_${project_name}_tests
        )
        add_local_target(run_unit_tests DEPENDS run_${project_name}_unit_tests)
    endif()

    # Create custom run-integration-tests target for the project, if enabled
    if (TEST_INTEGRATION)
        add_custom_target(run_${project_name}_integ_tests
            COMMAND ${CMAKE_CTEST_COMMAND} -L integration -L ${project_name} --output-on-failure \$\(ARGS\)
            DEPENDS build_${project_name}_tests
        )
        add_local_target(run_integ_tests DEPENDS run_${project_name}_integ_tests)
    endif()
endfunction()
