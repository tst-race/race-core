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
import ShimsJava.IRacePluginComms;
import ShimsJava.JChannelProperties;
import ShimsJava.JEncPkg;
import ShimsJava.JLinkProperties;
import ShimsJava.JRaceSdkComms;
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

public class StubCommsPlugin implements IRacePluginComms {

    static {
        try {
            System.loadLibrary("RaceJavaShims");
        } catch (Error e) {
            System.err.println("error loading library: " + e.getMessage());
        }
    }

    private JRaceSdkComms sdk;

    StubCommsPlugin(JRaceSdkComms sdk) {
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
                        "StubCommsPlugin",
                        "getEntropy test failed, received " + Arrays.toString(entropy),
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            String persona = sdk.getActivePersona();
            if (!"expected-persona".equals(persona)) {
                RaceLog.logError(
                        "StubCommsPlugin", "getActivePersona test failed, received " + persona, "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse pluginUserInputResp =
                    sdk.requestPluginUserInput(
                            "expected-user-input-key", "expected-user-input-prompt", true);
            if (!checkSdkResponse(pluginUserInputResp)) {
                RaceLog.logError(
                        "StubCommsPlugin",
                        "requestPluginUserInput test failed, received " + pluginUserInputResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse commonUserInputResp = sdk.requestCommonUserInput("expected-user-input-key");
            if (!checkSdkResponse(commonUserInputResp)) {
                RaceLog.logError(
                        "StubCommsPlugin",
                        "requestCommonUserInput test failed, received " + commonUserInputResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse pkgStatusResp =
                    sdk.onPackageStatusChanged(
                            new RaceHandle(0x8877665544332211l), PackageStatus.PACKAGE_SENT, 1);
            if (!checkSdkResponse(pkgStatusResp)) {
                RaceLog.logError(
                        "StubCommsPlugin",
                        "onPackageStatusChanged test failed, received " + pkgStatusResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            JLinkProperties props = new JLinkProperties();
            props.linkType = LinkType.LT_SEND;
            props.transmissionType = TransmissionType.TT_UNICAST;
            SdkResponse connStatusResp =
                    sdk.onConnectionStatusChanged(
                            new RaceHandle(0x12345678l),
                            "expected-conn-id",
                            ConnectionStatus.CONNECTION_CLOSED,
                            props,
                            2);
            if (!checkSdkResponse(connStatusResp)) {
                RaceLog.logError(
                        "StubCommsPlugin",
                        "onConnectionStatusChanged test failed, received " + connStatusResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse linkStatusResp =
                    sdk.onLinkStatusChanged(
                            new RaceHandle(0x12345678l),
                            "expected-link-id",
                            LinkStatus.LINK_DESTROYED,
                            props,
                            2);
            if (!checkSdkResponse(linkStatusResp)) {
                RaceLog.logError(
                        "StubCommsPlugin",
                        "onLinkStatusChanged test failed, received " + linkStatusResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            JChannelProperties channelProps = new JChannelProperties();
            channelProps.channelGid = "expected-channel-gid";
            SdkResponse channelStatusResp =
                    sdk.onChannelStatusChanged(
                            new RaceHandle(0x12345678l),
                            "expected-channel-gid",
                            ChannelStatus.CHANNEL_AVAILABLE,
                            channelProps,
                            2);
            if (!checkSdkResponse(channelStatusResp)) {
                RaceLog.logError(
                        "StubCommsPlugin",
                        "onChannelStatusChanged test failed, received " + channelStatusResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            String[] tags = new String[] {"expected-tag1", "expected-tag2"};

            props.linkType = LinkType.LT_BIDI;
            props.transmissionType = TransmissionType.TT_MULTICAST;
            SdkResponse updLinkResp = sdk.updateLinkProperties("expected-link-id", props, 3);
            if (!checkSdkResponse(updLinkResp)) {
                RaceLog.logError(
                        "StubCommsPlugin",
                        "updateLinkProperties test failed, received " + updLinkResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            String connId = sdk.generateConnectionId("expected-link-id");
            if (!"expected-conn-id".equals(connId)) {
                RaceLog.logError(
                        "StubCommsPlugin",
                        "generateConnectionId test failed, received " + connId,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            String linkId = sdk.generateLinkId("expected-channel-gid");
            if (!"expected-channel-gid/expected-link-id".equals(linkId)) {
                RaceLog.logError(
                        "StubCommsPlugin", "generateLinkId test failed, received " + linkId, "");
                return PluginResponse.PLUGIN_ERROR;
            }

            JEncPkg encPkg =
                    new JEncPkg(
                            0x1122334455667788l,
                            0x2211331144115511l,
                            new byte[] {0x08, 0x67, 0x53, 0x09});
            SdkResponse rcvEncPkgResp =
                    sdk.receiveEncPkg(
                            encPkg, new String[] {"expected-conn-id-1", "expected-conn-id-2"}, 4);
            if (!checkSdkResponse(rcvEncPkgResp)) {
                RaceLog.logError(
                        "StubCommsPlugin",
                        "receiveEncPkg test failed, received " + rcvEncPkgResp,
                        "");
                return PluginResponse.PLUGIN_ERROR;
            }

            SdkResponse unblockQueueResp = sdk.unblockQueue("expected-conn-id");
            if (!checkSdkResponse(unblockQueueResp)) {
                RaceLog.logError(
                        "StubCommsPlugin",
                        "unblockQueue test failed, received " + unblockQueueResp,
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
    public PluginResponse sendPackage(
            RaceHandle handle,
            String connectionId,
            JEncPkg pkg,
            double timeoutTimestamp,
            long batchId) {
        byte[] expected = new byte[] {0x08, 0x67, 0x53, 0x09};
        if (new RaceHandle(0x8877665544332211l).equals(handle)
                && "expected-conn-id".equals(connectionId)
                && pkg != null
                && pkg.traceId == 0x0011223344556677l
                && pkg.spanId == 0x2211331144115511l
                && Arrays.equals(expected, pkg.cipherText)
                && timeoutTimestamp == Double.POSITIVE_INFINITY
                && batchId == 6789) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubCommsPlugin",
                "sendPackage test failed, received "
                        + handle
                        + ", "
                        + connectionId
                        + ", "
                        + pkg
                        + ", "
                        + batchId,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse openConnection(
            RaceHandle handle,
            LinkType linkType,
            String linkId,
            String linkHints,
            int sendTimeout) {
        if (new RaceHandle(0x03).equals(handle)
                && LinkType.LT_RECV.equals(linkType)
                && "expected-link-id".equals(linkId)
                && "expected-link-hints".equals(linkHints)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubCommsPlugin",
                "openConnection test failed, received "
                        + handle
                        + ", "
                        + linkType
                        + ", "
                        + linkId
                        + ", "
                        + linkHints,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse closeConnection(RaceHandle handle, String connectionId) {
        if (new RaceHandle(0x12345678).equals(handle) && "expected-conn-id".equals(connectionId)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubCommsPlugin",
                "closeConnection test failed, received " + handle + ", " + connectionId,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse deactivateChannel(RaceHandle handle, String channelGid) {
        if (new RaceHandle(0x12345678).equals(handle)
                && "expected-channel-gid".equals(channelGid)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubCommsPlugin",
                "deactivateChannel test failed, received " + handle + ", " + channelGid,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse activateChannel(RaceHandle handle, String channelGid, String roleName) {
        if (new RaceHandle(0x42).equals(handle)
                && "expected-channel-gid".equals(channelGid)
                && "expected-role-name".equals(roleName)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubCommsPlugin",
                "activateChannel test failed, received " + handle + ", " + channelGid,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse destroyLink(RaceHandle handle, String linkId) {
        if (new RaceHandle(0x12345678).equals(handle) && "expected-link-id".equals(linkId)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubCommsPlugin",
                "destroyLink test failed, received " + handle + ", " + linkId,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse createLink(RaceHandle handle, String channelGid) {
        if (new RaceHandle(0x03).equals(handle) && "expected-channel-gid".equals(channelGid)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubCommsPlugin",
                "createLink test failed, received " + handle + ", " + channelGid,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse createLinkFromAddress(
            RaceHandle handle, String channelGid, String linkAddress) {
        if (new RaceHandle(0x03).equals(handle)
                && "expected-channel-gid".equals(channelGid)
                && "expected-link-address".equals(linkAddress)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubCommsPlugin",
                "createLinkFromAddress test failed, received "
                        + handle
                        + ", "
                        + channelGid
                        + ", "
                        + linkAddress,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse loadLinkAddress(
            RaceHandle handle, String channelGid, String linkAddress) {
        if (new RaceHandle(0x03).equals(handle)
                && "expected-channel-gid".equals(channelGid)
                && "expected-link-address".equals(linkAddress)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubCommsPlugin",
                "loadLinkAddress test failed, received "
                        + handle
                        + ", "
                        + channelGid
                        + ", "
                        + linkAddress,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse loadLinkAddresses(
            RaceHandle handle, String channelGid, String[] linkAddresses) {
        String[] expectedAddresses =
                new String[] {"expected-link-address1", "expected-link-address2"};
        if (new RaceHandle(0x03).equals(handle)
                && "expected-channel-gid".equals(channelGid)
                && Arrays.equals(expectedAddresses, linkAddresses)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubCommsPlugin",
                "loadLinkAddresses test failed, received "
                        + handle
                        + ", "
                        + channelGid
                        + ", "
                        + linkAddresses,
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
                "StubCommsPlugin",
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
    public PluginResponse flushChannel(RaceHandle handle, String connId, long batchId) {
        if (new RaceHandle(0x4321l).equals(handle)
                && "connection-id-for-flush".equals(connId)
                && batchId == 27) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubCommsPlugin",
                "flushChannel test failed, received " + handle + ", " + connId + ", " + batchId,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse serveFiles(String linkId, String path) {
        if ("link-id-for-serveFiles".equals(linkId)
                && "/some/path/of/files/to/serve".equals(path)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubCommsPlugin", "serveFiles test failed, received " + linkId + ", " + path, "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse createBootstrapLink(
            RaceHandle handle, String channelGid, String passphrase) {
        if (new RaceHandle(0x654321l).equals(handle)
                && "channel-gid-for-createBootstrapLink".equals(channelGid)
                && "passphrase-for-createBootstrapLink".equals(passphrase)) {
            return PluginResponse.PLUGIN_OK;
        }
        RaceLog.logError(
                "StubCommsPlugin",
                "createBootstrapLink test failed, received "
                        + handle
                        + ", "
                        + channelGid
                        + ", "
                        + passphrase,
                "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse onUserAcknowledgementReceived(RaceHandle handle) {
        return PluginResponse.PLUGIN_OK;
    }
}
