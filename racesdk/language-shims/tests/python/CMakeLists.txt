
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


add_executable(unitTestPythonCommsBindings
    main.cpp
    PyDecomposeTest.cpp
    CommsPluginTest.cpp
)

target_include_directories(unitTestPythonCommsBindings SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../common/include/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../core/include/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../core/test/common/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../core/source/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../core/source/plugin-loading/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../storage-encryption/include/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../test-mocks/include/>
)

target_compile_definitions(unitTestPythonCommsBindings
    PUBLIC PLUGIN_PATH="${CMAKE_CURRENT_SOURCE_DIR}/"
)

# Link external dependencies
find_package(GTest REQUIRED CONFIG)

target_link_libraries(unitTestPythonCommsBindings
    GTest::gmock
    raceSdkCommon
    raceSdkCore
    commsPluginBindings
    networkManagerPluginBindings
)

setup_coverage_for_target(
    TARGET unitTestPythonCommsBindings
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
    EXCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/*"
)

add_dependencies(build_racesdk_tests
    unitTestPythonCommsBindings)

add_test(python-bindings ${CMAKE_CURRENT_BINARY_DIR}/unitTestPythonCommsBindings)
set_tests_properties(python-bindings PROPERTIES LABELS "unit;racesdk")

string(CONCAT PYTHON_BINDINGS_PATHDEPS_STR
            "${CMAKE_CURRENT_SOURCE_DIR}:"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../:"
            "${CMAKE_CURRENT_SOURCE_DIR}/stubs:"
            "${CMAKE_CURRENT_BINARY_DIR}/include:"
            "${CMAKE_CURRENT_BINARY_DIR}/../../source/include")
add_definitions(-DPYTHON_BINDINGS_PATHDEPS="${PYTHON_BINDINGS_PATHDEPS_STR}")
