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

// For API documentation please see the equivalent C++ header:
// racesdk/common/include/IRacePluginNM.h

package ShimsJava;

public interface IRacePluginNM {

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
     * Given a cleartext message, do everything necessary to encrypt and send the encrypted package
     * out on the correct Transport, etc.
     *
     * @param handle The RaceHandle for this call
     * @param msg The clear text message to process.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse processClrMsg(RaceHandle handle, JClrMsg msg);

    /**
     * Given an encrypted package, do everything necessary to either display it to the user, forward
     * it (if this is a server), or just read it (if this message was intended for the network
     * manager module).
     *
     * @param handle The RaceHandle for this call
     * @param ePkg The encrypted package to process.
     * @param connIDs List of connection IDs that the package may have come in on. Since multiple
     *     logical connection IDs can be used for the same physical connection, all logical
     *     connection IDs are provided. Note that only one of the connection IDs will actually be
     *     associated with this package.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse processEncPkg(RaceHandle handle, JEncPkg ePkg, String[] connIDs);

    /**
     * Notify network manager that a device needs to be bootstrapped. network manager should
     * generate the necessary configs and determine what plugins to use. Once everything necessary
     * has been prepared, the networkManager should call sdk::bootstrapDevice.
     *
     * @param handle The RaceHandle that should be handed to the bootstrapNode call later
     * @param linkId the link id of the bootstrap link
     * @param configPath A path to the directory to store configs in
     * @param deviceInfo Information about the device being bootstrapped
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse prepareToBootstrap(
            RaceHandle handle, String linkId, String configPath, DeviceInfo deviceInfo);

    /**
     * Inform the network manager when the key from a bootstrapped node is receive. The network
     * manager should should preform necessary steps to introduce the node the network.
     *
     * @param persona The persona of the new bootstrapped node
     * @param key The key associated with the bootstrapped node
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse onBootstrapKeyReceived(String persona, byte[] key);

    /**
     * Notify network manager about a change in package status. The handle will correspond to a
     * handle returned inside an SdkResponse to a previous sendEncryptedPackage call.
     *
     * @param handle The RaceHandle of the previous sendEncryptedPackage call
     * @param status The PackageStatus of the package updated
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse onPackageStatusChanged(RaceHandle handle, PackageStatus status);

    /**
     * Notify network manager about a change in the status of a connection. The handle will
     * correspond to a handle returned inside an SdkResponse to a previous openConnection or
     * closeConnection call.
     *
     * @param handle The RaceHandle of the original openConnection or closeConnection call, if that
     *     is what caused the change. Otherwise, 0
     * @param connId The ConnectionID of the connection
     * @param status The ConnectionStatus of the connection updated
     * @param linkId The LinkID of the link this connection is on
     * @param properties The LinkPropertes of the connection updated
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse onConnectionStatusChanged(
            RaceHandle handle,
            String connId,
            ConnectionStatus status,
            String linkId,
            JLinkProperties properties);

    /**
     * Notify network manager about a change in the status of a link. The handle will correspond to
     * a handle returned inside an SdkResponse to a previous createLink, loadLinkAddress,
     * loadLinkAddresses, or destroyLink call.
     *
     * @param handle The RaceHandle of the original openLink or closeLink call, if that is what
     *     caused the change. Otherwise, 0
     * @param linkId The LinkID of the link this link is on
     * @param status The LinkStatus of the link updated
     * @param properties The LinkPropertes of the link updated
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse onLinkStatusChanged(
            RaceHandle handle, String linkId, LinkStatus status, JLinkProperties properties);

    /**
     * Notify network manager about a change in the status of a channel.
     *
     * @param handle The RaceHandle of the original deactivateChannel call, if that is what caused
     *     the change. Otherwise, 0
     * @param channelGid The name of the channel this channel is on
     * @param status The ChannelStatus of the channel updated
     * @param properties The ChannelPropertes of the channel updated
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse onChannelStatusChanged(
            RaceHandle handle,
            String channelGid,
            ChannelStatus status,
            JChannelProperties properties);

    /**
     * Notify network manager about a change to the LinkProperties of a Link
     *
     * @param linkId The LinkdID of the link that has been updated
     * @param linkProperties The LinkProperties that were updated
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse onLinkPropertiesChanged(String linkId, JLinkProperties linkProperties);

    /**
     * Notify network manager about a change to the Links associated with a Persona
     *
     * @param recipientPersona The Persona that has changed link associations
     * @param linkType The LinkType of the links (send, recv, bidi)
     * @param links The list of links that are now associated with this persona
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse onPersonaLinksChanged(
            String recipientPersona, LinkType linkType, String[] links);

    /**
     * Notify network manager about received user input response
     *
     * @param handle The handle for this callback
     * @param answered True if the response contains an actual answer to the input prompt, otherwise
     *     the response is an empty string and not valid
     * @param response The user response answer to the input prompt
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse onUserInputReceived(RaceHandle handle, boolean answered, String response);

    /**
     * Notify network manager to perform epoch changeover processing
     *
     * @param data Data associated with epoch (network manager implementation specific)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse notifyEpoch(String data);

    /**
     * @brief Notify the plugin that the user acknowledged the displayed information
     * @param handle The handle for asynchronously returning the status of this call.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    public PluginResponse onUserAcknowledgementReceived(RaceHandle handle);
}
