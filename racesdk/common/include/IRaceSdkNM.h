
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

#ifndef __I_RACE_SDK_NETWORK_MANAGER_H_
#define __I_RACE_SDK_NETWORK_MANAGER_H_

#include <map>
#include <string>
#include <vector>

#include "ChannelProperties.h"
#include "ClrMsg.h"
#include "EncPkg.h"
#include "IRaceSdkCommon.h"
#include "LinkProperties.h"
#include "MessageStatus.h"
#include "PluginStatus.h"
#include "SdkResponse.h"

class IRaceSdkNM : public IRaceSdkCommon {
public:
    /**
     * @brief Destroy the IRaceSdkNM object
     *
     */
    virtual ~IRaceSdkNM() {}

    /**
     * @brief Notify the Race App of status change (e.g. when it is ready to send client messages)
     *
     * @param pluginStatus The status of the plugin
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse onPluginStatusChanged(PluginStatus pluginStatus) = 0;

    /**
     * @brief Pass an EncPkg to a comms channel via the SDK to send out
     *
     * @param ePkg The EncPkg to send
     * @param connectionId The ConnectionID for the connection to use to send it out - will place
     * the send call on a queue for that particular connection
     * arguments
     * @param batchId An ID to "batch" encrypted packages. Used by the flushChannel API so that
     * all encrypted packages in a batch will be flushed together. Set this to zero if you don't
     * care about flushing or if the connection you're using does not support flushing.
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse sendEncryptedPackage(EncPkg ePkg, ConnectionID connectionId,
                                             uint64_t batchId, int32_t timeout) = 0;

    /**
     * @brief Pass a ClrMsg to the client or server Race App (likely for presentation to the user)
     *
     * @param msg The ClrMsg to present
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse presentCleartextMessage(ClrMsg msg) = 0;

    /**
     * @brief Open a connection of a given type on the given link. Additional link-specific options
     * can be specified in the linkHints string as JSON. The ConnID of the new connection will be
     * provided in the onConnectionStatusChanged call with the handle matching the handle returned
     * in this SdkResponse object.
     *
     * @param linkType The link type: send, receive, or bi-directional.
     * @param linkId The ID of the link to open the connection on.
     * @param linkHints Additional optional configuration information provided by network manager as
     * a stringified JSON Object. May be ignored/honored by the comms plugin.
     * @param priority The priority of this call, higher is more important
     * @param sendTimeout If a package sent on this link takes songer than this many seconds,
     * generate a package failed callback.
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse openConnection(LinkType linkType, LinkID linkId, std::string linkHints,
                                       int32_t priority, int32_t sendTimeout, int32_t timeout) = 0;

    /**
     * @brief Close a connection with a given ID.
     *
     * @param connectionId The ID of the connection to close.
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse closeConnection(ConnectionID connectionId, int32_t timeout) = 0;

    /**
     * @brief Get all of the links that connect to a set of personas.
     *
     * @param recipientPersonas The personas that the links can connect to. All personas must be
     * reachable.
     * @param linkType The type of links to get: send, receive, or bi-directional.
     * @return std::vector<LinkID> Vector of link IDs that can connect to the given persona.
     */
    virtual std::vector<LinkID> getLinksForPersonas(std::vector<std::string> recipientPersonas,
                                                    LinkType linkType) = 0;

    /**
     * @brief Get the links for a given channel.
     *
     * @param channelGid The ID of the channel to get links for.
     * @return std::vector<LinkID> A vector of the links for the given channel.
     */
    virtual std::vector<LinkID> getLinksForChannel(std::string channelGid) = 0;

    /**
     * @brief Get the LinkID of the link the connection specified by this ConnectionID is one
     *
     * @param connectionId The ConnectionID of the connection
     * @return LinkID of the link the connection is on or ""
     */
    virtual LinkID getLinkForConnection(ConnectionID connectionId) = 0;

    /**
     * @brief Request properties of a link with a given type and ID. This data will be queried from
     * the internal cache held by core.
     *
     * @param linkId The ID of the link.
     * @return LinkProperties The properties of the requested link.
     */
    virtual LinkProperties getLinkProperties(LinkID linkId) = 0;

    /**
     * @brief Get a map of channelGid:ChannelProperties for all channels that can be created/loaded.
     *
     * @return std::map<string, ChannelProperties> of supported channels
     */
    virtual std::map<std::string, ChannelProperties> getSupportedChannels() = 0;

    /**
     * @brief Deactivate a channel. This should set this channel status to CHANNEL_ENABLED,
     * destroy all links based on this channel, close all connections on that link and fail all
     * packages queued for them.
     *
     * @param channelGid The name of the channel to deactivate
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse deactivateChannel(std::string channelGid, std::int32_t timeout) = 0;

    // TODO: documentation
    virtual SdkResponse activateChannel(std::string channelGid, std::string roleName,
                                        std::int32_t timeout) = 0;

    /**
     * @brief Destroy a link. This should close all connections on that link and fail all packages
     * queued for them.
     *
     * @param linkId The LinkID of the link to destroy
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse destroyLink(LinkID linkId, std::int32_t timeout) = 0;

    /**
     * @brief Create a new link of this type of channel and associate it with a list of personas.
     * This means this node will operate the CREATOR side of the link and the generated LinkAddress
     * will need to be shared to another node to call loadLinkAddress on.
     *
     * @param channelGid The name of the channel to create a new link for
     * @param personas The list of personas to associate with this link
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse createLink(std::string channelGid, std::vector<std::string> personas,
                                   std::int32_t timeout) = 0;

    /**
     * @brief Load a new link of this channel type using this LinkAddress and associate it with a
     * list of personas.
     *
     * @param channelGid The name of the channel to create a new link for
     * @param linkAddress The LinkAddress used to load this link
     * @param personas The list of personas to associate this link with
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse loadLinkAddress(std::string channelGid, std::string linkAddress,
                                        std::vector<std::string> personas,
                                        std::int32_t timeout) = 0;

    /**
     * @brief Load a new link of this channel type using a list of LinkAddresses and associate it
     * with a list of personas. NOTE: not all channels will implement this method because many links
     * do not support multiple addresses. This is primarily meant for service-addressed channels
     * like those using email.
     *
     * @param channelGid The name of the channel to create a new link for
     * @param linkAddresses The list of LinkAddresses used to load this link
     * @param personas The list of personas to associate this link with
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse loadLinkAddresses(std::string channelGid,
                                          std::vector<std::string> linkAddresses,
                                          std::vector<std::string> personas,
                                          std::int32_t timeout) = 0;

    /**
     * @brief create link from address specified by genensis configs
     *
     * @param channelGid The name of the channel to create a new link for
     * @param linkAddress The LinkAddress used to create this link
     * @param personas The list of personas to associate this link with
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse createLinkFromAddress(std::string channelGid, std::string linkAddress,
                                              std::vector<std::string> personas,
                                              std::int32_t timeout) = 0;

    /**
     * @brief Bootstrap a node with the specified configs
     *
     * @param handle The handle passed to network manager in prepareToBootstrap
     * @param commsChannels the list of comms plugins to install in the bootstrapp
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse bootstrapDevice(RaceHandle handle,
                                        std::vector<std::string> commsChannels) = 0;

    /**
     * @brief Inform the sdk that a bootstrap has failed
     *
     * @param handle The handle passed to network manager in prepareToBootstrap
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse bootstrapFailed(RaceHandle handle) = 0;

    /**
     * @brief Set the personas associated with a link.
     *
     * @param linkId The LinkId of the link to set personas for
     * @param personas The list of personas to set associated with this link.
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse setPersonasForLink(std::string linkId,
                                           std::vector<std::string> personas) = 0;

    /**
     * @brief Get the list of personas associated with a link.
     *
     * @param linkId The LinkId of the link to get personas for
     * @return The list of personas associated with this link
     */
    virtual std::vector<std::string> getPersonasForLink(std::string linkId) = 0;

    /**
     * @brief Callback for update the SDK on clear message status.
     *
     * @param handle The handle passed into processClrMsg when the network manager plugin initally
     * received the relevant clear message.
     * @param status The new status of the message.
     * @return SdkResponse the status of the SDK in response to this call.
     */
    virtual SdkResponse onMessageStatusChanged(RaceHandle handle, MessageStatus status) = 0;

    /**
     * @brief Send a bootstrap package containing the persona and package used for enrollment of
     * this node over the specified connection
     *
     * @param connectionId The ConnectionId of the connect to send the package over
     * @param persona The persona of this node
     * @param pkg The pkg used by the bootstrapping node to enroll this node
     * @return SdkResponse the status of the SDK in response to this call.
     */
    virtual SdkResponse sendBootstrapPkg(ConnectionID connectionId, std::string persona,
                                         RawData pkg, int32_t timeout) = 0;
    /**
     * @brief Request plugin-specific input from the user with the specified prompt message.
     * The response may be cached in persistent storage, in which case the user will not be
     * re-prompted if a cached response exists for the given prompt.
     *
     * The response will be provided in the userInputReceived callback with the handle matching
     * the handle returned in this SdkResponse object.
     *
     * @param key Prompt identifier for the user input request
     * @param prompt Message to be presented to the user
     * @param cache If true, the response will be cached in persistent storage
     * @return SdkResponse indicator of success or failure of the request
     */
    virtual SdkResponse requestPluginUserInput(const std::string &key, const std::string &prompt,
                                               bool cache) = 0;

    /**
     * @brief Request application-wide input from the user associated with the given key.
     * The key identifies a common user input prompt and must be a key supported by the
     * RACE SDK. The response is cached in persistent storage, so the user will not be
     * re-prompted if a cached response exists for the given key.
     *
     * The response will be provided in the userInputReceived callback with the handle matching
     * the handle returned in this SdkResponse object.
     *
     * @param key Prompt identifier for the application-wide user input request
     * @return SdkResponse indicator of success or failure of the request
     */
    virtual SdkResponse requestCommonUserInput(const std::string &key) = 0;

    /**
     * @brief Flush any pending encrypted packages queued to be sent out over the given channel.
     * @param handle The handle for asynchronously returning the status of this call.
     * @param channelGid The ID of the channel to flush.
     * @param batchId The batch ID of the encrypted packages to be flushed. If batch ID is set to
     * zero (the null value) this call will return an error. A valid batch ID must be provided.
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse Status of SDK indicating success or failure of the call.
     */
    virtual SdkResponse flushChannel(std::string channelGid, uint64_t batchId, int32_t timeout) = 0;

    /**
     * @brief Displays information to the User
     *
     * The task posted to the work queue will display information to the user input prompt, wait an
     * optional amount of time, then notify the SDK of the user acknowledgment.
     *
     * @param data data to display
     * @param displayType type of user display to display data in
     * @return SdkResponse object that contains whether the post was successful
     */
    virtual SdkResponse displayInfoToUser(const std::string &data,
                                          RaceEnums::UserDisplayType displayType) = 0;
};

#endif
