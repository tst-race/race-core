#!/usr/bin/env bash

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


set -o errexit
set -o nounset
set -o pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

FORCE=false
COMPILE_IMAGE_TAG="latest"


HELP=\
"Setup the environment in order to build Android on M1 Macs. Specifically
copies necessary files out of the compile image.

Build Arguments:
    -f, --force
        Overwrite any existing files.
    -v [value], --version [value], --version=[value]
        Specify the compile image tag. Defaults to '${COMPILE_IMAGE_TAG}'

Help Arguments:
    -h, --help
        Print this message

Examples:
    ./setup.sh
    ./setup.sh --force
    ./setup.sh --version my-custom-compile-image-tag
"

while [ $# -gt 0 ]
do
    key="$1"

    case $key in

        -f|--force)
        FORCE=true
        shift
        ;;

        -v|--version)
        COMPILE_IMAGE_TAG="$2"
        shift
        shift
        ;;
        --version=*)
        COMPILE_IMAGE_TAG="${1#*=}"
        shift
        ;;

        -h|--help)
        printf "%s" "${HELP}"
        exit 1;
        ;;

        *)
        echo "${CALL_NAME} unknown argument \"$1\""
        exit 1
        ;;
    esac
done

if [ "$FORCE" = true ] || [ ! -d "${DIR}/arm64-v8a" ]; then
  rm -rf "${DIR}/arm64-v8a" "${DIR}/include/"
  COMPILE_CONTAINER_NAME="temporary-race-compile-image-for-m1-arm-build"
  docker run --detach -it --rm --name $COMPILE_CONTAINER_NAME ghcr.io/tst-race/race-images/race-compile:"${COMPILE_IMAGE_TAG}" bash

  docker cp -a ${COMPILE_CONTAINER_NAME}:/android/arm64-v8a "${DIR}"/
  mkdir -p "${DIR}/include/"
  docker cp -a ${COMPILE_CONTAINER_NAME}:/linux/arm64-v8a/include/nlohmann "${DIR}/include/"

  docker rm -f ${COMPILE_CONTAINER_NAME}
else
  echo "Android files already exist. Skipping copy. Run with --force flag to overwrite existing files."
fi
