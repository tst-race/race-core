
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

# NOTE: File must be included after the project() command.

# Set warning flags for the compiler
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_compile_options(
    -pedantic
    -Wall
    -Wextra
    -Wcast-align
    -Wcast-qual
    -Wctor-dtor-privacy
    -Wdisabled-optimization
    -Wformat=2
    -Winit-self
    -Wlogical-op
    -Wmissing-declarations
    -Wmissing-include-dirs
    -Wnoexcept
    -Wold-style-cast
    -Woverloaded-virtual
    -Wredundant-decls
    -Wshadow
    -Wsign-conversion
    -Wsign-promo
    -Wstrict-null-sentinel
    -Wstrict-overflow=5
    -Wswitch-default
    -Wundef
    -Werror
    -Wunused-parameter
    -Wsuggest-override)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-pedantic
    -Wall
    -Wextra
    -Wcast-align
    -Wcast-qual
    -Wctor-dtor-privacy
    -Wdisabled-optimization
    -Wformat=2
    -Winit-self
    -Wmissing-declarations
    -Wmissing-include-dirs
    -Woverloaded-virtual
    -Wredundant-decls
    -Wsign-conversion
    -Wsign-promo
    -Wstrict-overflow=5
    -Wswitch-default
    -Wundef
    -Wno-gnu-zero-variadic-macro-arguments)
    if(NOT ANDROID)
      add_compile_options(-Wold-style-cast
      -Wshadow
      -Werror)
    endif()
elseif(MSVC)
  add_compile_options(/W4 /WX)
else()
  message(WARNING "unrecognized compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()
