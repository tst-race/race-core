
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

cmake_minimum_required(VERSION 3.2.0)

# TODO replace this with list(TRANSFORM PREPEND) when we get CMake 3.12
function(prepend var prefix)
    set(tmp "")
    foreach(item ${${var}})
        list(APPEND tmp "${prefix}${item}")
    endforeach()
    set(${var} "${tmp}" PARENT_SCOPE)
endfunction()

set(PLUGIN_ID PluginCommsTwoSixPython)
set(PLUGIN_FILES
    __init__.py
    channels.py
    Log.py
    PluginCommsTwoSixPython.py
    response_logger.py
    manifest.json
)
set(PLUGIN_SOURCES "${PLUGIN_FILES}")
prepend(PLUGIN_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/source/)

if(ANDROID)
    if ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_arm64-v8a")
        set(NODE_TYPE android-arm64-v8a-client)
    else()
        set(NODE_TYPE android-x86_64-client)
    endif()

    set(ANDROID_CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}/${PLUGIN_ID})
    set(ANDROID_CLIENT_FILES "${PLUGIN_FILES}")
    prepend(ANDROID_CLIENT_FILES ${ANDROID_CLIENT_DIR}/)

    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/kit/channels/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ANDROID_CLIENT_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN_SOURCES} ${ANDROID_CLIENT_DIR}/
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../../source/aux-data ${ANDROID_CLIENT_DIR}/aux-data
        DEPENDS ${PLUGIN_SOURCES}
        OUTPUT ${ANDROID_CLIENT_FILES}
        COMMENT "Copying ${PLUGIN_ID} files to plugin/artifacts/${NODE_TYPE}"
    )

    add_custom_target(PluginCommsTwoSixPython ALL DEPENDS ${ANDROID_CLIENT_FILES})

    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        ${ANDROID_CLIENT_DIR}/
    )
else()
    if ("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_arm64-v8a")
        set(NODE_TYPE linux-arm64-v8a)
    else()
        set(NODE_TYPE linux-x86_64)
    endif()    

    set(LINUX_CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-client/${PLUGIN_ID})
    set(LINUX_CLIENT_FILES "${PLUGIN_FILES}")
    prepend(LINUX_CLIENT_FILES ${LINUX_CLIENT_DIR}/)

    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/kit/channels/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LINUX_CLIENT_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN_SOURCES} ${LINUX_CLIENT_DIR}/
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../../source/aux-data ${LINUX_CLIENT_DIR}/aux-data
        DEPENDS ${PLUGIN_SOURCES}
        OUTPUT ${LINUX_CLIENT_FILES}
        COMMENT "Copying ${PLUGIN_ID} files to plugin/artifacts/${NODE_TYPE}-client"
    )

    set(LINUX_SERVER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kit/artifacts/${NODE_TYPE}-server/${PLUGIN_ID})
    set(LINUX_SERVER_FILES "${PLUGIN_FILES}")
    prepend(LINUX_SERVER_FILES ${LINUX_SERVER_DIR}/)

    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/kit/channels/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LINUX_SERVER_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN_SOURCES} ${LINUX_SERVER_DIR}/
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../../source/aux-data ${LINUX_SERVER_DIR}/aux-data
        DEPENDS ${PLUGIN_SOURCES}
        OUTPUT ${LINUX_SERVER_FILES}
        COMMENT "Copying ${PLUGIN_ID} files to plugin/artifacts/${NODE_TYPE}-server"
    )

    add_custom_target(PluginCommsTwoSixPython ALL
        DEPENDS ${LINUX_CLIENT_FILES} ${LINUX_SERVER_FILES}
    )

    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        ${LINUX_CLIENT_DIR}/
        ${LINUX_SERVER_DIR}/
    )
endif()

install(
    DIRECTORY source/
    DESTINATION lib/race/comms/PluginCommsTwoSixPython
)
