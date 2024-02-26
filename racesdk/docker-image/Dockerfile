
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

FROM --platform=$TARGETPLATFORM ghcr.io/tst-race/race-images/race-compile:main as base
LABEL maintainer="Two Six Labs <race@twosixlabs.com>"

ARG TARGETARCH
RUN echo "building for architecture $TARGETARCH"

# Copy locally-built artifacts into the image
# Note: If you are adding new artifacts you may
#       need to update the .dockerignore file
#       so that they are included in the docker
#       context. Otherwise the COPY may fail.

FROM base as amd64-linux
COPY package/LINUX_x86_64 /linux/x86_64
COPY package/LINUX_x86_64/go /usr/local/go
COPY package/ANDROID_x86_64 /android/x86_64
RUN mkdir -p /linux/x86_64/include/race/mocks/
COPY package/test-mocks/include/* /linux/x86_64/include/race/mocks/

FROM base as arm64-linux
# TODO: do any performers build on ARM? trying to think how I can test this. -GP
COPY package/LINUX_arm64-v8a /linux/arm64-v8a
COPY package/LINUX_arm64-v8a/go /usr/local/go
RUN mkdir -p /linux/arm64-v8a/include/race/mocks/
COPY package/test-mocks/include/* /linux/arm64-v8a/include/race/mocks/

FROM ${TARGETARCH}-linux
COPY package/ANDROID_arm64-v8a /android/arm64-v8a
RUN mkdir -p /opt/race/race-cmake-modules
COPY package/race-cmake-modules /opt/race/race-cmake-modules
