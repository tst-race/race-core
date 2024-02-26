
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

# Add the source to the test executable
add_executable(unitTestRaceSdkStorageEncryption
    main.cpp
    StorageEncryptionTest.cpp
)

# Link external dependencies
find_package(GTest REQUIRED CONFIG)
find_package(Threads REQUIRED)

target_link_libraries(unitTestRaceSdkStorageEncryption
    GTest::gmock
    GTest::gtest
    Threads::Threads
    raceSdkStorageEncryption
)

add_dependencies(build_racesdk_tests unitTestRaceSdkStorageEncryption)
add_test(storage-encryption ${CMAKE_CURRENT_BINARY_DIR}/unitTestRaceSdkStorageEncryption)
set_tests_properties(storage-encryption PROPERTIES LABELS "unit;racesdk")
setup_coverage_for_target(
    TARGET unitTestRaceSdkStorageEncryption
    SOURCE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../..
)
setup_valgrind_for_target(unitTestRaceSdkStorageEncryption)

# Setup style validation
setup_clang_format_for_target(unitTestRaceSdkStorageEncryption)
