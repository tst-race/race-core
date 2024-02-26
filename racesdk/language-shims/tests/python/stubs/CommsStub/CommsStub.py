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
        Stub Comms
"""

import sys

sys.path.append("../../source/include/")
sys.path.append("../../source/")

# Python Library Imports
from typing import List, Optional

# Swig Libraries
from commsPluginBindings import (
    ChannelProperties,
    EncPkg,
    IRacePluginComms,
    IRaceSdkComms,
    LinkProperties,
    PluginConfig,
    RaceLog,
    SdkResponse,
)
from commsPluginBindings import BS_COMPLETE  # BootstrapActionType
from commsPluginBindings import CHANNEL_AVAILABLE  # ChannelStatus
from commsPluginBindings import CONNECTION_CLOSED  # ConnectionStatus
from commsPluginBindings import LT_BIDI, LT_SEND  # LinkType
from commsPluginBindings import LINK_DESTROYED  # LinkStatus
from commsPluginBindings import PACKAGE_SENT  # PackageStatus
from commsPluginBindings import PLUGIN_OK, PLUGIN_ERROR  # PluginResponse
from commsPluginBindings import SDK_OK  # SdkResponse
from commsPluginBindings import TT_MULTICAST, TT_UNICAST  # TransmissionType
from commsPluginBindings import UD_QR_CODE, UD_TOAST  # UserDisplayType

PluginResponse = int
Handle = int
LinkType = int


def logError(message):
    RaceLog.logError("CommsStub", message, "")


def check_sdk_response(response: Optional[SdkResponse]) -> bool:
    if (
        response
        and response.status == SDK_OK
        and response.handle == 0x1122334455667788
        and response.queueUtilization == 0.15
    ):
        return True
    return False


class PluginCommsTwoSixPy(IRacePluginComms):
    """A Comms stub"""

    def __init__(self, sdk: Optional[IRaceSdkComms]):
        super().__init__()

        if sdk is None:
            raise Exception("sdk was not passed to plugin")
        self.sdk = sdk

    def init(self, plugin_config: PluginConfig) -> PluginResponse:
        if plugin_config.etcDirectory != "/expected/etc/path":
            logError(
                f"PluginConfig.etcDirectory test failed, received {plugin_config.etcDirectory}"
            )
            return PLUGIN_ERROR

        if plugin_config.loggingDirectory != "/expected/logging/path":
            logError(
                f"PluginConfig.loggingDirectory test failed, received {plugin_config.loggingDirectory}"
            )
            return PLUGIN_ERROR

        if plugin_config.auxDataDirectory != "/expected/auxData/path":
            logError(
                f"PluginConfig.auxDataDirectory test failed, received {plugin_config.auxDataDirectory}"
            )
            return PLUGIN_ERROR

        if plugin_config.tmpDirectory != "/expected/tmp/path":
            logError(
                f"PluginConfig.tmpDirectory test failed, received {plugin_config.tmpDirectory}"
            )
            return PLUGIN_ERROR

        entropy = self.sdk.getEntropy(2)
        if len(entropy) != 2 or entropy[0] != 0x01 or entropy[1] != 0x02:
            logError(f"getEntropy test failed, received {entropy}")
            return PLUGIN_ERROR

        persona = self.sdk.getActivePersona()
        if persona != "expected-persona":
            logError(f"getactivePersona test failed, received {persona}")
            return PLUGIN_ERROR

        plugin_user_input_resp = self.sdk.requestPluginUserInput(
            "expected-user-input-key", "expected-user-input-prompt", True
        )
        if not check_sdk_response(plugin_user_input_resp):
            logError(
                f"requestPluginUserInput test failed, received {plugin_user_input_resp}"
            )
            return PLUGIN_ERROR

        common_user_input_resp = self.sdk.requestCommonUserInput(
            "expected-user-input-key"
        )
        if not check_sdk_response(common_user_input_resp):
            logError(
                f"requestCommonUserInput test failed, received {common_user_input_resp}"
            )
            return PLUGIN_ERROR

        disp_info_resp = self.sdk.displayInfoToUser("expected-message", UD_TOAST)
        if not check_sdk_response(disp_info_resp):
            logError(f"displayInfoToUser test failed, received {disp_info_resp}")
            return PLUGIN_ERROR

        disp_bootstrap_info_resp = self.sdk.displayBootstrapInfoToUser(
            "expected-message", UD_QR_CODE, BS_COMPLETE
        )
        if not check_sdk_response(disp_bootstrap_info_resp):
            logError(
                f"displayBootstrapInfoToUser test failed, received {disp_bootstrap_info_resp}"
            )
            return PLUGIN_ERROR

        pkg_status_resp = self.sdk.onPackageStatusChanged(
            0x8877665544332211, PACKAGE_SENT, 1
        )
        if not check_sdk_response(pkg_status_resp):
            logError(f"onPackageStatusChanged test failed, received {pkg_status_resp}")
            return PLUGIN_ERROR

        link_props = LinkProperties()
        link_props.linkType = LT_SEND
        link_props.transmissionType = TT_UNICAST
        conn_status_resp = self.sdk.onConnectionStatusChanged(
            0x12345678, "expected-conn-id", CONNECTION_CLOSED, link_props, 2
        )
        if not check_sdk_response(conn_status_resp):
            logError(
                f"onConnectionStatusChanged test failed, received {conn_status_resp}"
            )
            return PLUGIN_ERROR

        link_status_resp = self.sdk.onLinkStatusChanged(
            0x12345678, "expected-link-id", LINK_DESTROYED, link_props, 2
        )
        if not check_sdk_response(link_status_resp):
            logError(f"onLinkStatusChanged test failed, received {link_status_resp}")
            return PLUGIN_ERROR

        channel_props = ChannelProperties()
        channel_props.channelGid = "expected-channel-gid"
        channel_props.maxSendsPerInterval = 42
        channel_props.secondsPerInterval = 3600
        channel_props.intervalEndTime = 8675309
        channel_props.sendsRemainingInInterval = 7
        channel_status_resp = self.sdk.onChannelStatusChanged(
            0x12345678, "expected-channel-gid", CHANNEL_AVAILABLE, channel_props, 3
        )
        if not check_sdk_response(channel_status_resp):
            logError(
                f"onChannelStatusChanged test failed, received {channel_status_resp}"
            )
            return PLUGIN_ERROR

        link_props.linkType = LT_BIDI
        link_props.transmissionType = TT_MULTICAST
        upd_link_resp = self.sdk.updateLinkProperties("expected-link-id", link_props, 4)
        if not check_sdk_response(upd_link_resp):
            logError(f"updateLinkProperties test failed, received {upd_link_resp}")
            return PLUGIN_ERROR

        conn_id = self.sdk.generateConnectionId("expected-link-id")
        if conn_id != "expected-conn-id":
            logError(f"generateConnectionId test failed, received {conn_id}")
            return PLUGIN_ERROR

        link_id = self.sdk.generateLinkId("expected-channel-gid")
        if link_id != "expected-channel-gid/expected-link-id":
            logError(f"generateLinkId test failed, received {link_id}")
            return PLUGIN_ERROR

        enc_pkg = EncPkg(
            0x1122334455667788, 0x2211331144115511, [0x08, 0x67, 0x53, 0x09]
        )
        recv_enc_pkg_resp = self.sdk.receiveEncPkg(
            enc_pkg, ["expected-conn-id-1", "expected-conn-id-2"], 5
        )
        if not check_sdk_response(recv_enc_pkg_resp):
            logError(f"receiveEncPkg test failed, received {recv_enc_pkg_resp}")
            return PLUGIN_ERROR

        # Can't test this one, it's a method on the CommsWrapper itself rather than RaceSdk
        # unblock_queue_resp = self.sdk.unblockQueue("expected-conn-id")
        # if not check_sdk_response(unblock_queue_resp):
        #     logError(f"unblockQueue test failed, received {unblock_queue_resp}")
        #     return PLUGIN_ERROR

        return PLUGIN_OK

    def shutdown(self) -> PluginResponse:
        pass

    def sendPackage(
        self,
        handle: Handle,
        conn_id: str,
        enc_pkg: EncPkg,
        timeout_timestamp: float,
        batch_id: int,
    ) -> PluginResponse:
        pass

    def openConnection(
        self,
        handle: Handle,
        link_type: LinkType,
        link_id: str,
        link_hints: str,
        send_timeout: int,
    ) -> PluginResponse:
        pass

    def closeConnection(self, handle: Handle, conn_id: str) -> PluginResponse:
        pass

    def destroyLink(self, handle: Handle, link_id: str) -> PluginResponse:
        pass

    def createLink(self, handle: Handle, channel_gid: str) -> PluginResponse:
        pass

    def loadLinkAddress(
        self, handle: Handle, channel_gid: str, link_address: str
    ) -> PluginResponse:
        pass

    def loadLinkAddresses(
        self, handle: Handle, channel_gid: str, link_addresses: List[str]
    ) -> PluginResponse:
        pass

    def createLinkFromAddress(
        self, handle: Handle, channel_gid: str, link_address: str
    ) -> PluginResponse:
        pass

    def deactivateChannel(self, handle: Handle, channel_gid: str) -> PluginResponse:
        pass

    def activateChannel(
        self, handle: Handle, channel_gid: str, role_name: str
    ) -> PluginResponse:
        pass

    def onUserInputReceived(
        self, handle: Handle, answered: bool, response: str
    ) -> PluginResponse:
        pass

    def onUserAcknowledgementReceived(self, handle: int) -> PluginResponse:
        pass

    def serveFiles(self, link_id: str, path: str) -> PluginResponse:
        pass

    def createBootstrapLink(
        self, handle: Handle, channel_gid: str, passphrase: str
    ) -> PluginResponse:
        pass

    def flushChannel(
        self, handle: int, channel_gid: str, batch_id: int
    ) -> PluginResponse:
        pass
