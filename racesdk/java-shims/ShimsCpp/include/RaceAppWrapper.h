
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

#ifndef __RACE_CLIENT_WRAPPER_H_
#define __RACE_CLIENT_WRAPPER_H_

#include <jni.h>

#include "racetestapp/RaceApp.h"
#include "racetestapp/RaceTestAppOutputLog.h"

class RaceAppWrapper : public RaceApp {
public:
    /**
     * @brief Construct a new Race Client Wrapper object
     *
     */
    RaceAppWrapper(RaceTestAppOutputLog &output, IRaceSdkApp &raceSdk,
                   std::shared_ptr<opentracing::Tracer> tracer, JNIEnv *env, jclass jRaceAppClass,
                   jobject jRaceApp);

    /**
     * @brief Destroy the Race Client Wrapper object
     *
     */
    virtual ~RaceAppWrapper();

    /**
     * @brief Handle a received message. For example, the client may simply present the message to
     * the user.
     *
     * @param msg The received message.
     */
    virtual void handleReceivedMessage(ClrMsg msg) override;

    /**
     * @brief Callback for update the SDK on clear message status.
     *
     * @param handle The handle passed into processClrMsg when the network manager plugin initally
     * received the relevant clear message.
     * @param status The new status of the message.
     * @return SdkResponse the status of the SDK in response to this call.
     */
    virtual void onMessageStatusChanged(RaceHandle handle, MessageStatus status) override;

    /**
     * @brief Add a send message that was created/sent through RaceTestApp to the UI
     *
     * @param msg The clear text message to send for the client application.
     */
    virtual void addMessageToUI(const ClrMsg &msg) override;

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
     * @param key User input key
     * @param prompt User input prompt
     * @param cache If true, the response will be cached
     * @return SdkResponse object that contains whether the post was successful
     */
    virtual SdkResponse requestUserInput(RaceHandle handle, const std::string &pluginId,
                                         const std::string &key, const std::string &prompt,
                                         bool cache) override;

    /**
     * @brief Requests input from the user
     *
     * The task posted to the work queue will lookup a response for the user input prompt, wait an
     * optional amount of time, then notify the SDK of the user response.
     *
     * @param handle The RaceHandle to use for the onUserInputReceived callback
     * @param pluginId Plugin ID of the plugin requesting user input (or "Common" for common user
     *      input)
     * @param key User input key
     * @param prompt User input prompt
     * @param cache If true, the response will be cached
     * @return SdkResponse object that contains whether the post was successful
     */
    SdkResponse nativeRequestUserInput(RaceHandle handle, const std::string &pluginId,
                                       const std::string &key, const std::string &prompt,
                                       bool cache);

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

private:
    jclass jRaceAppClass;
    jobject jRaceApp;

    jmethodID jHandleReceivedMessageMethodId;
    jmethodID jAddMessageToUIMethodId;
    jmethodID jOnMessageStatusChangedMethodId;
    jmethodID jOnSdkStatusChangedMethodId;
    jmethodID jDisplayInfoToUserMethodId;
    jmethodID jDisplayBootstrapInfoToUserMethodId;
    jmethodID jRequestUserInputMethodId;
};

#endif
