#!/bin/bash

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




HELPER_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

function start_containers {
	echo "building docker image in case it's not up-to-date" | tee >(cat >&2)
	cd "${HELPER_DIR}/../.."
	bash build_docker_image.sh -v test > /dev/null
	cd "${HELPER_DIR}"

	echo "create docker network if in case it doesn't exist" | tee >(cat >&2)
	docker network create --driver bridge whiteboard-net >&/dev/null

	echo "start containers" | tee >(cat >&2)
	REDIS_CONFIG=$(realpath ${HELPER_DIR}/../../src/config/)
	REDIS=$(docker run --rm -d -p 6379:6379 $1 -u $(id -u):$(id -g) -v "${REDIS_CONFIG}:/config" --network whiteboard-net --name twosix-redis redis /config/redis.conf)
	WHITEBOARD=$(docker run --rm -d -p 5000:5000 $2 -u $(id -u):$(id -g) --network whiteboard-net --name twosix-whiteboard twosix-whiteboard:test -w 8)
}

function stop_containers {
	echo "stop containers" | tee >(cat >&2)
	docker stop ${WHITEBOARD} >/dev/null
	docker stop ${REDIS} >/dev/null
}
