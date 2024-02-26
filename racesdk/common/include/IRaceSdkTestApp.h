
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

#ifndef __I_RACE_SDK_CLIENT_H_
#define __I_RACE_SDK_CLIENT_H_

#include <string>
#include <vector>

#include "ClrMsg.h"
#include "IRaceSdkApp.h"

/**
 * @brief Interface for the client to interact with the Race SDK.
 *
 */
class IRaceSdkTestApp : public IRaceSdkApp {
public:
    /**
     * @brief Destroy the IRaceSdkTestApp object
     *
     */
    virtual ~IRaceSdkTestApp() {}

    /**
     * @brief Send a message directly using a comms plugin, bypassing the network manager plugin.
     *
     * If the given route specifies a particular connection ID (e.g.,
     * <plugin-id>/<channel-id>/<link-id>/<conn-id>), that connection will be used to send the
     * message. If the route specifies a link ID (e.g., <plugin-id>/<channel-id>/<link-id>), a
     * temporary connection will be opened for the specified link and closed after the message is
     * sent. If the route only specifies a channel ID (e.g., <plugin-id>/channel-id> or just
     * <channel-id>), then the first discovered link will be used to open a temporary connection.
     *
     * @param msg The clear text message to send for the client application.
     * @param route A specific connection ID, link ID, or channel ID over which to
     *      send the message.
     */
    virtual void sendNMBypassMessage(ClrMsg msg, const std::string &route) = 0;

    /**
     * @brief Open a receive connection from the specified persona.
     *
     * If the given route specifies a Link ID (e.g., <plugin-id>/<channel-id>/<link-id>), a
     * temporary connection will be opened for the specified link and closed after a message
     * is received. If the route only specified a channel ID (e.g., <plugin-id>/<channel-id>
     * or just <channel-id>), then the first discovered link will be used to open a temporary
     * connection.
     *
     * @param persona The persona from which to open a receive connection
     * @param route A specific link ID or channel ID over which to receive
     */
    virtual void openNMBypassReceiveConnection(const std::string &persona,
                                               const std::string &route) = 0;

    /**
     * @brief Deactivate the specified channel.
     *
     * @param channelGid Name of the channel to deactivate
     */
    virtual void rpcDeactivateChannel(const std::string &channelGid) = 0;

#pragma clang diagnostic push
// Have to disable comment diagnostic warnings/errors because it'll complain about "/*" being within
// a block comment--but that's exactly what we're trying to describe in the function comments below.
#pragma clang diagnostic ignored "-Wcomment"

    /**
     * @brief Destroy the specified link. If specified in the form "<channelGid>/*" then all links
     * for the specified channel will be destroyed.
     *
     * @param linkId ID of the link to destroy, or "<channelGid>/*" to destroy all links for a
     *      channel
     */
    virtual void rpcDestroyLink(const std::string &linkId) = 0;

    /**
     * @brief Close the specified connection. If specified in the form "<linkId>/*" then all
     * connections for the specified link will be destroyed.
     *
     * @param connectionId ID of the connection to close, or "<linkId>/*" to close all connections
     *      for a link
     */
    virtual void rpcCloseConnection(const std::string &connectionId) = 0;

    /**
     * @brief Notify network manager to perform epoch changeover processing
     *
     * @param data Data associated with epoch (network manager implementation specific)
     */
    virtual void rpcNotifyEpoch(const std::string &data) = 0;

#pragma clang diagnostic pop

    /**
     * @brief Get the initial set of channels to be enabled, based on the RACE config. If no
     * explicit configuration exists, all channels will included.
     *
     * @return Names of channels to be initially enabled
     */
    virtual std::vector<std::string> getInitialEnabledChannels() = 0;
};

#endif
