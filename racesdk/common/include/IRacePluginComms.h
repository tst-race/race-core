
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

#ifndef __I_RACE_PLUGIN_COMMS_H_
#define __I_RACE_PLUGIN_COMMS_H_

#include <string>

#include "ChannelProperties.h"
#include "EncPkg.h"
#include "IRaceSdkComms.h"
#include "LinkProperties.h"
#include "PluginConfig.h"
#include "PluginResponse.h"
#include "RacePluginExports.h"

class IRacePluginComms {
public:
    /**
     * @brief Destroy the IRacePluginComms object
     *
     */
    virtual ~IRacePluginComms() {}

    /**
     * @brief Set the Sdk object and perform minimum work to
     * be abe to respond to incoming calls. Do not use any calls
     * to raceSdk that require the network manager. Use minimal calls to raceSdk.
     *
     * @param sdk Pointer to the SDK instance.
     * @param pluginConfig Config object containing dynamic config variables (e.g. paths)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse init(const PluginConfig &pluginConfig) = 0;

    /**
     * @brief Shutdown the plugin. Close open connections, remove state, etc.
     *
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse shutdown() = 0;

    /**
     * @brief Send an encrypted package.
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @param connectionId The ID of the connection to use to send the package.
     * @param pkg The encrypted package to send.
     * @param timeoutTimestamp The time the package must be sent by. Measured in seconds since epoch
     * @param batchId The batch ID used to group encrypted packages so that they can be flushed at
     * the same time when flushChannel is called. If this value is zero then it can safely be
     * ignored.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse sendPackage(RaceHandle handle, ConnectionID connectionId, EncPkg pkg,
                                       double timeoutTimestamp, uint64_t batchId) = 0;

    /**
     * @brief Open a connection with a given type on the specified link. Additional configuration
     * info can be provided via the linkHints param.
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @param linkType The type of link to open: send, receive, or bi-directional.
     * @param linkId The ID of the link that the connection should be opened on.
     * @param linkHints Additional optional configuration information provided by network manager as
     * a stringified JSON Object.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse openConnection(RaceHandle handle, LinkType linkType, LinkID linkId,
                                          std::string linkHints, int32_t sendTimeout) = 0;

    /**
     * @brief Close a connection with a given ID.
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @param connectionId The ID of the connection to close.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse closeConnection(RaceHandle handle, ConnectionID connectionId) = 0;

    /**
     * @brief Destroy the specified link and closed all connections
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @param linkId The LinkID of the link to destroy
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse destroyLink(RaceHandle handle, LinkID linkId) = 0;

    /**
     * @brief Create a link of the channel specified
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @param channelGid The name of the channel to create a link for
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse createLink(RaceHandle handle, std::string channelGid) = 0;

    /**
     * @brief Load a link of the specified channel using the provided LinkAddress
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @param channelGid The name of the channel to create a link for
     * @param linkAddress The LinkAddress to load the link based on
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse loadLinkAddress(RaceHandle handle, std::string channelGid,
                                           std::string linkAddress) = 0;

    /**
     * @brief Load a link of the specified channel using the provided LinkAddresses
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @param channelGid The name of the channel to create a link for
     * @param linkAddress The LinkAddress to load the link based on
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse loadLinkAddresses(RaceHandle handle, std::string channelGid,
                                             std::vector<std::string> linkAddresses) = 0;

    /**
     * @brief create link from address specified by genensis configs
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to create a new link for
     * @param linkAddress The LinkAddress used to create this link
     * @return PluginResponse the status of the SDK in response to this call
     */
    virtual PluginResponse createLinkFromAddress(RaceHandle handle, std::string channelGid,
                                                 std::string linkAddress) = 0;

    /**
     * @brief Deactivate the specified channel, destroying all associated links and closing all
     * associated connections, and setting the channel to unavailable
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @param channelGid The name of the channel to destroy
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse deactivateChannel(RaceHandle handle, std::string channelGid) = 0;

    /**
     * @brief Activate the specified channel, allowing links to be created on it.
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to activate
     * @param roleName The name of the role to activate
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse activateChannel(RaceHandle handle, std::string channelGid,
                                           std::string roleName) = 0;

    /**
     * @brief Notify comms about received user input response
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @param answered True if the response contains an actual answer to the input prompt, otherwise
     * the response is an empty string and not valid
     * @param response The user response answer to the input prompt
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onUserInputReceived(RaceHandle handle, bool answered,
                                               const std::string &response) = 0;

    /**
     * @brief Notify the plugin that the user acknowledged the displayed information
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onUserAcknowledgementReceived(RaceHandle handle) = 0;

    /**
     * @brief Serve files located in the specified directory. The files served are associated with
     * the specified link and may stop being served when the link is closed.
     *
     * @param linkId The link to serve the files on
     * @param path A path to the directory containing the files to serve
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse serveFiles(LinkID /*linkId*/, std::string /*path*/) {
        return PLUGIN_ERROR;
    };

    /**
     * @brief Create a bootstrap link of the channel specified with the specified passphrase
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @param channelGid The name of the channel to create a link for
     * @param passphrase The passphrase to use with the link
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse createBootstrapLink(RaceHandle /*handle*/, std::string /*channelGid*/,
                                               std::string /*passphrase*/) {
        return PLUGIN_ERROR;
    };

    /**
     * @brief Flush any pending encrypted packages queued to be sent out over the given channel.
     * @param handle The handle for asynchronously returning the status of this call.
     * @param channelGid The ID of the channel to flush.
     * @param batchId The batch ID of the encrypted packages to be flushed. If batch ID is set to
     * zero (the null value) this call will return an error. A valid batch ID must be provided.
     * @return PluginResponse Status of plugin indicating success or failure of the call.
     */
    virtual PluginResponse flushChannel(RaceHandle handle, std::string channelGid,
                                        uint64_t batchId) = 0;
};

extern "C" EXPORT IRacePluginComms *createPluginComms(IRaceSdkComms *sdk);
extern "C" EXPORT void destroyPluginComms(IRacePluginComms *plugin);

#endif
