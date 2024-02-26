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

"""
    Purpose:
        Stub network manager
"""

import sys

sys.path.append("../../source/include/")
sys.path.append("../../source/")

# Swig Libraries
from networkManagerPluginBindings import (
    ByteVector,
    EncPkg,
    IRacePluginNM,
    IRaceSdkNM,
    IRaceSdkApp,
    LinkProperties,
    ChannelProperties,
    CHANNEL_AVAILABLE,
)
from networkManagerPluginBindings import CONNECTION_CLOSED, CONNECTION_OPEN  # ConnectionStatus
from networkManagerPluginBindings import (
    LINK_CREATED,
    LINK_DESTROYED,
    LINK_LOADED,
)  # LinkStatus
from networkManagerPluginBindings import LT_RECV, LT_SEND, LT_UNDEF  # LinkTypes
from networkManagerPluginBindings import PLUGIN_ERROR, PLUGIN_FATAL, PLUGIN_OK  # PluginResponse
from networkManagerPluginBindings import CT_UNDEF  # ConnectionTypes
from networkManagerPluginBindings import RACE_UNLIMITED  # timeout constant
from networkManagerPluginBindings import RACE_BLOCKING
from networkManagerPluginBindings import PLUGIN_READY  # PluginStatus
from networkManagerPluginBindings import NULL_RACE_HANDLE

from networkManagerPluginBindings import UD_TOAST  # UserDisplayTypes

from networkManagerPluginBindings import PluginConfig, ClrMsg

# from typing import Dict, List, Set, Tuple
from typing import Dict, List, Tuple
from tokenize import Double

PluginResponse = int


class PluginNMTwoSixPy(IRacePluginNM):
    """
    A network manager stub
    """

    def __init__(self, sdk: IRaceSdkNM):
        """ """
        super().__init__()

        if sdk is None:
            raise Exception("sdk was not passed to plugin")
        self._race_sdk = sdk

    def init(self, plugin_config: PluginConfig) -> PluginResponse:
        """ """
        res = self._race_sdk.getLinksForChannel("stubPy")
        if res[0] == "stubPy:link":
            return PLUGIN_OK
        else:
            return PLUGIN_ERROR

    def shutdown(self, plugin_config: PluginConfig) -> PluginResponse:
        """ """
        return PLUGIN_OK

    def processClrMsg(self, handle: int, msg: ClrMsg) -> PluginResponse:
        """ """
        return PLUGIN_OK

    def processEncPkg(
        self, handle: int, ePkg: EncPkg, conn_ids: List[str]
    ) -> PluginResponse:
        """ """
        return PLUGIN_OK

    def prepareToBootstrap(
        self, handle: int, link_id: str, config_path: str, device_info: Dict
    ):
        if (
            (link_id == "link1")
            and (config_path == "config/")
            and (device_info.platform == "linux")
            and (device_info.architecture == "x86_64")
            and (device_info.nodeType == "client")
        ):
            return PLUGIN_OK
        return PLUGIN_ERROR

    def onBootstrapPkgReceived(self, persona: str, pkg: bytearray):
        """ """
        if (persona == "persona1") and (pkg[0] == 0x01):
            return PLUGIN_OK
        return PLUGIN_ERROR

    def onPackageStatusChanged(self, handle: int, status: int) -> PluginResponse:
        """ """
        return PLUGIN_OK

    def onConnectionStatusChanged(
        self,
        handle: int,
        conn_id: str,
        status: int,
        link_id: str,
        properties: LinkProperties,
    ) -> PluginResponse:
        """ """
        return PLUGIN_OK

    def onLinkPropertiesChanged(
        self, link_id: str, link_properties: LinkProperties
    ) -> PluginResponse:
        """ """
        return PLUGIN_OK

    def onPersonaLinksChanged(
        self, recipient_persona: str, linkType: int, links: List[str]
    ) -> PluginResponse:
        """ """
        return PLUGIN_OK

    def onChannelStatusChanged(
        self, handle: int, channelGid: str, status: int, properties: ChannelProperties
    ):
        """ """
        return PLUGIN_OK

    def onLinkStatusChanged(
        self, handle: int, linkId: str, status: int, properties: LinkProperties
    ):
        """ """
        return PLUGIN_OK

    def onUserInputReceived(
        self,
        handle: int,
        answered: bool,
        response: str,
    ) -> PluginResponse:
        """ """
        if (handle == 1) and (answered == True) and (response == "hello"):
            return PLUGIN_OK
        else:
            return PLUGIN_ERROR

    def onUserAcknowledgementReceived(
        self,
        handle: int,
    ) -> int:
        """ """
        logDebug(f"onUserAcknowledgementReceived: called")
        return PLUGIN_OK

    def notifyEpoch(self, data: str) -> PluginResponse:
        """ """
        logDebug(f"notifyEpoch: called with data={data}")
        return PLUGIN_OK
