
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

#ifndef _LOADER_PLUGIN_COMMS_RUST_CPP_WRAPPER_H__
#define _LOADER_PLUGIN_COMMS_RUST_CPP_WRAPPER_H__

#include <IRacePluginComms.h>
#include <IRaceSdkComms.h>
#include <string.h>  // strncpy

#include <iostream>

#include "helper.h"
#include "plugin_extern_c.h"

class PluginCommsRustCppWrapper : public IRacePluginComms {
public:
    explicit PluginCommsRustCppWrapper(IRaceSdkComms *_sdk);
    virtual ~PluginCommsRustCppWrapper();

    /**
     * @brief Set the Sdk object and perform minimum work to
     * be abe to respond to incoming calls. Do not use any calls
     * to raceSdk that require the network manager. Use minimal calls to raceSdk.
     *
     * @param sdk Pointer to the SDK instance.
     * @param pluginConfig Config object containing dynamic config variables (e.g. paths)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse init(const PluginConfig &pluginConfig) override;

    /**
     * @brief Shutdown the plugin. Close open connections, remove state, etc.
     *
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse shutdown() override;

    /**
     * @brief Send an encrypted package.
     *
     * @param handle The RaceHandle to use for updating package status in onPackageStatusChanged
     * calls
     * @param connectionId The ID of the connection to use to send the package.
     * @param pkg The encrypted package to send.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse sendPackage(RaceHandle handle, ConnectionID connectionId, EncPkg pkg,
                                       double timeoutTimestamp, uint64_t batchId) override;

    /**
     * @brief Open a connection with a given type on the specified link. Additional configuration
     * info can be provided via the linkHints param.
     *
     * @param handle The RaceHandle to use for onConnectionStatusChanged calls
     * @param linkType The type of link to open: send, receive, or bi-directional.
     * @param linkId The ID of the link that the connection should be opened on.
     * @param config Configuration string for this link, registered by the plugin in
     * registerLinkProfile
     * @param linkHints Additional optional configuration information provided by network manager as
     * a stringified JSON Object.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse openConnection(RaceHandle handle, LinkType linkType, LinkID linkId,
                                          std::string linkHints, int32_t sendTimeout) override;

    /**
     * @brief Close a connection with a given ID.
     *
     * @param handle The RaceHandle to use for onConnectionStatusChanged calls
     * @param connectionId The ID of the connection to close.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse closeConnection(RaceHandle handle, ConnectionID connectionId) override;

    /**
     * @brief Destroy the specified link and closed all connections
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param linkId The LinkID of the link to destroy
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse destroyLink(RaceHandle handle, LinkID linkId) override;

    /**
     * @brief Create a link of the channel specified
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to create a link for
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse createLink(RaceHandle handle, std::string channelGid) override;

    /**
     * @brief Load a link of the specified channel using the provided LinkAddress
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to create a link for
     * @param linkAddress The LinkAddress to load the link based on
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse loadLinkAddress(RaceHandle handle, std::string channelGid,
                                           std::string linkAddress) override;

    /**
     * @brief Load a link of the specified channel using the provided LinkAddresses
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to create a link for
     * @param linkAddress The LinkAddress to load the link based on
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse loadLinkAddresses(RaceHandle handle, std::string channelGid,
                                             std::vector<std::string> linkAddresses) override;

    virtual PluginResponse createLinkFromAddress(RaceHandle handle, std::string channelGid,
                                                 std::string linkAddress) override;

    /**
     * @brief Deactivate the specified channel, destroying all associated links and closing all
     * associated connections, and setting the channel to unavailable
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to destroy
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse deactivateChannel(RaceHandle handle, std::string channelGid) override;

    /**
     * @brief Activate the specified channel, allowing links to be created on it.
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to activate
     * @param roleName The name of the role to activate
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse activateChannel(RaceHandle handle, std::string channelGid,
                                           std::string roleName) override;

    /**
     * @brief Notify comms about received user input response
     *
     * @param handle The handle for this callback
     * @param answered True if the response contains an actual answer to the input prompt, otherwise
     * the response is an empty string and not valid
     * @param response The user response answer to the input prompt
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onUserInputReceived(RaceHandle handle, bool answered,
                                               const std::string &response) override;

    // cppcheck-suppress passedByValue
    virtual PluginResponse flushChannel(RaceHandle handle, const std::string channelGid,
                                        uint64_t batchId) override;

    /**
     * @brief Notify the plugin that the user acknowledged the displayed information
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onUserAcknowledgementReceived(RaceHandle handle) override;

private:
    void destroyPlugin();

private:
    IRaceSdkComms *sdk;
    void *plugin;
};

#endif
