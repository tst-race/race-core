
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

package race;

import ShimsJava.ChannelStatus;
import ShimsJava.ConnectionStatus;
import ShimsJava.IRacePluginComms;
import ShimsJava.JEncPkg;
import ShimsJava.JLinkConfig;
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
import ShimsJava.UserDisplayType;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Vector;

public class PluginCommsTwoSixJava implements IRacePluginComms {

    private static String logLabel = "Java Comms Plugin";
    private JRaceSdkComms jSdk;
    String racePersona;

    // Map of linkIds to Links
    Map<String, Link> links = new HashMap<>();
    // Map of connectionIds to Connections
    Map<String, Connection> connections = new HashMap<>();

    // Map of channel GID keys to their respective status values.
    Map<String, ChannelStatus> channelStatuses =
            new HashMap<String, ChannelStatus>() {
                {
                    put(Channels.directChannelGid, ChannelStatus.CHANNEL_UNAVAILABLE);
                    put(Channels.indirectChannelGid, ChannelStatus.CHANNEL_UNAVAILABLE);
                }
            };

    // The next available port for creating direct links.
    int nextAvailablePort = 10000;
    // The whiteboard hostname for indirect channel.
    String whiteboardHostname = "twosix-whiteboard";
    // The whiteboard port for indirect channel.
    int whiteboardPort = 5000;
    // The next available hashtag for creating indirect links.
    int nextAvailableHashtag = 0;

    // The node hostname to use with direct links
    String hostname = "no-hostname-provided-by-user";
    RaceHandle requestHostnameHandle;
    RaceHandle requestStartPortHandle;

    Set<RaceHandle> userInputRequests = new HashSet<RaceHandle>();

    /** @return the jSdk */
    public JRaceSdkComms getjSdk() {
        return jSdk;
    }

    PluginCommsTwoSixJava(JRaceSdkComms sdk) {
        RaceLog.logDebug(logLabel, "PluginCommsTwoSixJava constructor called", "");
        jSdk = sdk;
        RaceLog.logDebug(logLabel, "PluginCommsTwoSixJava constructor returned", "");
    }

    /**
     * Initialize the plugin. Set the RaceSdk object and other prep work to begin allowing calls
     * from core and other plugins.
     *
     * @param pluginConfig Config object containing dynamic config variables (e.g. paths)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    @Override
    public PluginResponse init(PluginConfig pluginConfig) {
        RaceLog.logDebug(logLabel, "inside init", "");
        RaceLog.logDebug(
                logLabel,
                " Plugin Config: "
                        + "{ etcDirectory: \""
                        + pluginConfig.etcDirectory
                        + "\", loggingDirectory: \""
                        + pluginConfig.loggingDirectory
                        + "\", auxDataDirectory: \""
                        + pluginConfig.auxDataDirectory
                        + "\", tmpDirectory: \""
                        + pluginConfig.tmpDirectory
                        + "\", pluginDirectory: \""
                        + pluginConfig.pluginDirectory
                        + "\" }",
                "");

        // Configuring Persona
        racePersona = jSdk.getActivePersona();
        RaceLog.logDebug(logLabel, "    active persona: " + racePersona, "");

        SdkResponse response =
                jSdk.writeFile("initialized.txt", "Comms Java Plugin Initialized\n".getBytes());
        if (response == null || response.getStatus() != SdkResponse.SdkStatus.SDK_OK) {
            RaceLog.logWarning(logLabel, "Failed to write to plugin storage", "");
        }
        byte[] readMsg = jSdk.readFile("initialized.txt");
        if (readMsg != null) {
            RaceLog.logDebug(logLabel, new String(readMsg), "");
        }

        RaceLog.logDebug(logLabel, "ending init", "");
        return PluginResponse.PLUGIN_OK;
    }

    /**
     * Activate a specific channel
     *
     * @param raceHandle The RaceHandle to use for activateChannel calls
     * @param channelGid The channel to activate
     * @param roleName The role to activate
     * @return PluginResponse the status of the Plugin in response to this call
     */
    @Override
    public PluginResponse activateChannel(RaceHandle handle, String channelGid, String roleName) {
        RaceLog.logDebug(logLabel, "activateChannel called for " + channelGid, "");
        if (channelGid.compareTo(Channels.indirectChannelGid) == 0) {
            this.channelStatuses.put(channelGid, ChannelStatus.CHANNEL_AVAILABLE);
            jSdk.onChannelStatusChanged(
                    RaceHandle.NULL_RACE_HANDLE,
                    channelGid,
                    ChannelStatus.CHANNEL_AVAILABLE,
                    Channels.getDefaultChannelPropertiesForChannel(jSdk, channelGid),
                    jSdk.getBlockingTimeout());
             jSdk.displayInfoToUser(channelGid + " is available", UserDisplayType.UD_TOAST);
        } else if (channelGid.compareTo(Channels.directChannelGid) == 0) {
            SdkResponse response =
                    jSdk.requestPluginUserInput(
                            "startPort", "What is the first available port?", true);
            if (response == null || response.getStatus() != SdkResponse.SdkStatus.SDK_OK) {
                RaceLog.logWarning(logLabel, "Failed to request start port from user", "");
            } else {
                this.requestStartPortHandle = response.getHandle();
                userInputRequests.add(response.getHandle());
            }

            response = jSdk.requestCommonUserInput("hostname");
            if (response == null || response.getStatus() != SdkResponse.SdkStatus.SDK_OK) {
                RaceLog.logWarning(logLabel, "Failed to request hostname from user", "");
                this.channelStatuses.put(channelGid, ChannelStatus.CHANNEL_FAILED);
                jSdk.onChannelStatusChanged(
                        RaceHandle.NULL_RACE_HANDLE,
                        channelGid,
                        ChannelStatus.CHANNEL_FAILED,
                        Channels.getDefaultChannelPropertiesForChannel(jSdk, channelGid),
                        jSdk.getBlockingTimeout());
                return PluginResponse.PLUGIN_OK;
            } else {
                this.requestHostnameHandle = response.getHandle();
                userInputRequests.add(response.getHandle());
            }

        } else {
            RaceLog.logWarning(logLabel, "activateChannel: unrecognized channel " + channelGid, "");
        }
        return PluginResponse.PLUGIN_OK;
    }

    /**
     * Shutdown the plugin. Close open connections, remove state, etc.
     *
     * @return PluginResponse the status of the Plugin in response to this call
     */
    @Override
    public PluginResponse shutdown() {
        RaceLog.logDebug(logLabel, "shutdown: called", "");

        ArrayList<String> connectionIds = new ArrayList<>(connections.keySet());
        for (String connectionId : connectionIds) {
            closeConnection(RaceHandle.NULL_RACE_HANDLE, connectionId);
        }
        System.gc();
        return PluginResponse.PLUGIN_OK;
    }

    /**
     * Open a connection with a given type on the specified link. Additional configuration info can
     * be provided via the config param.
     *
     * @param handle The RaceHandle to use for onConnectionStatusChanged calls
     * @param linkType The type of link to open: send, receive, or bi-directional.
     * @param linkId The ID of the link that the connection should be opened
     * @param linkHints Additional optional configuration information provided by network manager as a
     *     stringified JSON Object.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    @Override
    public PluginResponse openConnection(
            RaceHandle handle,
            LinkType linkType,
            String linkId,
            String linkHints,
            int sendTimeout) {
        synchronized (this) {
            RaceLog.logDebug(logLabel, "openConnection called", "");

            RaceLog.logDebug(logLabel, "    type:         " + linkType.name(), "");
            RaceLog.logDebug(logLabel, "    ID:           " + linkId, "");

            String newConnectionId = jSdk.generateConnectionId(linkId);

            RaceLog.logDebug(logLabel, "    Assigned connectionID: " + newConnectionId, "");
            RaceLog.logDebug(
                    logLabel, "    Current number of connectionLinkIds: " + connections.size(), "");

            Link link = links.get(linkId);
            if (link == null) {
                RaceLog.logError(logLabel, "Unable to open connection for link: " + linkId + ", link does not exist", "");
                return PluginResponse.PLUGIN_ERROR;
            }

            Connection connection = link.openConnection(linkType, newConnectionId, linkHints);
            connections.put(connection.connectionId, connection);

            JLinkProperties linkProps = new JLinkProperties();
            if (links.containsKey(linkId)) {
                linkProps = links.get(linkId).getLinkProperties();
            }

            jSdk.onConnectionStatusChanged(
                    handle,
                    newConnectionId,
                    ConnectionStatus.CONNECTION_OPEN,
                    linkProps,
                    JRaceSdkComms.getBlockingTimeout());
            return PluginResponse.PLUGIN_OK;
        }
    }

    /**
     * Close a connection with a given ID.
     *
     * @param handle The RaceHandle to use for onConnectionStatusChanged calls
     * @param connectionId The ID of the connection to close.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    @Override
    public PluginResponse closeConnection(RaceHandle handle, String connectionId) {
        synchronized (this) {
            RaceLog.logDebug(logLabel, "closeConnection called", "");
            RaceLog.logDebug(logLabel, "    ID: " + connectionId, "");

            Link link = connections.get(connectionId).getLink();
            connections.remove(connectionId);
            link.closeConnection(connectionId);
            jSdk.onConnectionStatusChanged(
                    handle,
                    connectionId,
                    ConnectionStatus.CONNECTION_CLOSED,
                    link.getLinkProperties(),
                    JRaceSdkComms.getBlockingTimeout());
            return PluginResponse.PLUGIN_OK;
        }
    }

    @Override
    public PluginResponse destroyLink(RaceHandle handle, String linkId) {
        String logPrefix =
                String.format("destroyLink: (handle: %s link ID: %s):", handle.toString(), linkId);
        RaceLog.logDebug(logLabel, String.format("%s called", logPrefix), "");

        Link link = this.links.get(linkId);
        if (link == null) {
            RaceLog.logError(
                    logLabel, String.format("%s link with ID does not exist", logPrefix), "");
            return PluginResponse.PLUGIN_ERROR;
        }

        jSdk.onLinkStatusChanged(
                handle,
                linkId,
                LinkStatus.LINK_DESTROYED,
                links.get(linkId).getLinkProperties(),
                jSdk.getBlockingTimeout());

        Vector<Connection> connectionsInLink = link.getLinkConnections();
        for (Connection connection : connectionsInLink) {
            // Makes SDK API call to onConnectionStatusChanged.
            this.closeConnection(handle, connection.connectionId);
        }

        links.remove(linkId);

        RaceLog.logDebug(logLabel, String.format("%s returned", logPrefix), "");
        return PluginResponse.PLUGIN_OK;
    }

    @Override
    public PluginResponse createLink(RaceHandle handle, String channelGid) {
        String logPrefix =
                String.format(
                        "createLink: (handle: %s channel GID: %s):", handle.toString(), channelGid);
        RaceLog.logDebug(logLabel, String.format("%s called", logPrefix), "");

        if (this.channelStatuses.get(channelGid) != ChannelStatus.CHANNEL_AVAILABLE) {
            RaceLog.logError(logLabel, String.format("%s channel not available.", logPrefix), "");
            jSdk.onLinkStatusChanged(
                    handle,
                    "",
                    LinkStatus.LINK_DESTROYED,
                    Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid),
                    jSdk.getBlockingTimeout());
            return PluginResponse.PLUGIN_ERROR;
        }

        JLinkProperties linkProps = Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid);

        if (channelGid.equals(Channels.directChannelGid)) {
            RaceLog.logDebug(
                    logLabel, String.format("%s Creating TwoSix Direct Link", logPrefix), "");

            String linkAddress =
                    String.format(
                            "{\"hostname\":\"%s\",\"port\":%d}",
                            this.hostname, this.nextAvailablePort++);

            DirectLinkProfileParser parser = new DirectLinkProfileParser(linkAddress);
            linkProps.linkType = LinkType.LT_RECV;
            linkProps.channelGid = channelGid;

            JLinkConfig linkConfig = new JLinkConfig();
            linkConfig.linkProfile = linkAddress;
            linkConfig.linkProps = linkProps;
            Link link = parser.createLink(this, linkConfig, channelGid);
            if (link == null) {
                RaceLog.logError(
                        logLabel, String.format("%s failed to create direct link.", logPrefix), "");
                jSdk.onLinkStatusChanged(
                        handle,
                        "",
                        LinkStatus.LINK_DESTROYED,
                        Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid),
                        jSdk.getBlockingTimeout());
                return PluginResponse.PLUGIN_ERROR;
            }
            linkProps.linkAddress = linkAddress;

            links.put(link.linkId, link);
            RaceLog.logDebug(
                    logLabel,
                    String.format(
                            "%s calling onLinkStatusChanged with channel GID: %s",
                            logPrefix, linkProps.channelGid),
                    "");
            jSdk.onLinkStatusChanged(
                    handle,
                    link.linkId,
                    LinkStatus.LINK_CREATED,
                    linkProps,
                    jSdk.getBlockingTimeout());
            jSdk.updateLinkProperties(link.linkId, linkProps, jSdk.getBlockingTimeout());

            RaceLog.logDebug(
                    logLabel,
                    String.format(
                            "%s Created TwoSix Direct Link with link ID: %s and link address: %s",
                            logPrefix, link.linkId, linkAddress),
                    "");
        } else if (channelGid.equals(Channels.indirectChannelGid)) {
            RaceLog.logDebug(
                    logLabel, String.format("%s Creating TwoSix Indirect Link", logPrefix), "");

            String linkAddress =
                    String.format(
                            "{\"hostname\":\"%s\",\"port\":%d,\"hashtag\":\"java_%s_%d\",\"checkFrequency\":1000,\"timestamp\":%s}",
                            this.whiteboardHostname,
                            this.whiteboardPort,
                            this.racePersona,
                            this.nextAvailableHashtag++,
                            Time.getTimestampString(Time.getTimestampSeconds()));

            TwosixWhiteboardLinkProfileParser parser =
                    new TwosixWhiteboardLinkProfileParser(linkAddress);
            linkProps.linkType = LinkType.LT_BIDI;

            JLinkConfig linkConfig = new JLinkConfig();
            linkConfig.linkProfile = linkAddress;
            linkConfig.linkProps = linkProps;
            Link link = parser.createLink(this, linkConfig, channelGid);
            if (link == null) {
                RaceLog.logError(
                        logLabel,
                        String.format("%s failed to create indirect link.", logPrefix),
                        "");
                jSdk.onLinkStatusChanged(
                        handle,
                        "",
                        LinkStatus.LINK_DESTROYED,
                        Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid),
                        jSdk.getBlockingTimeout());
                return PluginResponse.PLUGIN_ERROR;
            }
            linkProps.linkAddress = linkAddress;

            links.put(link.linkId, link);
            RaceLog.logDebug(
                    logLabel,
                    String.format(
                            "%s calling onLinkStatusChanged with channel GID: %s",
                            logPrefix, linkProps.channelGid),
                    "");
            jSdk.onLinkStatusChanged(
                    handle,
                    link.linkId,
                    LinkStatus.LINK_CREATED,
                    linkProps,
                    jSdk.getBlockingTimeout());
            jSdk.updateLinkProperties(link.linkId, linkProps, jSdk.getBlockingTimeout());

            RaceLog.logDebug(
                    logLabel,
                    String.format(
                            "%s Created TwoSix Indirect Link with link ID: %s and link address: %s",
                            logPrefix, link.linkId, linkAddress),
                    "");
        } else {
            RaceLog.logError(logLabel, String.format("%s invalid channel GID.", logPrefix), "");
            jSdk.onLinkStatusChanged(
                    handle,
                    "",
                    LinkStatus.LINK_DESTROYED,
                    Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid),
                    jSdk.getBlockingTimeout());
            return PluginResponse.PLUGIN_ERROR;
        }

        RaceLog.logDebug(logLabel, String.format("%s returned", logPrefix), "");
        return PluginResponse.PLUGIN_OK;
    }

    @Override
    public PluginResponse loadLinkAddress(
            RaceHandle handle, String channelGid, String linkAddress) {
        String logPrefix =
                String.format(
                        "loadLinkAddress: (handle: %s channel GID: %s):",
                        handle.toString(), channelGid);
        RaceLog.logDebug(
                logLabel,
                String.format("%s called with link address: %s", logPrefix, linkAddress),
                "");

        if (this.channelStatuses.get(channelGid) != ChannelStatus.CHANNEL_AVAILABLE) {
            RaceLog.logError(logLabel, String.format("%s channel not available.", logPrefix), "");
            jSdk.onLinkStatusChanged(
                    handle,
                    "",
                    LinkStatus.LINK_DESTROYED,
                    Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid),
                    jSdk.getBlockingTimeout());
            return PluginResponse.PLUGIN_ERROR;
        }

        JLinkProperties linkProps = Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid);

        if (channelGid.equals(Channels.directChannelGid)) {
            RaceLog.logDebug(
                    logLabel, String.format("%s Loading TwoSix Direct Link", logPrefix), "");

            DirectLinkProfileParser parser = new DirectLinkProfileParser(linkAddress);
            linkProps.linkType = LinkType.LT_SEND;

            JLinkConfig linkConfig = new JLinkConfig();
            linkConfig.linkProfile = linkAddress;
            linkConfig.linkProps = linkProps;
            Link link = parser.createLink(this, linkConfig, channelGid);
            if (link == null) {
                RaceLog.logError(
                        logLabel, String.format("%s failed to load direct link.", logPrefix), "");
                jSdk.onLinkStatusChanged(
                        handle,
                        "",
                        LinkStatus.LINK_DESTROYED,
                        Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid),
                        jSdk.getBlockingTimeout());
                return PluginResponse.PLUGIN_ERROR;
            }

            links.put(link.linkId, link);
            RaceLog.logDebug(
                    logLabel,
                    String.format(
                            "%s calling onLinkStatusChanged with channel GID: %s",
                            logPrefix, linkProps.channelGid),
                    "");
            jSdk.onLinkStatusChanged(
                    handle,
                    link.linkId,
                    LinkStatus.LINK_LOADED,
                    linkProps,
                    jSdk.getBlockingTimeout());
            jSdk.updateLinkProperties(link.linkId, linkProps, jSdk.getBlockingTimeout());

            RaceLog.logDebug(
                    logLabel,
                    String.format(
                            "%s Loaded TwoSix Direct Link with link ID: %s and link address: %s",
                            logPrefix, link.linkId, linkAddress),
                    "");
        } else if (channelGid.equals(Channels.indirectChannelGid)) {
            RaceLog.logDebug(
                    logLabel, String.format("%s Loading TwoSix Indirect Link", logPrefix), "");

            TwosixWhiteboardLinkProfileParser parser =
                    new TwosixWhiteboardLinkProfileParser(linkAddress);
            linkProps.linkType = LinkType.LT_BIDI;

            JLinkConfig linkConfig = new JLinkConfig();
            linkConfig.linkProfile = linkAddress;
            linkConfig.linkProps = linkProps;
            Link link = parser.createLink(this, linkConfig, channelGid);
            if (link == null) {
                RaceLog.logError(
                        logLabel, String.format("%s failed to load indirect link.", logPrefix), "");
                jSdk.onLinkStatusChanged(
                        handle,
                        "",
                        LinkStatus.LINK_DESTROYED,
                        Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid),
                        jSdk.getBlockingTimeout());
                return PluginResponse.PLUGIN_ERROR;
            }

            links.put(link.linkId, link);
            RaceLog.logDebug(
                    logLabel,
                    String.format(
                            "%s calling onLinkStatusChanged with channel GID: %s",
                            logPrefix, linkProps.channelGid),
                    "");
            jSdk.onLinkStatusChanged(
                    handle,
                    link.linkId,
                    LinkStatus.LINK_LOADED,
                    linkProps,
                    jSdk.getBlockingTimeout());
            jSdk.updateLinkProperties(link.linkId, linkProps, jSdk.getBlockingTimeout());

            RaceLog.logDebug(
                    logLabel,
                    String.format(
                            "%s Loaded TwoSix Indirect Link with link ID: %s and link address: %s",
                            logPrefix, link.linkId, linkAddress),
                    "");
        } else {
            RaceLog.logError(logLabel, String.format("%s invalid channel GID.", logPrefix), "");
            jSdk.onLinkStatusChanged(
                    handle,
                    "",
                    LinkStatus.LINK_DESTROYED,
                    Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid),
                    jSdk.getBlockingTimeout());
            return PluginResponse.PLUGIN_ERROR;
        }

        RaceLog.logDebug(logLabel, String.format("%s returned", logPrefix), "");
        return PluginResponse.PLUGIN_OK;
    }

    @Override
    public PluginResponse loadLinkAddresses(
            RaceHandle handle, String channelGid, String[] linkAddresses) {
        String logPrefix =
                String.format(
                        "loadLinkAddresses: (handle: %s channel GID: %s):",
                        handle.toString(), channelGid);
        RaceLog.logDebug(
                logLabel,
                String.format(
                        "%s called with link addresses: %s",
                        logPrefix, Arrays.toString(linkAddresses)),
                "");
        RaceLog.logError(
                logLabel,
                String.format("%s API not supported for any TwoSix channels", logPrefix),
                "");
        jSdk.onLinkStatusChanged(
                handle,
                "",
                LinkStatus.LINK_DESTROYED,
                Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid),
                jSdk.getBlockingTimeout());
        RaceLog.logDebug(logLabel, String.format("%s returned", logPrefix), "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse createLinkFromAddress(
            RaceHandle handle, String channelGid, String linkAddress) {
        String logPrefix =
                String.format(
                        "createLinkFromAddress: (handle: %s channel GID: %s):",
                        handle.toString(), channelGid);
        RaceLog.logDebug(
                logLabel,
                String.format("%s called with link address: %s", logPrefix, linkAddress),
                "");

        if (this.channelStatuses.get(channelGid) != ChannelStatus.CHANNEL_AVAILABLE) {
            RaceLog.logError(logLabel, String.format("%s channel not available.", logPrefix), "");
            jSdk.onLinkStatusChanged(
                    handle,
                    "",
                    LinkStatus.LINK_DESTROYED,
                    Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid),
                    jSdk.getBlockingTimeout());
            return PluginResponse.PLUGIN_ERROR;
        }

        JLinkProperties linkProps = Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid);
        linkProps.linkAddress = linkAddress;

        if (channelGid.equals(Channels.directChannelGid)) {
            RaceLog.logDebug(
                    logLabel, String.format("%s Creating TwoSix Direct Link", logPrefix), "");

            DirectLinkProfileParser parser = new DirectLinkProfileParser(linkAddress);
            linkProps.linkType = LinkType.LT_RECV;

            JLinkConfig linkConfig = new JLinkConfig();
            linkConfig.linkProfile = linkAddress;
            linkConfig.linkProps = linkProps;
            Link link = parser.createLink(this, linkConfig, channelGid);
            if (link == null) {
                RaceLog.logError(
                        logLabel, String.format("%s failed to create direct link.", logPrefix), "");
                jSdk.onLinkStatusChanged(
                        handle,
                        "",
                        LinkStatus.LINK_DESTROYED,
                        Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid),
                        jSdk.getBlockingTimeout());
                return PluginResponse.PLUGIN_ERROR;
            }

            links.put(link.linkId, link);
            RaceLog.logDebug(
                    logLabel,
                    String.format(
                            "%s calling onLinkStatusChanged with channel GID: %s",
                            logPrefix, linkProps.channelGid),
                    "");
            jSdk.onLinkStatusChanged(
                    handle,
                    link.linkId,
                    LinkStatus.LINK_CREATED,
                    linkProps,
                    jSdk.getBlockingTimeout());
            jSdk.updateLinkProperties(link.linkId, linkProps, jSdk.getBlockingTimeout());

            RaceLog.logDebug(
                    logLabel,
                    String.format(
                            "%s Created TwoSix Direct Link with link ID: %s and link address: %s",
                            logPrefix, link.linkId, linkAddress),
                    "");
        } else if (channelGid.equals(Channels.indirectChannelGid)) {
            RaceLog.logDebug(
                    logLabel, String.format("%s Creating TwoSix Indirect Link", logPrefix), "");

            TwosixWhiteboardLinkProfileParser parser =
                    new TwosixWhiteboardLinkProfileParser(linkAddress);
            linkProps.linkType = LinkType.LT_BIDI;

            JLinkConfig linkConfig = new JLinkConfig();
            linkConfig.linkProfile = linkAddress;
            linkConfig.linkProps = linkProps;
            Link link = parser.createLink(this, linkConfig, channelGid);
            if (link == null) {
                RaceLog.logError(
                        logLabel,
                        String.format("%s failed to create indirect link.", logPrefix),
                        "");
                jSdk.onLinkStatusChanged(
                        handle,
                        "",
                        LinkStatus.LINK_DESTROYED,
                        Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid),
                        jSdk.getBlockingTimeout());
                return PluginResponse.PLUGIN_ERROR;
            }

            links.put(link.linkId, link);
            RaceLog.logDebug(
                    logLabel,
                    String.format(
                            "%s calling onLinkStatusChanged with channel GID: %s",
                            logPrefix, linkProps.channelGid),
                    "");
            jSdk.onLinkStatusChanged(
                    handle,
                    link.linkId,
                    LinkStatus.LINK_CREATED,
                    linkProps,
                    jSdk.getBlockingTimeout());
            jSdk.updateLinkProperties(link.linkId, linkProps, jSdk.getBlockingTimeout());

            RaceLog.logDebug(
                    logLabel,
                    String.format(
                            "%s Created TwoSix Indirect Link with link ID: %s and link address: %s",
                            logPrefix, link.linkId, linkAddress),
                    "");
        } else {
            RaceLog.logError(logLabel, String.format("%s invalid channel GID.", logPrefix), "");
            jSdk.onLinkStatusChanged(
                    handle,
                    "",
                    LinkStatus.LINK_DESTROYED,
                    Channels.getDefaultLinkPropertiesForChannel(jSdk, channelGid),
                    jSdk.getBlockingTimeout());
            return PluginResponse.PLUGIN_ERROR;
        }

        RaceLog.logDebug(logLabel, String.format("%s returned", logPrefix), "");
        return PluginResponse.PLUGIN_OK;
    }

    @Override
    public PluginResponse deactivateChannel(RaceHandle handle, String channelGid) {
        String logPrefix =
                String.format(
                        "deactivateChannel: (handle: %s channel GID: %s):",
                        handle.toString(), channelGid);
        RaceLog.logDebug(logLabel, String.format("%s called", logPrefix), "");

        if (this.channelStatuses.get(channelGid) == null) {
            RaceLog.logError(logLabel, String.format("%s channel does not exist.", logPrefix), "");
            return PluginResponse.PLUGIN_ERROR;
        }

        this.channelStatuses.put(channelGid, ChannelStatus.CHANNEL_UNAVAILABLE);
        jSdk.onChannelStatusChanged(
                handle,
                channelGid,
                ChannelStatus.CHANNEL_UNAVAILABLE,
                Channels.getDefaultChannelPropertiesForChannel(jSdk, channelGid),
                jSdk.getBlockingTimeout());

        List<String> linkIdsOfLinksToDestroy = new ArrayList<String>();

        for (String linkId : links.keySet()) {
            if (links.get(linkId).getLinkProperties().channelGid.equals(channelGid)) {
                linkIdsOfLinksToDestroy.add(linkId);
            }
        }

        for (String linkId : linkIdsOfLinksToDestroy) {
            destroyLink(handle, linkId);
        }

        RaceLog.logDebug(logLabel, String.format("%s returned", logPrefix), "");
        return PluginResponse.PLUGIN_OK;
    }

    @Override
    public PluginResponse onUserInputReceived(
            RaceHandle handle, boolean answered, String response) {
        String logPrefix = String.format("onUserInputReceived (handle: %s):", handle.toString());
        RaceLog.logDebug(logLabel, String.format("%s called", logPrefix), "");

        if (handle.equals(this.requestHostnameHandle)) {
            if (answered) {
                this.hostname = response;
                RaceLog.logInfo(
                        logLabel,
                        String.format("%s using hostname %s", logPrefix, this.hostname),
                        "");
            } else {
                RaceLog.logError(
                        logLabel,
                        String.format(
                                "%s direct channel not available without the hostname", logPrefix),
                        "");
                jSdk.onChannelStatusChanged(
                        RaceHandle.NULL_RACE_HANDLE,
                        Channels.directChannelGid,
                        ChannelStatus.CHANNEL_UNAVAILABLE,
                        Channels.getDefaultChannelPropertiesForChannel(
                                jSdk, Channels.directChannelGid),
                        jSdk.getBlockingTimeout());
            }
        } else if (handle.equals(this.requestStartPortHandle)) {
            if (answered) {
                try {
                    int port = Integer.parseInt(response);
                    RaceLog.logInfo(
                            logLabel, String.format("%s using start port %d", logPrefix, port), "");
                    this.nextAvailablePort = port;
                } catch (NumberFormatException nfe) {
                    RaceLog.logWarning(
                            logLabel,
                            String.format("%s error parsing start port, %s", logPrefix, response),
                            "");
                }
            } else {
                RaceLog.logWarning(
                        logLabel,
                        String.format("%s no answer, using default start port", logPrefix),
                        "");
            }
        } else {
            RaceLog.logWarning(
                    logLabel, String.format("%s handle is not recognized", logPrefix), "");
            return PluginResponse.PLUGIN_ERROR;
        }
        userInputRequests.remove(handle);
        if (userInputRequests.size() == 0) {
            this.channelStatuses.put(Channels.directChannelGid, ChannelStatus.CHANNEL_AVAILABLE);
            jSdk.onChannelStatusChanged(
                    RaceHandle.NULL_RACE_HANDLE,
                    Channels.directChannelGid,
                    ChannelStatus.CHANNEL_AVAILABLE,
                    Channels.getDefaultChannelPropertiesForChannel(jSdk, Channels.directChannelGid),
                    jSdk.getBlockingTimeout());
                jSdk.displayInfoToUser(Channels.directChannelGid + " is available", UserDisplayType.UD_TOAST);
        }

        RaceLog.logDebug(logLabel, String.format("%s returned", logPrefix), "");
        return PluginResponse.PLUGIN_OK;
    }

    /**
     * Open a connection with a given type on the specified link. Additional configuration info can
     * be provided via the config param.
     *
     * @param handle The RaceHandle to use for updating package status in onPackageStatusChanged
     *     calls
     * @param connectionId The ID of the connection to use to send the package.
     * @param pkg The encrypted package to send.
     * @param timeoutTimestamp The time the package must be sent by. Measured in seconds since epoch
     * @param batchId The batch ID used to group encrypted packages so that they can be flushed at
     *     the same time when flushChannel is called. If this value is zero then it can safely be
     *     ignored.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    @Override
    public PluginResponse sendPackage(
            RaceHandle handle,
            String connectionId,
            JEncPkg pkg,
            double timeoutTimestamp,
            long batchId) {
        synchronized (this) {
            String loggingPrefix = logLabel + ": sendPackage (" + connectionId + "): ";
            RaceLog.logDebug(loggingPrefix, "sendPackage called", "");

            RaceLog.logDebug(loggingPrefix, "connectionId: " + connectionId, "");
            Connection conn = connections.get(connectionId);
            if (conn == null) {
                RaceLog.logWarning(loggingPrefix, "connectionId: " + connectionId + " not found", "");
                return PluginResponse.PLUGIN_ERROR;
            }

            RaceLog.logDebug(loggingPrefix, "link: " + conn.getLink(), "");
            final String linkProfile = conn.getLink().getProfile();
            RaceLog.logDebug(loggingPrefix, "profile: " + linkProfile, "");

            connections.get(connectionId).getLink().sendPackage(pkg);
            jSdk.onPackageStatusChanged(
                    handle, PackageStatus.PACKAGE_SENT, JRaceSdkComms.getBlockingTimeout());
            return PluginResponse.PLUGIN_OK;
        }
    }

    @Override
    public PluginResponse flushChannel(RaceHandle handle, String channelGid, long batchId) {
        RaceLog.logError("StubCommsPlugin", "API not supported by exemplar plugin", "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse serveFiles(String linkId, String path) {
        RaceLog.logError("StubCommsPlugin", "API not supported by exemplar plugin", "");
        return PluginResponse.PLUGIN_ERROR;
    }

    @Override
    public PluginResponse createBootstrapLink(
            RaceHandle handle, String channelGid, String passphrase) {
        RaceLog.logError("StubCommsPlugin", "API not supported by exemplar plugin", "");
        return PluginResponse.PLUGIN_ERROR;
    }

    /**
     * @param key
     * @param value
     * @return boolean
     */
    boolean saveValue(String key, int value) {
        String valueString = "" + value;
        SdkResponse response = this.jSdk.writeFile(key, valueString.getBytes());
        return response.getStatus() == SdkResponse.SdkStatus.SDK_OK;
    }

    /**
     * @param key
     * @param defaultValue
     * @return int
     */
    int readValue(String key, int defaultValue) {

        byte[] valueData = this.jSdk.readFile(key);
        if (valueData.length == 0) {
            return defaultValue;
        }
        ByteBuffer valueBuffer = ByteBuffer.wrap(valueData);
        return valueBuffer.getInt();
    }

    /**
     * @brief Notify the plugin that the user acknowledged the displayed information
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    @Override
    public PluginResponse onUserAcknowledgementReceived(RaceHandle handle) {
        return PluginResponse.PLUGIN_OK;
    }
}
