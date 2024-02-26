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

CALL_NAME="$0"

###
# Arguments
###

DOCKER_ARGS=""
BUILD_ARGS=""

FILEPATH="$(pwd)"

INTERACTIVE_FLAG="-it"
RM_FLAG="--rm"

LOCAL_IMAGE="false"
CONTAINER_IMAGE="race-compile"
CONTAINER_NAME="race-builder"
CONTAINER_REGISTRY="ghcr.io"
CONTAINER_REPO="tst-race/race-images"
CONTAINER_VERSION="main"
CONTAINER_PULL="false"
CONTAINER_EXISTS="false"

# Linux default is to run as the current user, Mac is only allowed to run as root
RUN_AS_ROOT="false"

COMMAND="./build.sh"

HELP=\
'This script starts a docker container that may be used for building the RACE
system. By default, the build command is run inside the docker container.
Optionally, additional arguments to docker run may be provided, or a different
command may be run instead of build (such as a shell).

Arguments:
    -a [value], --args [value], --args=[value]
        Additional arguments to pass to docker when running the container.
    -b [value], --build-args [value], --build-args=[value]
        Additional arguments to pass to the command used when running the
        container. By default this is the build script. This may be used to
        build specific targets. Alternatively, -- may be used to terminate the
        list of arguments given to this script and the rest will be given to the
        run command.
    -c [value], --command [value], --command=[value]
        The command to begin the container with. Specifying bash will provide an
        interactive shell.
    -f [value], --filepath [value], --filepath=[value]
        The location to mount into the build container. By default this is the
        current directory.
    --no-rm
        Do not remove the container after the container exits. By default the
        container will be cleaned up. If you want the container to remain for
        debugging, etc., this flag may be used.
    -l, --local
        Use a local image, instead of one from the container repository
    -r [value], --repo [value], --repo=[value]
        Specify a container repo. Defaults to '${CONTAINER_REPO}'.
    -i [value], --image [value], --image=[value]
        Specify a container image. Defaults to '${CONTAINER_IMAGE}'.
    -n [value], --name [value], --name=[value]
        Specify a container name.  Defaults to '${CONTAINER_NAME}'.
    -v [value], --version [value], --version=[value]
        Specify a container version.  Defaults to '${CONTAINER_VERSION}'.
    -p, --pull
        Pull an updated container version if the local image is out of date.
    -e, --existing
        Run the command in an existing build container
    --root
        Run the command as root in the build container (this is the default for MacOS)
    -h, --help
        Print this message.
    --
        Terminate the list of arguments given to this script and give any
        following arguments to the run command.

Examples:
    ./docker_run.sh -c bash
    ./docker_run.sh -- --verbose -j 1
    ./docker_run.sh -- -h

'

# Parse CLI Arguments
while [ $# -gt 0 ]
do
    key="$1"

    case $key in
        -a|--args)
        DOCKER_ARGS="$2"
        shift
        shift
        ;;
        --args=*)
        DOCKER_ARGS="${1#*=}"
        shift
        ;;

        -b|--build-args)
        BUILD_ARGS="$2"
        shift
        shift
        ;;
        --build-args=*)
        BUILD_ARGS="${1#*=}"
        shift
        ;;

        -c|--command)
        COMMAND="$2"
        shift
        shift
        ;;
        --command=*)
        COMMAND="${1#*=}"
        shift
        ;;

        -f|--filepath)
        FILEPATH="$2"
        shift
        shift
        ;;
        --filepath=*)
        FILEPATH="${1#*=}"
        shift
        ;;

        --no-rm)
        RM_FLAG=""
        shift
        ;;

        -l|--local)
        LOCAL_IMAGE="true"
        shift
        ;;

        -r|--repo)
        CONTAINER_REPO="$2"
        shift
        shift
        ;;
        --repo=*)
        CONTAINER_REPO="${1#*=}"
        shift
        ;;

        -i|--image)
        CONTAINER_IMAGE="$2"
        shift
        shift
        ;;
        --image=*)
        CONTAINER_IMAGE="${1#*=}"
        shift
        ;;

        -v|--version)
        CONTAINER_VERSION="$2"
        shift
        shift
        ;;
        --version=*)
        CONTAINER_VERSION="${1#*=}"
        shift
        ;;

        -n|--name)
        CONTAINER_NAME="$2"
        shift
        shift
        ;;
        --name=*)
        CONTAINER_NAME="${1#*=}"
        shift
        ;;

        -p|--pull)
        CONTAINER_PULL="true"
        shift
        ;;

        -e|--existing)
        CONTAINER_EXISTS="true"
        shift
        ;;

        --root)
        RUN_AS_ROOT="true"
        shift
        ;;

        -h|--help)
        printf "%s" "${HELP}"
        exit 1;
        ;;

        --)
        shift
        break
        ;;
        *)
        echo "${CALL_NAME} unknown argument \"$1\""
        exit 1
        ;;
    esac
done

###
# Main Execution
###


FULL_IMAGE=""
if [ "$LOCAL_IMAGE" = "false" ] ; then
    FULL_IMAGE="${CONTAINER_REGISTRY}/${CONTAINER_REPO}/${CONTAINER_IMAGE}:${CONTAINER_VERSION}"
    if [ "${CONTAINER_PULL}" = "true" ] ; then
        docker pull "${FULL_IMAGE}"
    fi
else
    FULL_IMAGE="${CONTAINER_IMAGE}:${CONTAINER_VERSION}"
fi

USER_FLAGS=""
# If OS is Linux, we can run as the current user inside the build container. This requires mounting
# in the user's home directory (otherwise there would be no home directory and many tools would
# fail to run properly) and the Linux passwd & group files (so that the user/group IDs are properly
# recognized).
if [ "$(uname)" = "Linux" ] ; then
    if [ "${RUN_AS_ROOT}" = "false" ]; then
        USER_FLAGS="
        -u "$(id -u):$(id -g)" \
        -v "/home/$(whoami):/home/$(whoami)" \
        -v /etc/passwd:/etc/passwd:ro \
        -v /etc/group:/etc/group:ro"
    fi
fi
# Else OS is Mac, and we always run as root on mac (no user flags needed)

echo "Using image ${FULL_IMAGE}"

docker inspect -f '{{ .Created }}' "${FULL_IMAGE}"

if [ "$CONTAINER_EXISTS" == "false" ] ; then
    # shellcheck disable=SC2086
    docker run \
        ${RM_FLAG} \
        ${INTERACTIVE_FLAG} \
        ${USER_FLAGS} \
        -v "${FILEPATH}":/code \
        -w /code \
        --name "${CONTAINER_NAME}" \
        ${DOCKER_ARGS} \
        "${FULL_IMAGE}" \
        "${COMMAND}" ${BUILD_ARGS} "$@"
else
    # shellcheck disable=SC2086
    docker exec -it ${DOCKER_ARGS} "${CONTAINER_NAME}" "${COMMAND}" ${BUILD_ARGS} "$@"
fi
