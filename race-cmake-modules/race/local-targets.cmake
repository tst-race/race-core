
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

# Enable non-unique custom local targets (e.g., format)

# Duplicate targets are only supported for makefiles
if("${CMAKE_GENERATOR}" MATCHES "Make")
    set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)
    set(ALLOW_LOCAL_TARGETS 1)
endif()

function(add_local_target target)
    if(ALLOW_LOCAL_TARGETS)
        cmake_parse_arguments(LOCAL_TARGET "" "" "DEPENDS" ${ARGN})
        add_custom_target(${target} DEPENDS ${LOCAL_TARGET_DEPENDS})
    endif()
endfunction()
