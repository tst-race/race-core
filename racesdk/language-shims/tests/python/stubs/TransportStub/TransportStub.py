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


from sqlite3 import paramstyle
import sys
from tokenize import Double

sys.path.append("../../source/include/")
sys.path.append("../../source/")

from commsPluginBindings import (
    ITransportComponent,
    TransportProperties,
    Event,
    EncodingParameters,
    LinkProperties,
    LinkPropertySet,
    LinkPropertyPair,
    LinkParameters,
    COMPONENT_OK,
    COMPONENT_ERROR,
    LT_BIDI,
    TT_MULTICAST,
    CT_LOCAL,
    ST_EPHEM_SYNC,
    LINK_CREATED,
    PACKAGE_SENT,
)


class TransportStub(ITransportComponent):
    def __init__(self, sdk):
        super().__init__()
        self.sdk = sdk

    def getTransportProperties(self):
        prop = TransportProperties()
        supportedActions = {
            "action1": ["application/octet-stream", "text/plain", "image/*"]
        }
        prop.supportedActions = supportedActions
        return prop

    def getLinkProperties(self, linkId):
        if linkId == "link_1":

            ps = LinkPropertySet()
            ps.bandwidth_bps = 101
            ps.latency_ms = 202
            ps.loss = 0.5

            pp = LinkPropertyPair()
            pp.send = ps
            pp.receive = ps

            # Call function in the SDK to test binding in reverse direction
            self.sdk.getChannelProperties()

            prop = LinkProperties()
            prop.linkType = LT_BIDI
            prop.transmissionType = TT_MULTICAST
            prop.connectionType = CT_LOCAL
            prop.sendType = ST_EPHEM_SYNC
            prop.reliable = True
            prop.isFlushable = True
            prop.duration_s = 10101
            prop.period_s = 20202
            prop.mtu = 30303
            prop.worst = pp
            prop.expected = pp
            prop.best = pp
            prop.supported_hints = ["hint1"]
            prop.channelGid = "mockChannel"
            prop.linkAddress = "mockLinkAddress"
            return prop
        return None

    def createLink(self, handle, linkId):
        if (handle == 1) and (linkId == "link_1"):
            params = LinkParameters()
            params.json = "{}"
            self.sdk.onLinkStatusChanged(handle, linkId, LINK_CREATED, params)
            return COMPONENT_OK
        return COMPONENT_ERROR

    def loadLinkAddress(self, handle, linkId, linkAddr):
        if (handle == 1) and (linkId == "link_1") and (linkAddr == "linkAddress_1"):
            return COMPONENT_OK
        return COMPONENT_ERROR

    def loadLinkAddresses(self, handle, linkId, linkAddrs):
        if (handle == 1) and (linkId == "link_1") and (len(linkAddrs) == 2):
            return COMPONENT_OK
        return COMPONENT_ERROR

    def createLinkFromAddress(self, handle, linkId, linkAddr):
        if (handle == 1) and (linkId == "link_1") and (linkAddr == "linkAddress_1"):
            return COMPONENT_OK
        return COMPONENT_ERROR

    def destroyLink(self, handle, linkId):
        if (handle == 1) and (linkId == "link_1"):
            return COMPONENT_OK
        return COMPONENT_ERROR

    def getActionParams(self, action):
        if action.timestamp == 1 and action.actionId == 0x10 and action.json == "{}":
            params = EncodingParameters()
            params.linkId = "link_1"
            params.type = "application/octet-stream"
            params.encodePackage = False
            params.json = "{}"
            return [params]
        return None

    def enqueueContent(self, params, action, content):
        if (
            params.linkId == "link_1"
            and action.timestamp == 1
            and action.actionId == 0x10
            and action.json == "{}"
            and len(content) == 3
        ):
            self.sdk.onReceive("link_1", EncodingParameters(), [0x01, 0x02])
            return COMPONENT_OK
        return COMPONENT_ERROR

    def dequeueContent(self, action):
        if action.actionId == 0x10:
            self.sdk.onPackageStatusChanged(1, PACKAGE_SENT)
            return COMPONENT_OK
        return COMPONENT_ERROR

    def doAction(self, handles, action):
        if len(handles) == 3 and action.actionId == 0x10:
            return COMPONENT_OK
        return COMPONENT_ERROR

    def onUserInputReceived(self, handle, answered, response):
        if (handle == 3) and (answered) and (response == "response"):
            self.sdk.onEvent(Event())
            return COMPONENT_OK
        return COMPONENT_ERROR


def createTransport(name, sdk, role, plugin):
    if (
        name == "twoSixStubTransportPy"
        and role == "roleName"
        and plugin.tmpDirectory == "/tmp"
    ):
        return TransportStub(sdk)
    return None
