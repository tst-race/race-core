
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

# Race Storage Encryption Command Line Interface
add_executable(racesecli
    main.cpp
)

# find_package(Boost COMPONENTS program_options REQUIRED)

if(ANDROID)
    find_package(Boost COMPONENTS filesystem system REQUIRED)

    target_link_libraries(racesecli
        PRIVATE
            raceSdkCommon
            raceSdkStorageEncryption
            Boost::system
            Boost::filesystem
            # Boost::program_options
    )
else()
    ## Default to Linux
    target_link_libraries(racesecli
        PUBLIC
            raceSdkCommon
            raceSdkStorageEncryption
        PRIVATE
            stdc++fs
            # Boost::program_options
    )
endif()

set_target_properties(racesecli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    BUILD_RPATH "\${ORIGIN}/../lib"
    INSTALL_RPATH "\${ORIGIN}/../lib"
)

# TODO enable performance-* and readability-* checks (needs fixes)
setup_clang_tidy_for_target(TARGET racesecli)
setup_cppcheck_for_target(
    TARGET racesecli
    CHECKS all
    SUPPRESS unmatchedSuppression unusedFunction missingIncludeSystem
)
setup_clang_format_for_target(racesecli PARENT racesdk)

################################################################################
# Install
################################################################################

install(
    TARGETS racesecli
    DESTINATION bin
)
