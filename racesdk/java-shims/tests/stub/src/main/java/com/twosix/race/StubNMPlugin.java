//
// Copyright 2023 Two Six Technologies
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

package com.twosix.race;

import ShimsJava.ChannelStatus;
import ShimsJava.ConnectionStatus;
import ShimsJava.DeviceInfo;
import ShimsJava.IRacePluginNM;
import ShimsJava.JChannelProperties;
import ShimsJava.JClrMsg;
import ShimsJava.JEncPkg;
import ShimsJava.JLinkProperties;
import ShimsJava.JRaceSdkNM;
import ShimsJava.LinkDirection;
import ShimsJava.LinkStatus;
import ShimsJava.LinkType;
import ShimsJava.PackageStatus;
import ShimsJava.PluginConfig;
import ShimsJava.PluginResponse;
import ShimsJava.RaceHandle;
import ShimsJava.RaceLog;
import ShimsJava.SdkResponse;
import ShimsJava.SdkResponse.SdkStatus;
import ShimsJava.TransmissionType;

import java.util.Arrays;
import java.util.HashMap;

public class StubNMPlugin implements IRacePluginNM {

    static {
        try {
            System.loadLibrary("RaceJavaShims");
        } catch (Error e) {
            System.err.println("error loading library: " + e.getMessage());
        }
    }

    private JRaceSdkNM sdk;

    StubNMPlugin(JRaceSdkNM sdk) {
        this.sdk = sdk;
    }

    @Override
    public PluginResponse init(PluginConfig pluginConfig) {
        if ("/expected/global/path".equals(pluginConfig.etcDirectory)
                && "/expected/logging/path".equals(pluginConfig.loggingDirectory)
                && "/expected/aux-data/path".equals(pluginConfig.auxDataDirectory)) {

            byte[] entropy = sdk.getEntropy(2);
            if (entropy.length != 2 || entropy[0] != 0x01 || entropy[1] != 0x02) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "getEntropy test failed, received " + Arrays.toString(entropy),
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            String persona = sdk.getActivePersona();
            if (!"expected-persona".equals(persona)) {
                RaceLog.logError(
                        "StubNMPlugin", "getActivePersona test failed, received " + persona, "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse pluginUserInputResp =
                    sdk.requestPluginUserInput(
                            "expected-user-input-key", "expected-user-input-prompt", true);
            if (!checkSdkResponse(pluginUserInputResp)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "requestPluginUserInput test failed, received " + pluginUserInputResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse commonUserInputResp = sdk.requestCommonUserInput("expected-user-input-key");
            if (!checkSdkResponse(commonUserInputResp)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "requestCommonUserInput test failed, received " + commonUserInputResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse sendEncPkgResp =
                    sdk.sendEncryptedPackage(
                            new JEncPkg(0x1122113311441155l, 0x43214321l, new byte[] {0x42}),
                            "expected-conn-id",
                            0,
                            1);
            if (!checkSdkResponse(sendEncPkgResp)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "sendEncryptedPackage test failed, received " + sendEncPkgResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse presentClearResp =
                    sdk.presentCleartextMessage(
                            new JClrMsg(
                                    "expected-plaintext",
                                    "expected-from-persona",
                                    "expected-to-persona",
                                    0,
                                    0,
                                    (byte) (0),
                                    0,
                                    0));
            if (!checkSdkResponse(presentClearResp)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "presentCleartextMessage test failed, received " + presentClearResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse openConnResp =
                    sdk.openConnection(
                            LinkType.LT_SEND, "expected-link-id", "expected-link-hints", 7, 2, 3);
            if (!checkSdkResponse(openConnResp)) {
                RaceLog.logError(
                        "StubNMPlugin", "openConnection test failed, received " + openConnResp, "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse closeConnResp = sdk.closeConnection("expected-conn-id", 3);
            if (!checkSdkResponse(closeConnResp)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "closeConnection test failed, received " + closeConnResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            String[] links =
                    sdk.getLinksForPersonas(
                            new String[] {"expected-persona-1", "expected-persona-2"},
                            LinkType.LT_RECV);
            if (!Arrays.equals(new String[] {"expected-link-id-1", "expected-link-id-2"}, links)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "getLinksForPersonas test failed, received " + Arrays.toString(links),
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            String[] chanLinks = sdk.getLinksForChannel("expected-channel-gid");
            if (!Arrays.equals(new String[] {}, chanLinks)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "getLinksForChannel test failed, received " + Arrays.toString(chanLinks),
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            JLinkProperties props = sdk.getLinkProperties("expected-link-id");
            if (props == null
                    || !LinkType.LT_SEND.equals(props.linkType)
                    || !TransmissionType.TT_UNICAST.equals(props.transmissionType)) {
                RaceLog.logError(
                        "StubNMPlugin", "getLinkProperties test failed, received " + props, "");
                return PluginResponse.PLUGIN_ERROR;
            }

            JChannelProperties channelProps = sdk.getChannelProperties("expected-channel-gid");
            if (channelProps == null
                    || !LinkDirection.LD_CREATOR_TO_LOADER.equals(channelProps.linkDirection)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "getChannelProperties test failed, received " + channelProps,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            HashMap<String, JChannelProperties> supportedChannels = sdk.getSupportedChannels();
            if (supportedChannels == null
                    || !supportedChannels
                            .get("expected-channel-gid")
                            .linkDirection
                            .equals(LinkDirection.LD_CREATOR_TO_LOADER)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "getSupportedChannels test failed, received " + supportedChannels,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse deactivateChannelResp = sdk.deactivateChannel("expected-channel-gid", 3);
            if (!checkSdkResponse(deactivateChannelResp)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "deactivateChannel test failed, received " + deactivateChannelResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse destroyLinkResp = sdk.destroyLink("expected-link-id", 3);
            if (!checkSdkResponse(destroyLinkResp)) {
                RaceLog.logError(
                        "StubNMPlugin", "destroyLink test failed, received " + destroyLinkResp, "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse createLinkResp =
                    sdk.createLink("expected-channel-gid", new String[] {"expected-persona"}, 3);
            if (!checkSdkResponse(createLinkResp)) {
                RaceLog.logError(
                        "StubNMPlugin", "createLink test failed, received " + createLinkResp, "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse loadLinkAddressResp =
                    sdk.loadLinkAddress(
                            "expected-channel-gid",
                            "expected-link-address",
                            new String[] {"expected-persona"},
                            3);
            if (!checkSdkResponse(loadLinkAddressResp)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "loadLinkAddress test failed, received " + loadLinkAddressResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            String[] addresses = new String[] {"expected-link-address"};
            SdkResponse loadLinkAddressesResp =
                    sdk.loadLinkAddresses(
                            "expected-channel-gid",
                            addresses,
                            new String[] {"expected-persona"},
                            3);
            if (!checkSdkResponse(loadLinkAddressesResp)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "loadLinkAddresses test failed, received " + loadLinkAddressesResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            String[] personas = sdk.getPersonasForLink("expected-link-id");

            if (!Arrays.equals(
                    new String[] {"expected-persona-1", "expected-persona-2"}, personas)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "getPersonasForLink test failed, received " + Arrays.toString(personas),
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse setPersonasForLinkResp =
                    sdk.setPersonasForLink(
                            "expected-link-id",
                            new String[] {"expected-persona-1", "expected-persona-2"});
            if (!checkSdkResponse(setPersonasForLinkResp)) {
                RaceLog.logError(
                        "StubNMPlugin",
                        "setPersonasForLink test failed, received " + setPersonasForLinkResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }
            return PluginResponse.PLUGIN_OK;
        }
        return PluginResponse.PLUGIN_ERROR;
    }

    private boolean checkSdkResponse(SdkResponse response) {
        if (response != null
                && SdkStatus.SDK_OK.equals(response.getStatus())
                && new RaceHandle(0x1122334455667788l).equals(response.getHandle())
                && response.getQueueUtilization() == 0.15) {
            return true;
        }
        return false;
    }

    @Override
    public PluginResponse shutdown() {
        return PluginResponse.PLUGIN_OK;
    }

    @Override
    public PluginResponse processClrMsg(RaceHandle handle, JClrMsg msg) {
        if (new RaceHandle(0x8877665544332211l).equals(handle)
                && msg != null
                && "expected-message".equals(msg.plainMsg)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubNMPlugin", "processClrMsg test failed, received " + handle + ", " + msg, "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse processEncPkg(RaceHandle handle, JEncPkg ePkg, String[] connIDs) {
        byte[] expectedCipher = new byte[] {0x08, 0x67, 0x53, 0x09};
        String[] expectedConnIds = new String[] {"expected-conn-id-1", "expected-conn-id-2"};
        if (new RaceHandle(0x12345678l).equals(handle)
                && ePkg != null
                && ePkg.traceId == 0x1122113311441155l
                && ePkg.spanId == 0x12344321l
                && Arrays.equals(expectedCipher, ePkg.cipherText)
                && Arrays.equals(expectedConnIds, connIDs)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubNMPlugin",
                "processEncPkg test failed, received "
                        + handle
                        + ", "
                        + ePkg
                        + ", "
                        + Arrays.toString(connIDs),
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse prepareToBootstrap(
            RaceHandle handle, String linkId, String configPath, DeviceInfo deviceInfo) {
        if (new RaceHandle(0x1234l).equals(handle)
                && linkId.equals("link id")
                && configPath.equals("config path")
                && deviceInfo.getPlatform().equals("platform")
                && deviceInfo.getArchitecture().equals("architecture")
                && deviceInfo.getNodeType().equals("node type")) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubNMPlugin",
                "prepareToBootstrap test failed, received "
                        + handle
                        + ", "
                        + linkId
                        + ", "
                        + configPath
                        + ", "
                        + deviceInfo,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse onBootstrapKeyReceived(String persona, byte[] key) {
        String expectedPersona = "persona";
        byte[] expectedKey = new byte[] {8, 7, 6, 5, 4, 3, 2, 1};

        if (expectedPersona.equals(persona) && Arrays.equals(expectedKey, key)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubNMPlugin",
                "onBootstrapKeyReceived test failed, received "
                        + persona
                        + ", "
                        + Arrays.toString(key),
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse onPackageStatusChanged(RaceHandle handle, PackageStatus status) {
        if (new RaceHandle(0x11223344l).equals(handle)
                && PackageStatus.PACKAGE_RECEIVED.equals(status)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubNMPlugin",
                "onPackageStatusChanged test failed, received " + handle + ", " + status,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse onConnectionStatusChanged(
            RaceHandle handle,
            String connId,
            ConnectionStatus status,
            String linkId,
            JLinkProperties properties) {
        if (new RaceHandle(0x7777l).equals(handle)
                && "expected-conn-id".equals(connId)
                && ConnectionStatus.CONNECTION_OPEN.equals(status)
                && "expected-link-id".equals(linkId)
                && properties != null
                && LinkType.LT_SEND.equals(properties.linkType)
                && TransmissionType.TT_UNICAST.equals(properties.transmissionType)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubNMPlugin",
                "onConnectionStatusChanged test failed, received "
                        + handle
                        + ", "
                        + connId
                        + ", "
                        + status
                        + ", "
                        + linkId
                        + ", "
                        + properties,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse onLinkStatusChanged(
            RaceHandle handle, String linkId, LinkStatus status, JLinkProperties properties) {
        if (new RaceHandle(0x7777l).equals(handle)
                && "expected-link-id".equals(linkId)
                && LinkStatus.LINK_CREATED.equals(status)
                && properties != null
                && LinkType.LT_SEND.equals(properties.linkType)
                && TransmissionType.TT_MULTICAST.equals(properties.transmissionType)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubNMPlugin",
                "onLinkStatusChanged test failed, received "
                        + handle
                        + ", "
                        + linkId
                        + ", "
                        + status
                        + ", "
                        + properties,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse onChannelStatusChanged(
            RaceHandle handle,
            String channelGid,
            ChannelStatus status,
            JChannelProperties properties) {
        if (new RaceHandle(0x7777l).equals(handle)
                && "expected-channel-gid".equals(channelGid)
                && ChannelStatus.CHANNEL_AVAILABLE.equals(status)
                && properties != null
                && properties.linkDirection.equals(LinkDirection.LD_CREATOR_TO_LOADER)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubNMPlugin",
                "onChannelStatusChanged test failed, received "
                        + handle
                        + ", "
                        + channelGid
                        + ", "
                        + status
                        + ", "
                        + properties,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse onLinkPropertiesChanged(String linkId, JLinkProperties linkProperties) {
        if ("expected-link-id".equals(linkId)
                && linkProperties != null
                && LinkType.LT_RECV.equals(linkProperties.linkType)
                && TransmissionType.TT_MULTICAST.equals(linkProperties.transmissionType)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubNMPlugin",
                "onLinkPropertiesChanged test failed, received " + linkId + ", " + linkProperties,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse onPersonaLinksChanged(
            String recipientPersona, LinkType linkType, String[] links) {
        String[] expected = new String[] {"expected-link-id-1", "expected-link-id-2"};
        if ("expected-recipient".equals(recipientPersona)
                && LinkType.LT_BIDI.equals(linkType)
                && Arrays.equals(expected, links)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubNMPlugin",
                "onPersonaLinksChanged test failed, received "
                        + recipientPersona
                        + ", "
                        + linkType
                        + ", "
                        + Arrays.toString(links),
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse onUserInputReceived(
            RaceHandle handle, boolean answered, String response) {
        if (new RaceHandle(0x11223344l).equals(handle)
                && answered
                && "expected-user-input".equals(response)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubNMPlugin",
                "onUserInputReceived test failed, received "
                        + handle
                        + ", "
                        + answered
                        + ", "
                        + response,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse notifyEpoch(String data) {
        if (data.equals("expected-epoch-data")) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError("StubNMPlugin", "notifyEpoch test failed, received " + data, "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse onUserAcknowledgementReceived(RaceHandle handle) {
        return PluginResponse.PLUGIN_OK;
    }
}
