
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

# Add the source to the shared library
add_library(raceSdkStorageEncryption SHARED
    PassphraseHash.cpp
    Salt.cpp
    StorageEncryption.cpp
)

# Add the headers to the shared library
file(GLOB STORAGE_ENCRYPTION_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/*.h
)
set_target_properties(raceSdkStorageEncryption PROPERTIES
    PUBLIC_HEADER "${STORAGE_ENCRYPTION_HEADERS}"
)
target_include_directories(raceSdkStorageEncryption PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(raceSdkStorageEncryption PUBLIC "SWIG_TYPE_TABLE=RaceSdk")
target_link_options(raceSdkStorageEncryption PRIVATE --no-undefined)


if(ANDROID)
    find_package(Boost COMPONENTS filesystem system REQUIRED)
    find_library(LIB_CRYPTO crypto REQUIRED)

    target_link_libraries(raceSdkStorageEncryption
        PRIVATE
            raceSdkCommon
            Boost::system
            Boost::filesystem
            ${LIB_CRYPTO}
    )
else()
    ## Default to Linux
    target_link_libraries(raceSdkStorageEncryption
        PUBLIC
            raceSdkCommon
        PRIVATE
            stdc++fs
            crypto
    )
endif()

# TODO enable performance-* and readability-* checks (needs fixes)
setup_clang_tidy_for_target(TARGET raceSdkStorageEncryption)
setup_cppcheck_for_target(TARGET raceSdkStorageEncryption CHECKS warning style)
setup_clang_format_for_target(raceSdkStorageEncryption PARENT racesdk)
