
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

#ifndef __I_RACE_SDK_COMMS_H_
#define __I_RACE_SDK_COMMS_H_

#include <vector>

#include "ChannelProperties.h"
#include "ChannelStatus.h"
#include "ConnectionStatus.h"
#include "EncPkg.h"
#include "IRaceSdkCommon.h"
#include "LinkProperties.h"
#include "LinkStatus.h"
#include "PackageStatus.h"
#include "SdkResponse.h"

class IRaceSdkComms : public IRaceSdkCommon {
public:
    /**
     * @brief Destroy the IRaceSdkComms object
     *
     */
    virtual ~IRaceSdkComms() {}

    /**
     * @brief Notify network manager via the SDK that the status of this package has changed.
     *
     * @param handle The RaceHandle identifying the sendEncryptedPackage call it corresponds to
     * @param status The new PackageStatus
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call, negative
     * values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse onPackageStatusChanged(RaceHandle handle, PackageStatus status,
                                               int32_t timeout) = 0;

    /**
     * @brief Notify network manager via the SDK that the stauts of a connection has changed
     *
     * @param handle The RaceHandle identifying the network manager call it corresponds to
     * @param status The new ConnectionStatus
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse onConnectionStatusChanged(RaceHandle handle, ConnectionID connId,
                                                  ConnectionStatus status,
                                                  LinkProperties properties, int32_t timeout) = 0;

    /**
     * @brief Notify network manager via the SDK that the stauts of a channel has changed
     *
     * @param handle The RaceHandle identifying the network manager call it corresponds to (if any)
     * @param channelGid The name of the channel this status pertains to
     * @param status The new ChannelStatus
     * @param properties The ChannelProperties of the channel
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse onChannelStatusChanged(RaceHandle handle, std::string channelGid,
                                               ChannelStatus status, ChannelProperties properties,
                                               int32_t timeout) = 0;

    /**
     * @brief Notify network manager via the SDK that the stauts of a link has changed
     *
     * @param handle The RaceHandle identifying the network manager call it corresponds to (if any)
     * @param channelGid The linkId the link this status pertains to
     * @param status The new LinkStatus
     * @param properties The LinkProperties of the link
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse onLinkStatusChanged(RaceHandle handle, LinkID linkId, LinkStatus status,
                                            LinkProperties properties, int32_t timeout) = 0;

    /**
     * @brief Notify the SDK and network manager of a change in LinkProperties
     *
     * @param linkId The LinkID identifying the link with updated properties
     * @param status The new LinkProperties
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse updateLinkProperties(LinkID linkId, LinkProperties properties,
                                             int32_t timeout) = 0;

    /**
     * @brief Request the SDK to create a new ConnectionID for the plugin
     *
     * @param linkId The LinkID for the link the connection will be on
     * @return ConnectionID the ID for the connection provided by the SDK
     */
    virtual ConnectionID generateConnectionId(LinkID linkId) = 0;

    /**
     * @brief Request the SDK to create a new LinkID for the plugin/channel
     *
     * @param channelGid The name of the channel this link instantiates
     * @return LinkID the ID for the link provided by the SDK
     */
    virtual LinkID generateLinkId(std::string channelGid) = 0;

    /**
     * @brief Notify network manager via the SDK of a new EncPkg that was received
     *
     * @param pkg The EncPkg that was received
     * @param connIDs The Vector of ConnectionIDs the pkg was received on
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    virtual SdkResponse receiveEncPkg(const EncPkg &pkg, const std::vector<ConnectionID> &connIDs,
                                      int32_t timeout) = 0;

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

    /**
     * @brief Displays information to the User and forward information to target node for
     * automated testing
     *
     * @param data data to display
     * @param displayType type of user display to display data in
     * @param actionType type of action the Daemon must take
     * @return SdkResponse
     */
    virtual SdkResponse displayBootstrapInfoToUser(const std::string &data,
                                                   RaceEnums::UserDisplayType displayType,
                                                   RaceEnums::BootstrapActionType actionType) = 0;

    /**
     * @brief Unblock the queue for a connection previously blocked by a return value of
     * PLUGIN_TEMP_ERROR.
     *
     * @param connId The connection to unblock
     * @return SdkResponse
     */
    virtual SdkResponse unblockQueue(ConnectionID connId) = 0;
};

#endif
