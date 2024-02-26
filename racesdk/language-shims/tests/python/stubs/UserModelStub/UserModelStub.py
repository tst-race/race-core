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
    Action,
    IUserModelComponent,
    UserModelProperties,
    LinkParameters,
    COMPONENT_OK,
    COMPONENT_ERROR,
)


class UserModelStub(IUserModelComponent):
    def __init__(self, sdk):
        super().__init__()
        self.sdk = sdk

    def getUserModelProperties(self):
        prop = UserModelProperties()
        return prop

    def addLink(self, linkId, params):
        if linkId == "link_1" and params.json == "{}":
            self.sdk.onTimelineUpdated()
            return COMPONENT_OK
        return COMPONENT_ERROR

    def removeLink(self, linkId):
        if linkId == "link_1":
            return COMPONENT_OK
        return COMPONENT_ERROR

    def getTimeline(self, start, end):
        if start == 1000000.0 and end == 2000000.0:
            action = Action()
            action.timestamp = 1000000.0
            action.actionId = 0x10
            action.json = "{}"
            tl = [action]
            return tl
        return []

    def onTransportEvent(self, event):
        if event.json == "{}":
            return COMPONENT_OK
        return COMPONENT_ERROR

    def onUserInputReceived(self, handle, answered, response):
        if (handle == 3) and (answered) and (response == "response"):
            return COMPONENT_OK
        return COMPONENT_ERROR


def createUserModel(name, sdk, role, plugin):
    if (
        name == "twoSixStubUserModelPy"
        and role == "roleName"
        and plugin.tmpDirectory == "/tmp"
    ):
        return UserModelStub(sdk)
    return None
