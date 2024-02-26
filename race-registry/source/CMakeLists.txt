
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
# RaceRegistry
################################################################################

add_executable(raceregistry
    createPidFile.cpp
    input/RaceTestAppInputFifo.cpp
    main.cpp
    RaceRegistry.cpp
    RaceRegistryApp.cpp
    output/NodeDaemonPublisher.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(raceregistry
    raceSdkCommon
    raceSdkCore
    raceTestAppShared
    crypto
    stdc++fs
    Threads::Threads
)

set_target_properties(raceregistry PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    BUILD_RPATH "\${ORIGIN}/../lib"
    INSTALL_RPATH "\${ORIGIN}/../lib"
)

# TODO enable performance-* and readability-* checks (needs fixes)
setup_clang_tidy_for_target(TARGET raceregistry)
setup_cppcheck_for_target(
    TARGET raceregistry
    CHECKS all
    SUPPRESS unmatchedSuppression unusedFunction missingIncludeSystem
)
setup_clang_format_for_target(raceregistry PARENT raceregistry)

################################################################################
# Install
################################################################################

install(
    TARGETS raceregistry
    DESTINATION bin)



###
# RACE Test App Executable
###

add_custom_command(TARGET raceregistry POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/linux-x86_64-client/registry/bin/

    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:raceregistry> ${PROJECT_SOURCE_DIR}/kit/artifacts/linux-x86_64-client/registry/bin/
)

add_custom_command(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/kit/artifacts/linux-x86_64-client/registry/bin/

    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/install.sh ${PROJECT_SOURCE_DIR}/kit/artifacts/linux-x86_64-client/registry/

    OUTPUT
        ${PROJECT_SOURCE_DIR}/kit/artifacts/linux-x86_64-client/registry/install.sh

    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/install.sh

    COMMENT "Copying install.sh..."
)

###
# App artifacts target
###

add_custom_target(RaceRegistryArtifacts ALL
    DEPENDS
        ${PROJECT_SOURCE_DIR}/kit/artifacts/linux-x86_64-client/registry/install.sh
)

set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    ${PROJECT_SOURCE_DIR}/kit/artifacts/linux-x86_64-client/
)
