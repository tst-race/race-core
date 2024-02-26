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

package ShimsJava;

public interface IRacePluginComms {

    /**
     * Initialize the plugin. Set the RaceSdk object and other prep work to begin allowing calls
     * from core and other plugins.
     *
     * @param pluginConfig Config object containing dynamic config variables (e.g. paths)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse init(PluginConfig pluginConfig);

    /**
     * Shutdown the plugin. Close open connections, remove state, etc.
     *
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse shutdown();

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
    public PluginResponse sendPackage(
            RaceHandle handle,
            String connectionId,
            JEncPkg pkg,
            double timeoutTimestamp,
            long batchId);

    /**
     * Open a connection with a given type on the specified link. Additional configuration info can
     * be provided via the config param.
     *
     * @param handle The RaceHandle to use for onConnectionStatusChanged calls
     * @param linkType The type of link to open: send, receive, or bi-directional.
     * @param linkId The ID of the link that the connection should be opened
     * @param linkHints Additional optional configuration information provided by network manager as
     *     a stringified JSON Object.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse openConnection(
            RaceHandle handle, LinkType linkType, String linkId, String linkHints, int sendTimeout);

    /**
     * Close a connection with a given ID.
     *
     * @param handle The RaceHandle to use for onConnectionStatusChanged calls
     * @param connectionId The ID of the connection to close.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse closeConnection(RaceHandle handle, String connectionId);

    /**
     * Destroy a link
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param linkId The ID of the link to destroy
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse destroyLink(RaceHandle handle, String linkId);

    /**
     * Deactivate a channel
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to deactivate
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse deactivateChannel(RaceHandle handle, String channelGid);

    /**
     * ctivate a channel
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to activate
     * @param roleName The name of the role being activated
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse activateChannel(RaceHandle handle, String channelGid, String roleName);

    /**
     * Create a new link on the given channel
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of channel to create a link from
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse createLink(RaceHandle handle, String channelGid);

    /**
     * @brief create link from address specified by genensis configs
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to create a new link for
     * @param linkAddress The LinkAddress used to create this link
     * @return PluginResponse the status of the SDK in response to this call
     */
    public PluginResponse createLinkFromAddress(
            RaceHandle handle, String channelGid, String linkAddress);

    /**
     * Create a new link on the given channel
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of channel to create a link from
     * @param linkAddress The address to load
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse loadLinkAddress(RaceHandle handle, String channelGid, String linkAddress);

    /**
     * Create a new link on the given channel
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of channel to create a link from
     * @param linkAddresses The addresses to load
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse loadLinkAddresses(
            RaceHandle handle, String channelGid, String[] linkAddresses);

    /**
     * Notify comms about received user input response
     *
     * @param handle The handle for this callback
     * @param answered True if the response contains an actual answer to the input prompt, otherwise
     *     the response is an empty string and not valid
     * @param response The user response answer to the input prompt
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse onUserInputReceived(RaceHandle handle, boolean answered, String response);

    /**
     * Flush any pending encrypted packages queued to be sent out over the given connection.
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @param connId The ID of the connection to flush.
     * @param batchId The batch ID of the encrypted packages to be flushed. If batch ID is set to
     *     zero (the null value) this call will return an error. A valid batch ID must be provided.
     * @return PluginResponse Status of plugin indicating success or failure of the call.
     */
    public PluginResponse flushChannel(RaceHandle handle, String connId, long batchId);

    /**
     * @brief Serve files located in the specified directory. The files served are associated with
     *     the specified link and may stop being served when the link is closed.
     * @param linkId The link to serve the files on
     * @param path A path to the directory containing the files to serve
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse serveFiles(String linkId, String path);

    /**
     * @brief Create a bootstrap link of the channel specified with the specified passphrase
     * @param handle The handle for asynchronously returning the status of this call.
     * @param channelGid The name of the channel to create a link for
     * @param passphrase The passphrase to use with the link
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse createBootstrapLink(
            RaceHandle handle, String channelGid, String passphrase);

    /**
     * @brief Notify the plugin that the user acknowledged the displayed information
     * @param handle The handle for asynchronously returning the status of this call.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse onUserAcknowledgementReceived(RaceHandle handle);
}
