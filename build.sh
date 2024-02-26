#!/usr/bin/env bash

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

# -----------------------------------------------------------------------------

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

###
# Arguments
###

TARGET="all"
VERSION="dev"
COMPILE_IMAGE_TAG="latest"
OUTPUT_DIR=""
CMAKE_ARGS=""
JOBS="$(nproc)"
VERBOSE=""

LIST_TARGETS="false"

ANDROID_BUILD=""
ANDROID_ARM64_BUILD=""
LINUX_BUILD=""
LINUX_ARM64_BUILD=""
COVERAGE_BUILD=""
CROSS_COMPILE=""

CREATE_KITS="true"

HELP=\
"This script builds the RACE system. By default, the entire system will be built
for Linux. The target to build and the architecture may be overriden.

Build Arguments:
    -A, --android
        Build for ANDROID_x86_64.
    -L, --linux
        Build for LINUX_x86_64.
    --android-arm64
        Build for ANDROID_arm64-v8a.
    --linux-arm64
        Build for LINUX_arm64-v8a.
    --coverage
        Build with coverage instrumentation
    -c [value], --cmake_args [value], --cmake_args=[value]
        Additional arguments to pass to cmake.
    -o [value], --output [value], --output=[value]
        The output directory. Defaults to 'out/'
    -j [value], --jobs [value], --jobs=[value]
        The number of threads to use when building. Defaults to number of cpu
        cores.
    -t [value], --target [value], --target=[value]
        The target to build. By default, this builds all targets. To see what
        targets exist use --list
    --no-kits
        Disable creation of artifact kits post-build
    --verbose
        Make everything very verbose.
    -v [value], --version [value], --version=[value]
        Specify the BUILD_VERSION preprocessor definition. Defaults to '${VERSION}'
    --compile-image-tag [value], --compile-image-tag=[value]
        Tag of the compile image from which build files are copied. Only applies to M1 Mac builds.

Help Arguments:
    -h, --help
        Print this message
    -l, --list
        List the possible targets to build

Examples:
    $0
    $0 --android
    $0 --verbose -j 1
    $0 -t raceSdkCore
    $0 -l
"

while [ $# -gt 0 ]
do
    key="$1"

    case $key in
        -A|--android)
        ANDROID_BUILD="true"
        shift
        ;;
        --android-arm64)
        ANDROID_ARM64_BUILD="true"
        shift
        ;;
        -L|--linux)
        LINUX_BUILD="true"
        shift
        ;;
        --linux-arm64)
        LINUX_ARM64_BUILD="true"
        shift
        ;;        
        --coverage)
        COVERAGE_BUILD="true"
        shift
        ;;

        -c|--cmake_args)
        CMAKE_ARGS="$2"
        shift
        shift
        ;;
        --cmake_args=*)
        CMAKE_ARGS="${1#*=}"
        shift
        ;;

        -j|--jobs)
        JOBS="$2"
        shift
        shift
        ;;
        --jobs=*)
        JOBS="${1#*=}"
        shift
        ;;

        -o|--output)
        OUTPUT_DIR="$2"
        shift
        shift
        ;;
        --output=*)
        OUTPUT_DIR="${1#*=}"
        shift
        ;;

        -t|--target)
        TARGET="$2"
        shift
        shift
        ;;
        --target=*)
        TARGET="${1#*=}"
        shift
        ;;

        --no-kits)
        CREATE_KITS="false"
        shift
        ;;

        --verbose)
        VERBOSE="-DCMAKE_VERBOSE_MAKEFILE=ON"
        shift
        ;;

        -v|--version)
        VERSION="$2"
        shift
        shift
        ;;
        --version=*)
        VERSION="${1#*=}"
        shift
        ;;

        --compile-image-tag)
        COMPILE_IMAGE_TAG="$2"
        shift
        shift
        ;;
        --compile-image-tag=*)
        COMPILE_IMAGE_TAG="${1#*=}"
        shift
        ;;

        -h|--help)
        printf "%s" "${HELP}"
        shift
        exit 1;
        ;;

        -l|--list)
        LIST_TARGETS="true"
        shift
        ;;
        *)
        echo "unknown argument \"$1\""
        exit 1
        ;;
    esac
done

function isEnvM1() {
    if uname -a | grep -qE 'Darwin.*arm64'; then
        return 0
    else
        return 1
    fi
}

if [ -z ${ANDROID_BUILD} ] && [ -z ${ANDROID_ARM64_BUILD} ] && [ -z ${LINUX_BUILD} ] && [ -z ${LINUX_ARM64_BUILD} ] && [ -z ${COVERAGE_BUILD} ]; then
    ANDROID_ARM64_BUILD="true"
    # Don't build Android x86 or Linux on native M1 Mac.
    if ! isEnvM1; then
        ANDROID_BUILD="true"
        # TODO: does it make sense to check whether we are running in an ARM image and change this default to linux ARM?
        LINUX_BUILD="true"
    fi
fi

if [ -n "${VERBOSE}" ] ; then
    set -x
fi

if [ -z "${OUTPUT_DIR}" ] ; then
    OUTPUT_DIR="${DIR}/out"
fi

if [[ $(uname -m) == *"x86_64"* ]]; then
    CROSS_COMPILE="-cross-compile"
fi

export CC=clang
export CXX=clang++

if [ -n "${LINUX_BUILD}" ] ; then
    echo "CMAKE BUILD LINUX CACHE"
    # shellcheck disable=SC2086
    cmake --preset=LINUX_x86_64 -DBUILD_VERSION="${VERSION}" ${VERBOSE} ${CMAKE_ARGS}
fi
if [ -n "${LINUX_ARM64_BUILD}" ] ; then
    echo "CMAKE BUILD LINUX arm64-v8a CACHE"
    # shellcheck disable=SC2086
    cmake --preset=LINUX_arm64-v8a${CROSS_COMPILE} -DBUILD_VERSION="${VERSION}" ${VERBOSE} ${CMAKE_ARGS}
fi
if [ -n "${ANDROID_BUILD}" ] ; then
    echo "CMAKE BUILD ANDROID CACHE"
    # shellcheck disable=SC2086
    cmake --preset=ANDROID_x86_64 -DBUILD_VERSION="${VERSION}" ${VERBOSE} ${CMAKE_ARGS}
fi
if [ -n "${ANDROID_ARM64_BUILD}" ] ; then
    echo "CMAKE BUILD ANDROID ARM64 CACHE"
    CMAKE_PRESET_NAME="ANDROID_arm64-v8a"
    if isEnvM1; then
        CMAKE_PRESET_NAME="ANDROID_arm64-v8a_M1"
        export JAVA_HOME=/Library/Java/JavaVirtualMachines/zulu-8.jdk/Contents/Home
        export ANDROID_SDK_ROOT=$HOME/Library/Android/sdk/
        # shellcheck disable=SC1091
        "${DIR}/m1-build/android_arm64-v8a/setup.sh" --version "${COMPILE_IMAGE_TAG}"
    fi
    # shellcheck disable=SC2086
    cmake --preset=${CMAKE_PRESET_NAME} -DBUILD_VERSION="${VERSION}" ${VERBOSE} ${CMAKE_ARGS}
fi
if [ -n "${COVERAGE_BUILD}" ]; then
    echo "CMAKE BUILD COVERAGE CACHE"
    # shellcheck disable=SC2086
    cmake --preset=coverage -DBUILD_VERSION="${VERSION}" ${VERBOSE} ${CMAKE_ARGS}
fi

if [ "$LIST_TARGETS" = "true" ] ; then
    if [ -n "${LINUX_BUILD}" ] ; then
        echo "CMAKE LIST LINUX TARGETS"
        cmake --build --preset=LINUX_x86_64 --target help
    fi
    if [ -n "${LINUX_ARM64_BUILD}" ] ; then
        echo "CMAKE LIST LINUX ARM64 TARGETS"
        cmake --build --preset=LINUX_arm64-v8a${CROSS_COMPILE} --target help
    fi
    if [ -n "${ANDROID_BUILD}" ] ; then
        echo "CMAKE LIST ANDROID TARGETS"
        cmake --build --preset=ANDROID_x86_64 --target help
    fi
    if [ -n "${ANDROID_ARM64_BUILD}" ] ; then
        echo "CMAKE LIST ANDROID ARM64 TARGETS"
        cmake --build --preset=ANDROID_arm64-v8a --target help
    fi
    if [ -n "${COVERAGE_BUILD}" ] ; then
        echo "CMAKE LIST COVERAGE TARGETS"
        cmake --build --preset=coverage --target help
    fi
    exit 1
fi

# Using DESTDIR here instead of CMAKE_STAGING_PREFIX because staging prefix doesn't work if some
# targets aren't under cmake install prefix. The api is currently installed outside of the install
# prefix because it needs to be installed in the same place for both linux and android.
if [ -n "${LINUX_BUILD}" ] ; then
    echo "CMAKE BUILD LINUX"
    # shellcheck disable=SC2086
    cmake --build --preset=LINUX_x86_64 ${CMAKE_ARGS} --target "${TARGET}" -- --jobs "${JOBS}"
fi
if [ -n "${LINUX_ARM64_BUILD}" ] ; then
    echo "CMAKE BUILD LINUX ARM64"
    # shellcheck disable=SC2086
    cmake --build --preset=LINUX_arm64-v8a${CROSS_COMPILE} ${CMAKE_ARGS} --target "${TARGET}" -- --jobs "${JOBS}"
fi
if [ -n "${ANDROID_BUILD}" ] ; then
    echo "CMAKE BUILD ANDROID"
    # shellcheck disable=SC2086
    cmake --build --preset=ANDROID_x86_64 ${CMAKE_ARGS} --target "${TARGET}" -- --jobs "${JOBS}"
fi
if [ -n "${ANDROID_ARM64_BUILD}" ] ; then
    echo "CMAKE BUILD ANDROID ARM64"
    # shellcheck disable=SC2086
    cmake --build --preset=ANDROID_arm64-v8a ${CMAKE_ARGS} --target "${TARGET}" -- --jobs "${JOBS}"
fi
if [ -n "${COVERAGE_BUILD}" ] ; then
    echo "CMAKE BUILD COVERAGE"
    # shellcheck disable=SC2086
    cmake --build --preset=coverage ${CMAKE_ARGS} --target "${TARGET}"
fi

if [ "$CREATE_KITS" = "true" ]; then
    python3 "${DIR}/create-kits.py"
fi
