
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

#ifndef __SOURCE_RACE_APP_H__
#define __SOURCE_RACE_APP_H__

#include <IRaceApp.h>
#include <IRaceSdkApp.h>
#include <opentracing/tracer.h>

#include <atomic>
#include <vector>

#include "SdkResponse.h"
#include "UserInputResponseCache.h"
#include "UserInputResponseParser.h"
#include "racetestapp/IRaceTestAppOutput.h"

class UserInputResponseCache;
class UserInputResponseParser;

/**
 * @brief Implementation of the IRaceApp interface.
 *
 */
class RaceApp : public IRaceApp {
public:
    /**
     * @brief Constructor.
     *
     * @param appOutput The application output used for logging received messages.
     * @param tracer The opentracing tracer used for logging received messages.
     */
    explicit RaceApp(IRaceTestAppOutput &appOutput, IRaceSdkApp &_raceSdk,
                     std::shared_ptr<opentracing::Tracer> tracer);

    /**
     * @brief Add a send message that was created/sent through RaceTestApp to the UI
     *
     * @param msg The clear text message to send for the client application.
     */
    virtual void addMessageToUI(const ClrMsg &msg);

    /**
     * @brief Handle a message received over the RACE network.
     *
     * @param msg The message received over the RACE network.
     */
    virtual void handleReceivedMessage(ClrMsg msg) override;

    /**
     * @brief Callback for the SDK to update the app on clear message status.
     *
     * @param handle The handle passed into processClrMsg when the network manager plugin initally
     * received the relevant clear message.
     * @param status The new status of the message.
     */
    virtual void onMessageStatusChanged(RaceHandle handle, MessageStatus status) override;

    /* onSdkStatusChanged: call onSdkStatusChanged on the wrapped app
     * onSdkStatusChanged will be called on the app thread.
     *
     * @param sdkStatus The changed status
     */
    virtual void onSdkStatusChanged(const nlohmann::json &sdkStatus) override;

    virtual nlohmann::json getSdkStatus() override;

    /**
     * @brief Requests input from the user
     *
     * The task posted to the work queue will lookup a response for the user input prompt, wait an
     * optional amount of time, then notify the SDK of the user response.
     *
     * @param handle The RaceHandle to use for the onUserInputReceived callback
     * @param pluginId Plugin ID of the plugin requesting user input (or "Common" for common user
     *      input)
     * @param prompt User input key
     * @param prompt User input prompt
     * @param cache If true, the response will be cached
     * @return SdkResponse object that contains whether the post was successful
     */
    virtual SdkResponse requestUserInput(RaceHandle handle, const std::string &pluginId,
                                         const std::string &key, const std::string &prompt,
                                         bool cache) override;

    /**
     * @brief Displays information to the User
     *
     * The task posted to the work queue will display information to the user input prompt, wait an
     * optional amount of time, then notify the SDK of the user acknowledgment.
     *
     * @param handle The RaceHandle to use for the onUserInputReceived callback
     * @param data data to display
     * @param displayType type of user display to display data in
     * @return SdkResponse object that contains whether the post was successful
     */
    virtual SdkResponse displayInfoToUser(RaceHandle handle, const std::string &data,
                                          RaceEnums::UserDisplayType displayType) override;

    /**
     * @brief Displays information to the User and forward information to target node for
     * automated testing
     *
     * @param handle The RaceHandle to use for the onUserInputReceived callback
     * @param data data to display
     * @param displayType type of user display to display data in
     * @param actionType type of action the Daemon must take
     * @return SdkResponse
     */
    virtual SdkResponse displayBootstrapInfoToUser(
        RaceHandle handle, const std::string &data, RaceEnums::UserDisplayType displayType,
        RaceEnums::BootstrapActionType actionType) override;

    std::tuple<bool, std::string> getCachedResponse(const std::string &pluginId,
                                                    const std::string &key);
    std::tuple<bool, std::string> getAutoResponse(const std::string &pluginId,
                                                  const std::string &key);
    bool setCachedResponse(const std::string &pluginId, const std::string &key,
                           const std::string &response);

protected:
    IRaceTestAppOutput &appOutput;

    /**
     * @brief The interface used to interact with the RACE SDK.
     *
     */
    IRaceSdkApp &raceSdk;

    std::shared_ptr<opentracing::Tracer> tracer;
    bool ready;
    std::unique_ptr<UserInputResponseCache> responseCache;
    std::unique_ptr<UserInputResponseParser> responseParser;
    nlohmann::json currentSdkStatus;
};

#endif
