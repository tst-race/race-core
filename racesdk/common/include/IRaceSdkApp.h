
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

#ifndef __I_RACE_SDK_APP_H_
#define __I_RACE_SDK_APP_H_

#include "AppConfig.h"
#include "DeviceInfo.h"
#include "IRaceApp.h"
#include "IRaceSdkCommon.h"

class IRaceSdkApp : public IRaceSdkCommon {
public:
    /**
     * @brief Destroy the IRaceSdkApp object
     *
     */
    virtual ~IRaceSdkApp() {}

    virtual const AppConfig &getAppConfig() const = 0;

    /**
     * @brief Initialize the RACE system with the supplied config. This must be called first before
     * calling any other APIs from the app. After this call, the MPC and network obfuscation
     * plugins will be enabled and initialized, capable of encrypting and transmitting messages to
     * be routed over the RACE Network.
     *
     * @param app pointer to IRaceApp object
     */
    virtual bool initRaceSystem(IRaceApp *app) = 0;

    /**
     * @brief Introduce a new node into the network
     *
     * @param deviceInfo Info about the device being introduced into the network
     * @param passphrase phrase used to secure communication to the new device
     * @return valid RaceHandle on success, otherwise NULL_RACE_HANDLE
     */
    virtual RaceHandle prepareToBootstrap(DeviceInfo deviceInfo, std::string passphrase,
                                          std::string bootstrapChannelId) = 0;

    /**
     * @brief Cancel the current bootstrap process
     *
     * @param bootstrapHandle the handle that returned from prepareToBootstrap
     * @return bool true on success
     */
    virtual bool cancelBootstrap(RaceHandle /*bootstrapHandle*/) {
        return false;
    }

    /**
     * @brief Notify the requesting plugin via the SDK of the received user input
     * in response to a prompt request.
     *
     * @param handle The handle associated with the initial user input request
     * @param answered True if the user provided a response, else the user cancelled or
     *     the app timed out waiting for the user
     * @param response User input response text
     *
     * @return SdkResponse The status of the SDK in response to this call
     */
    virtual SdkResponse onUserInputReceived(RaceHandle handle, bool answered,
                                            const std::string &response) = 0;

    /**
     * @brief Notify the plugin that the user acknowledged the displayed information
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @return SdkResponse the status of the sdk in response to this call
     */
    virtual SdkResponse onUserAcknowledgementReceived(RaceHandle handle) = 0;

    /**
     * @brief Send a message from the client application.
     *
     * @param msg The clear text message to send for the client application.
     *
     * @return The RaceHandle associated with sending this clear message.
     */
    virtual RaceHandle sendClientMessage(ClrMsg msg) = 0;

    /**
     * @brief Invoke a VoA rule addition action
     *
     * @param payload The action payload
     *
     * @return A success flag
     */
    virtual bool addVoaRules(const nlohmann::json &payload) = 0;

    /**
     * @brief Invoke a VoA rule deletion action
     *
     * @param payload The action payload
     *
     * @return A success flag
     */
    virtual bool deleteVoaRules(const nlohmann::json &payload) = 0;

    /**
     * @brief Set VoA state (active or not)
     *
     * @param payload The desired state
     */
    virtual void setVoaActiveState(bool state) = 0;

    /**
     * @brief Set all channels enabled for covert communication. This method can only be
     * invoked prior to calling initRaceSystem and is intended to specify an initial set of
     * enabled channels. After initRaceSystem has been run, use the enableChannel and
     * disableChannel methods to change the channel selection.
     *
     * @param channelGids The names of all channels to be enabled
     */
    virtual bool setEnabledChannels(const std::vector<std::string> &channelGids) = 0;

    /**
     * @brief Enable a channel for covert communication. This will allow the channel to be activated
     * by the network manager plugin.
     *
     * @param channelGid The name of the channel to enable
     */
    virtual bool enableChannel(const std::string &channelGid) = 0;

    /**
     * @brief Disable a channel for covert communication. This will cause the channel to be
     * deactivated and no longer allow it to be used by the network manager.
     *
     * @param channelGid The name of the channel to disable
     */
    virtual bool disableChannel(const std::string &channelGid) = 0;

    /**
     * @brief Get a list of all contacts that can be sent messages from the client.
     *
     * @return std::vector<std::string> Vector of personas that can be
     *       reached by the client.
     */
    virtual std::vector<std::string> getContacts() = 0;

    /**
     * @brief Check if the client is connected to the network and ready to send/receive messages.
     *
     * @return true The client is connected and ready run.
     * @return false The client is not connected.
     */
    virtual bool isConnected() = 0;

    // TODO: can these be consolidated to a single function?
    /**
     * @brief Initiate shutdown of the server node. Ensure any persisted data is written and synced
     * to the filesystem and all network connections terminated.
     *
     */
    virtual void cleanShutdown() = 0;

    /**
     * @brief Notify the server node of a planned shutdown numSeconds from now.
     *
     * @param numSeconds The time (in seconds) until the shutdown will occur.
     */
    virtual void notifyShutdown(std::int32_t numSeconds) = 0;
};

#endif
