
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
add_library(raceTestAppShared SHARED
    raceTestAppHelpers.cpp
    output/RaceTestAppOutputLog.cpp
    client/ReceivedMessage.cpp
    client/RaceApp.cpp
    message/Message.cpp
    RaceTestApp.cpp
    UserInputResponseCache.cpp
    UserInputResponseParser.cpp)

# Add the headers to the shared library
file(GLOB RACE_TEST_APP_SHARED_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/racetestapp/*.h
)
set_target_properties(raceTestAppShared PROPERTIES
    PUBLIC_HEADER "${RACE_TEST_APP_SHARED_PUBLIC_HEADERS}"
)
target_include_directories(raceTestAppShared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

target_link_options(raceTestAppShared PRIVATE --no-undefined)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(raceTestAppShared
    raceSdkCommon
    raceSdkStorageEncryption
    raceSdkCore
)

# TODO enable performance-* and readability-* checks (needs fixes)
setup_clang_tidy_for_target(TARGET raceTestAppShared)
setup_cppcheck_for_target(TARGET raceTestAppShared CHECKS warning style)
setup_clang_format_for_target(raceTestAppShared PARENT racesdk)

# Install/export all targets
install(
    TARGETS raceTestAppShared
    EXPORT raceTestAppShared
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/racetestapp
)
install(
    EXPORT raceTestAppShared
    DESTINATION lib/cmake
)