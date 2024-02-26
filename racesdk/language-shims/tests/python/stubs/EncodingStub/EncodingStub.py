#!/usr/bin/env python3

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


import sys
from tokenize import Double

sys.path.append("../../source/include/")
sys.path.append("../../source/")

from commsPluginBindings import (
    IEncodingComponent,
    EncodingProperties,
    COMPONENT_OK,
    COMPONENT_ERROR,
    ENCODE_OK,
)


class EncodingStub(IEncodingComponent):
    def __init__(self, sdk):
        super().__init__()
        self.sdk = sdk

    def getEncodingProperties(self):
        prop = EncodingProperties()
        prop.encodingTime = 1010101
        prop.type = "text/plain"
        return prop

    def encodeBytes(self, handle, params, bytes):
        if (
            (handle == 1)
            and (params.linkId == "linkID_1")
            and (params.type == "application/octet-stream")
            and (params.encodePackage == False)
            and (params.json == "{}")
            and (len(bytes) == 3)
        ):
            self.sdk.onBytesEncoded(0, bytes, ENCODE_OK)
            return COMPONENT_OK
        return COMPONENT_ERROR

    def decodeBytes(self, handle, params, bytes):
        if (
            (handle == 2)
            and (params.linkId == "linkID_2")
            and (params.type == "application/octet-stream")
            and (params.encodePackage == False)
            and (params.json == "{}")
            and (len(bytes) == 3)
        ):
            self.sdk.onBytesDecoded(0, bytes, ENCODE_OK)
            return COMPONENT_OK
        return COMPONENT_ERROR

    def onUserInputReceived(self, handle, answered, response):
        if (handle == 3) and (answered) and (response == "response"):
            return COMPONENT_OK
        return COMPONENT_ERROR


def createEncoding(name, sdk, role, plugin):
    if (
        name == "twoSixStubEncodingPy"
        and role == "roleName"
        and plugin.tmpDirectory == "/tmp"
    ):
        return EncodingStub(sdk)
    return None
