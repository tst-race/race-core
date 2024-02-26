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
# This script expects to be run from a RACE installation bundle with the
# following directory layout:
#
# bundle/
#   artifact-manager/
#     PluginArtifactManagerId/
#       ...
#   config/
#     global/
#       race.json
#       ...
#     ...
#   race/
#     bin/
#       registry
#       ...
#     lib/
#       libraceSdkCommon.so
#       ...
#     install.sh
#   network-manager/
#     PluginNMId/
#       ...
#   comms/
#     PluginCommsId/
#       ...
#     ...
# -----------------------------------------------------------------------------

set -e

# Grab parent directory (i.e., the installation bundle root dir)
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." >/dev/null 2>&1 && pwd )"
PREFIX=${PREFIX:-/usr/local}

# RACE app
APP_PREFIX=${PREFIX}/lib/race/core/raceregistry
install --backup=numbered -D --mode=755 --target-directory=${DESTDIR}${APP_PREFIX}/bin/ ${DIR}/raceregistry/bin/*
install --backup=numbered -D --mode=644 --target-directory=${DESTDIR}${APP_PREFIX}/lib/ ${DIR}/raceregistry/lib/*.so
install --backup=numbered -D --mode=644 --target-directory=${DESTDIR}${APP_PREFIX}/lib/java ${DIR}/raceregistry/lib/java/*.jar
install --backup=numbered -D --mode=644 --target-directory=${DESTDIR}${APP_PREFIX}/lib/python ${DIR}/raceregistry/lib/python/*.py
install --backup=numbered -D --mode=644 --target-directory=${DESTDIR}${APP_PREFIX}/lib/python ${DIR}/raceregistry/lib/python/*.so

# RACE plugins (if present)
if (ls ${DIR}/artifacts/artifact-manager/* 2>&1) >/dev/null; then
    (cd $DIR/artifacts && find ./artifact-manager -type f -exec install --backup=numbered -D --mode=644 {} ${DESTDIR}${PREFIX}/lib/race/{} \;)
fi
if (ls ${DIR}/artifacts/network-manager/* 2>&1) >/dev/null; then
    (cd $DIR/artifacts && find ./network-manager -type f -exec install --backup=numbered -D --mode=644 {} ${DESTDIR}${PREFIX}/lib/race/{} \;)
fi
if (ls ${DIR}/artifacts/comms/* 2>&1) >/dev/null; then
    (cd $DIR/artifacts && find ./comms -type f -exec install --backup=numbered -D --mode=644 {} ${DESTDIR}${PREFIX}/lib/race/{} \;)
fi

# Configs (if present)
if (ls ${DIR}/configs.tar.gz 2>&1) >/dev/null; then
    (cd $DIR && find ./configs.tar.gz -type f -exec install --backup=numbered -D --mode=644 {} /tmp/{} \;)
fi
