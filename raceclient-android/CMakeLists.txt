
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

project(raceclient-android)

set(APK app/build/outputs/apk/debug/app-debug.apk)

file(GLOB_RECURSE APK_SRC app/src/**/*.java)

macro(get_target_file_or_find outvar targetname)
	if (TARGET ${targetname})
		set(${outvar} $<TARGET_FILE:${targetname}>)
	else()
		find_library(${outvar} ${targetname} ${ARGN})
	endif()
endmacro()

get_target_file_or_find(RACE_SDK_COMMON raceSdkCommon)
get_target_file_or_find(RACE_SDK_STORAGE_ENCRYPTION raceSdkStorageEncryption)
get_target_file_or_find(RACE_SDK_CORE raceSdkCore)
get_target_file_or_find(RACE_JAVA_SHIMS RaceJavaShims)
get_target_file_or_find(RACE_TEST_APP_SHARED raceTestAppShared)
get_target_file_or_find(RACE_COMMS_GOLANG_SHIMS commsPluginBindingsGolang NAMES commsPluginBindingsGolang.so)

if (TARGET networkManagerPluginBindings)
	get_target_property(RACE_NETWORK_MANAGER_PYTHON_SHIMS_BINARY_DIR networkManagerPluginBindings BINARY_DIR)
	set(RACE_NETWORK_MANAGER_PYTHON_SHIMS_INCLUDE_DIR ${RACE_NETWORK_MANAGER_PYTHON_SHIMS_BINARY_DIR}/include)
else()
	set(RACE_NETWORK_MANAGER_PYTHON_SHIMS_BINARY_DIR ${CMAKE_INSTALL_PREFIX}/lib/race/python)
	set(RACE_NETWORK_MANAGER_PYTHON_SHIMS_INCLUDE_DIR ${RACE_NETWORK_MANAGER_PYTHON_SHIMS_BINARY_DIR})
endif()
set(RACE_NETWORK_MANAGER_PYTHON_SHIMS ${RACE_NETWORK_MANAGER_PYTHON_SHIMS_BINARY_DIR}/_networkManagerPluginBindings.so ${RACE_NETWORK_MANAGER_PYTHON_SHIMS_INCLUDE_DIR}/networkManagerPluginBindings.py)
set(RACE_NETWORK_MANAGER_COPY_PYTHON_COMMAND COMMAND cp -f ${RACE_NETWORK_MANAGER_PYTHON_SHIMS} app/src/main/assets/race/python/)

if (TARGET commsPluginBindings)
	get_target_property(RACE_COMMS_PYTHON_SHIMS_BINARY_DIR commsPluginBindings BINARY_DIR)
	set(RACE_COMMS_PYTHON_SHIMS_INCLUDE_DIR ${RACE_COMMS_PYTHON_SHIMS_BINARY_DIR}/include)
else()
	set(RACE_COMMS_PYTHON_SHIMS_BINARY_DIR ${CMAKE_INSTALL_PREFIX}/lib/race/python)
	set(RACE_COMMS_PYTHON_SHIMS_INCLUDE_DIR ${RACE_COMMS_PYTHON_SHIMS_BINARY_DIR})
endif()
set(RACE_COMMS_PYTHON_SHIMS ${RACE_COMMS_PYTHON_SHIMS_BINARY_DIR}/_commsPluginBindings.so ${RACE_COMMS_PYTHON_SHIMS_INCLUDE_DIR}/commsPluginBindings.py)
set(RACE_COMMS_COPY_PYTHON_COMMAND COMMAND cp -f ${RACE_COMMS_PYTHON_SHIMS} app/src/main/assets/race/python/)

# TODO java shims should set up the jar as an imported target so the jar
# file can be referenced like other targets
#get_target_file_or_find(RACE_JAVA_SHIMS_JAR RaceSdkJavaShimsJar)
if (TARGET RaceSdkJavaShimsJar)
	get_target_property(RACE_JAVA_SHIMS_JAR_BINARY_DIR RaceSdkJavaShimsJar BINARY_DIR)
else()
	set(RACE_JAVA_SHIMS_JAR_BINARY_DIR ${CMAKE_INSTALL_PREFIX}/lib/race/java)
endif()
set(RACE_JAVA_SHIMS_JAR ${RACE_JAVA_SHIMS_JAR_BINARY_DIR}/racesdk-java-shims-1.jar)

find_library(LIB_BOOST_FILESYSTEM boost_filesystem)
find_library(LIB_BOOST_SYSTEM boost_system)
find_library(LIB_CRYPTO crypto)
find_library(LIB_FFI ffi)
find_library(LIB_PYTHON python3.7m)
find_library(LIB_SSL ssl)
find_library(LIB_THRIFT thrift)
find_library(LIB_YAML_CPP yaml-cpp)
find_library(LIB_CPP_SHARED c++_shared)
find_library(LIB_ZIP zip)
find_library(LIB_ARCHIVE archive REQUIRED)

if (TARGET raceSdkCommon)
	set(RACE_APK_DEPENDENCIES
		raceSdkCommon
		raceSdkStorageEncryption
		raceSdkCore
		RaceJavaShims
		raceTestAppShared
		commsPluginBindingsGolang
		networkManagerPluginBindings
		commsPluginBindings
	)
endif()

if ("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_arm64-v8a")
	set(ARCHITECTURE_DEPEND_DIR arm64-v8a)
else()
	set(ARCHITECTURE_DEPEND_DIR x86_64)
endif()

if (${CMAKE_HOST_SYSTEM} MATCHES "Darwin")
	set(PYTHON_PACKAGES_PATH ${CMAKE_SOURCE_DIR}/m1-build/android_arm64-v8a/${ARCHITECTURE_DEPEND_DIR})
else()
	set(PYTHON_PACKAGES_PATH /android/${ARCHITECTURE_DEPEND_DIR})
endif()
set(RACE_PYTHON_PACKAGES_COPY_COMMAND COMMAND cp -f ${PYTHON_PACKAGES_PATH}/python-packages-*.tar.gz  app/src/main/assets/)

add_custom_command(
	COMMAND rm -rf app/libs
	COMMAND rm -rf app/src/main/assets
	COMMAND rm -rf app/src/main/jniLibs
	COMMAND mkdir -p app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND mkdir -p app/libs
	COMMAND mkdir -p app/src/main/assets

	COMMAND mkdir -p app/src/main/assets/race/python

	COMMAND cp -f ${RACE_JAVA_SHIMS_JAR}           app/libs/

	# Copy AAR dependencies
	COMMAND cp -f ../aar/*         app/libs/

	# RACE shared libraries
	COMMAND cp -f ${RACE_SDK_COMMON}               app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${RACE_SDK_STORAGE_ENCRYPTION}   app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${RACE_SDK_CORE}                 app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${RACE_JAVA_SHIMS}               app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${RACE_TEST_APP_SHARED}          app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${RACE_COMMS_GOLANG_SHIMS}       app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/

	# Copy language bindings
	${RACE_NETWORK_MANAGER_COPY_PYTHON_COMMAND}
	${RACE_COMMS_COPY_PYTHON_COMMAND}

	# Cross-compiled python source
	${RACE_PYTHON_PACKAGES_COPY_COMMAND}

	# Cross-compiled shared libraries
	COMMAND cp -f ${LIB_BOOST_FILESYSTEM}          app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${LIB_BOOST_SYSTEM}              app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${LIB_CRYPTO}                    app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${LIB_FFI}                       app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${LIB_PYTHON}                    app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${LIB_SSL}                       app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${LIB_THRIFT}                    app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${LIB_YAML_CPP}                  app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${LIB_ZIP}                       app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
	COMMAND cp -f ${LIB_ARCHIVE}                   app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/

	# NDK shared libraries
	COMMAND cp -f ${LIB_CPP_SHARED}                app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/

	# building in SRI Comms x86_64 Dependencies
	COMMAND cp -f ../wedge/ANDROID_${ARCHITECTURE_DEPEND_DIR}/*wedge.so app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/

    COMMAND ./gradlew --info assembleDebug
    COMMAND cp -f ${APK} ${CMAKE_CURRENT_BINARY_DIR}/race.apk

    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/kit/artifacts/android-${ARCHITECTURE_DEPEND_DIR}-client/race/
    COMMAND ${CMAKE_COMMAND} -E copy ${APK} ${CMAKE_CURRENT_SOURCE_DIR}/kit/artifacts/android-${ARCHITECTURE_DEPEND_DIR}-client/race/race.apk

    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS
		build.gradle
		app/build.gradle
		${APK_SRC}
		${RACE_APK_DEPENDENCIES}
		${RACE_JAVA_SHIMS_JAR}
    OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/race.apk
		${CMAKE_CURRENT_SOURCE_DIR}/.gradle/
		${CMAKE_CURRENT_SOURCE_DIR}/kit/artifacts/android-${ARCHITECTURE_DEPEND_DIR}-client/race/race.apk
		${CMAKE_CURRENT_SOURCE_DIR}/app/build/
		${CMAKE_CURRENT_SOURCE_DIR}/app/libs/
		${CMAKE_CURRENT_SOURCE_DIR}/app/src/main/assets/
		${CMAKE_CURRENT_SOURCE_DIR}/app/src/main/jniLibs/${ARCHITECTURE_DEPEND_DIR}/
    COMMENT "Generating Race Apk..."
)

add_custom_target(RaceClientApk ALL
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/race.apk
)

if (TARGET raceSdkCommon)
	add_dependencies(RaceClientApk ${RACE_APK_DEPENDENCIES})
endif()

include(race/java-format)
setup_java_format_for_target(RaceClientApk "${APK_SRC}")

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/race.apk
    DESTINATION .
    )
