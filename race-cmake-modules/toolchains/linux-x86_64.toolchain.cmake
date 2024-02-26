
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

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(TARGET x86_64-linux-gnu)
set(CMAKE_PREFIX_PATH /linux/x86_64)

set(CMAKE_FIND_ROOT_PATH ${CMAKE_PREFIX_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_C_COMPILER_TARGET ${TARGET})
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_IMPLICIT_LINK_DIRECTORIES ${CMAKE_PREFIX_PATH}/lib)
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_PREFIX_PATH}/include)

set(CMAKE_CXX_COMPILER_TARGET ${TARGET})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES ${CMAKE_PREFIX_PATH}/lib)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_PREFIX_PATH}/include)

set(JAVA_HOME "/usr/lib/jvm/java-8-openjdk-amd64")