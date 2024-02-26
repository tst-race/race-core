
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

cmake_minimum_required(VERSION 3.10)
################################################################################
# Generated C++ header
################################################################################

add_custom_command(
    COMMAND go tool cgo -exportheader libPluginGolang.h ${CMAKE_CURRENT_SOURCE_DIR}/CommsPlugin.go
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/channels.go
        ${CMAKE_CURRENT_SOURCE_DIR}/CommsPlugin.go
    OUTPUT libPluginGolang.h
)

################################################################################
# Plugin library
################################################################################

set(PLUGIN_FILE_NAME libPluginCommsTwoSixGolang.so)
set(PLUGIN_FILE ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_FILE_NAME})

set(GOCMD go build -work -buildmode=c-shared -o ${PLUGIN_FILE})

if(TARGET commsPluginBindingsGolang)
    get_target_property(COMMS_PLUGIN_BINDINGS_GOLANG_BINARY_DIR commsPluginBindingsGolang BINARY_DIR)
    set(COMMS_PLUGIN_BINDINGS_GOLANG_GOPATH ${COMMS_PLUGIN_BINDINGS_GOLANG_BINARY_DIR}/include)
endif()

if(ANDROID)
    set(ANDROID_PLATFORM 29)
    # Setup common GOENV variables
    list(APPEND GOENV
        CC=${ANDROID_TOOLCHAIN_ROOT}/bin/${CMAKE_LIBRARY_ARCHITECTURE}${ANDROID_PLATFORM}-clang
        CXX=${ANDROID_TOOLCHAIN_ROOT}/bin/${CMAKE_LIBRARY_ARCHITECTURE}${ANDROID_PLATFORM}-clang++
        GOOS=android
        CGO_ENABLED=1
    )

    if ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_x86_64")
        list(APPEND CGO_CXXFLAGS "-I/android/x86_64/include")
        list(APPEND CGO_LDFLAGS "-L/android/x86_64/lib")
        list(APPEND GOENV
            GOPATH=/android/x86_64/go:${COMMS_PLUGIN_BINDINGS_GOLANG_GOPATH}
            GOARCH=amd64
        )
    else()
        list(APPEND CGO_CXXFLAGS "-I/android/arm64-v8a/include")
        list(APPEND CGO_LDFLAGS "-L/android/arm64-v8a/lib")
        list(APPEND GOENV
            GOPATH=/android/arm64-v8a/go:${COMMS_PLUGIN_BINDINGS_GOLANG_GOPATH}
            GOARCH=arm64
        )
    endif()

else()
    if ("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_x86_64")
        list(APPEND GOENV
            "CC=clang -target x86_64-linux-gnu"
            "CXX=clang++ -target x86_64-linux-gnu"
        )
        list(APPEND GOCMD -ldflags "-linkmode=external '-extldflags=-target x86_64-linux-gnu'")
        list(APPEND CGO_CXXFLAGS "-I/linux/x86_64/include")
        list(APPEND CGO_LDFLAGS "-L/linux/x86_64/lib")
        list(APPEND GOENV GOARCH=amd64)
    else()
        list(APPEND GOENV
            "CC=clang -target aarch64-linux-gnu"
            "CXX=clang++ -target aarch64-linux-gnu"
        )
        list(APPEND GOCMD -ldflags "-linkmode=external '-extldflags=-target aarch64-linux-gnu'")
        list(APPEND CGO_CXXFLAGS "-I/linux/arm64-v8a/include")
        list(APPEND CGO_LDFLAGS "-L/linux/arm64-v8a/lib")
        list(APPEND GOENV GOARCH=arm64)
    endif()

    list(APPEND GOENV
        GOPATH=/usr/local/go:${COMMS_PLUGIN_BINDINGS_GOLANG_GOPATH}
        GOOS=linux
        CGO_ENABLED=1
    )
endif()

# Set common CXX/LD flags
list(APPEND CGO_CXXFLAGS
    -DBUILD_VERSION=\"${BUILD_VERSION}\"
    -I${CMAKE_CURRENT_BINARY_DIR}
)
list(APPEND CGO_LDFLAGS
    -Wl,--unresolved-symbols=ignore-all
    -lraceSdkCommon
    -l:commsPluginBindingsGolang.so
)

if(TARGET raceSdkCommon)
    get_target_property(RACE_SDK_COMMON_INCLUDE_DIRECTORIES raceSdkCommon INCLUDE_DIRECTORIES)
    foreach (include_dir ${RACE_SDK_COMMON_INCLUDE_DIRECTORIES})
    list(APPEND CGO_CXXFLAGS
        -I${include_dir}
    )
    endforeach()

    get_target_property(RACE_SDK_COMMON_BINARY_DIR raceSdkCommon BINARY_DIR)
    list(APPEND CGO_LDFLAGS -L${RACE_SDK_COMMON_BINARY_DIR})
endif()

if(TARGET commsPluginBindingsGolang)
    get_target_property(COMMS_PLUGIN_BINDINGS_GOLANG_INCLUDE_DIRECTORIES commsPluginBindingsGolang INCLUDE_DIRECTORIES)
    foreach (include_dir ${COMMS_PLUGIN_BINDINGS_GOLANG_INCLUDE_DIRECTORIES})
    list(APPEND CGO_CXXFLAGS
        -I${include_dir}
    )
    endforeach()

    get_target_property(COMMS_PLUGIN_BINDINGS_GOLANG_BINARY_DIR commsPluginBindingsGolang BINARY_DIR)
    list(APPEND CGO_LDFLAGS -L${COMMS_PLUGIN_BINDINGS_GOLANG_BINARY_DIR})
endif()


# Convert array variables to strings
string(REPLACE ";" " " CGO_CXXFLAGS "${CGO_CXXFLAGS}")
string(REPLACE ";" " " CGO_LDFLAGS "${CGO_LDFLAGS}")

list(APPEND GOENV
    CGO_CXXFLAGS=${CGO_CXXFLAGS}
    CGO_LDFLAGS=${CGO_LDFLAGS}
)

add_custom_command(
    COMMAND ${CMAKE_COMMAND} -E env ${GOENV} ${GOCMD}
    DEPENDS libPluginGolang.h ${CMAKE_CURRENT_SOURCE_DIR}/loader.cpp
    OUTPUT ${PLUGIN_FILE_NAME}
    BYPRODUCTS libPluginCommsTwoSixGolang.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)
set_directory_properties(PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES libPluginCommsTwoSixGolang.h
)

add_custom_target(PluginCommsTwoSixGolang ALL DEPENDS ${PLUGIN_FILE_NAME})

if(TARGET raceSdkCommon)
    add_dependencies(PluginCommsTwoSixGolang raceSdkCommon commsPluginBindingsGolang)
endif()

install(
    FILES ${PLUGIN_FILE}
    DESTINATION lib/race/comms/PluginCommsTwoSixGolang
)

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json
    DESTINATION lib/race/comms/PluginCommsTwoSixGolang
)

if(ANDROID)
    if ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_x86_64")
        add_custom_command(TARGET PluginCommsTwoSixGolang POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/channels/race-python-utils
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-x86_64-client/PluginCommsTwoSixGolang/
            COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-x86_64-client/PluginCommsTwoSixGolang/
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-x86_64-client/PluginCommsTwoSixGolang/
        )
        set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-x86_64-client/
        )
    else()
        add_custom_command(TARGET PluginCommsTwoSixGolang POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/channels/race-python-utils
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-arm64-v8a-client/PluginCommsTwoSixGolang/
            COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-arm64-v8a-client/PluginCommsTwoSixGolang/
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-arm64-v8a-client/PluginCommsTwoSixGolang/
        )
        set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/android-arm64-v8a-client/
        )
    endif()
else()
    if ("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_arm64-v8a")
        set(NODE_TYPE linux-arm64-v8a)
    else()
        set(NODE_TYPE linux-x86_64)
    endif()

    add_custom_command(TARGET PluginCommsTwoSixGolang POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../race-python-utils ${CMAKE_CURRENT_SOURCE_DIR}/../kit/channels/race-python-utils
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixGolang/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixGolang/
        COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixGolang/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/PluginCommsTwoSixGolang/
        COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixGolang/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/PluginCommsTwoSixGolang/
    )
    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-client/
        ${CMAKE_CURRENT_SOURCE_DIR}/../kit/artifacts/${NODE_TYPE}-server/
    )
endif()

setup_clang_format_for_files(
    NAME PluginCommsTwoSixGolangLoader
    FILES loader.cpp
    PARENT plugin_comms_twosix_golang
)

add_custom_target(format_PluginCommsTwoSixGolang
    COMMAND gofmt -w ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Formatting PluginCommsTwoSixGolang files..."
)
add_dependencies(format format_PluginCommsTwoSixGolang)
add_dependencies(format_plugin_comms_twosix_golang format_PluginCommsTwoSixGolang)

add_custom_target(check_format_PluginCommsTwoSixGolang
    COMMAND test -z `gofmt -l ${CMAKE_CURRENT_SOURCE_DIR}`
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking format for PluginCommsTwoSixGolang files..."
)
add_dependencies(check_format check_format_PluginCommsTwoSixGolang)
add_dependencies(check_format_plugin_comms_twosix_golang check_format_PluginCommsTwoSixGolang)
