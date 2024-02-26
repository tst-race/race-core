
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

# Android Linux arm64-v8a, API v29
#
# Set required variables then include the NDK-provided toolchain file

set(ANDROID_ABI arm64-v8a)
set(ANDROID_PLATFORM 29)
set(ANDROID_STL c++_shared)
set(ANDROID_SO_UNDEFINED true)
set(MIN_SDK_VERSION 29)

set(CMAKE_CXX_FLAGS "-DANDROID_STL=c++_shared -frtti ${CMAKE_CXX_FLAGS}")
set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/m1-build/android_arm64-v8a/arm64-v8a)
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SOURCE_DIR}/m1-build/android_arm64-v8a/arm64-v8a)

set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/m1-build/android_arm64-v8a/arm64-v8a/include)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/m1-build/android_arm64-v8a/arm64-v8a/include)
set(CMAKE_C_IMPLICIT_LINK_DIRECTORIES ${CMAKE_SOURCE_DIR}/m1-build/android_arm64-v8a/arm64-v8a/lib)
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES ${CMAKE_SOURCE_DIR}/m1-build/android_arm64-v8a/arm64-v8a/lib)

include($ENV{HOME}/Library/Android/sdk/ndk/23.2.8568313/build/cmake/android.toolchain.cmake)
