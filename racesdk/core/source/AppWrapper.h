
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

#ifndef __CLIENT_WRAPPER_H__
#define __CLIENT_WRAPPER_H__

#include "Handler.h"
#include "IRaceApp.h"
#include "OpenTracingForwardDeclarations.h"
#include "RaceSdk.h"

/*
 * AppWrapper: A wrapper for a client plugin that calls associated methods on a separate plugin
 * thread
 */
class AppWrapper : public IRaceApp {
private:
    std::shared_ptr<opentracing::Tracer> mTracer;
    IRaceApp *mClient;
    Handler mThreadHandler;

    // nextPostId is used to identify which post matches with which call/return log.
    std::atomic<uint64_t> nextPostId;

public:
    AppWrapper(IRaceApp *client, RaceSdk &raceSdk) :
        mTracer(raceSdk.getTracer()),
        mClient(client),
        mThreadHandler("app-thread", raceSdk.getRaceConfig().wrapperQueueMaxSize,
                       raceSdk.getRaceConfig().wrapperTotalMaxSize),
        nextPostId(0) {}
    virtual ~AppWrapper() {}

    /* startHandler: start the client thread
     *
     * This starts the internally managed thread on which methods of the wrapped client are run.
     * Calling a method that executes something on this thread before calling startHandler will
     * schedule the client method to be called once startHandler is called.
     */
    void startHandler();

    /* stopHandler: stop the plugin thread
     *
     * This stops the internally managed thread on which methods of the wrapped plugin are run. Any
     * callbacks posted, but not yet completed will be finished. Attempting to post a new callback
     * will fail.
     */
    void stopHandler();

    /* handleReceivedMessage: call handleReceivedMessage on the wrapped plugin
     *
     * handleReceivedMessage will be called on the plugin thread. handleReceivedMessage may return
     * before the handleReceivedMessage method of the wrapped plugin is complete.
     */
    virtual void handleReceivedMessage(ClrMsg msg) override;

    /* onMessageStatusChanged: call onMessageStatusChanged on the wrapped client
     *
     * onMessageStatusChanged will be called on a separate thread. This call may return before the
     * onMessageStatusChanged method of the wrapped client is complete.
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
     * @brief Wait for all callbacks to finish, used for testing.
     */
    void waitForCallbacks();

    /**
     * @brief Checks if the given key is a valid common user input prompt key
     *
     * @param key User input prompt key
     * @return True if key is valid
     */
    bool isValidCommonKey(const std::string &key) const;

    /**
     * @brief Requests input from the user
     *
     * Invokes the app to get user input, the app will then notify the SDK of the user response.
     *
     * @param handle The RaceHandle to use for the onUserInputReceived callback
     * @param pluginId Plugin ID of the plugin requesting user input (or "Common" for common user
     *      input)
     * @param key User input key
     * @param prompt User input prompt
     * @param cache If true, the response will be cached
     * @return SdkResponse object that contains whether the post was successful
     */
    SdkResponse requestUserInput(RaceHandle handle, const std::string &pluginId,
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
    SdkResponse displayInfoToUser(RaceHandle handle, const std::string &data,
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
    SdkResponse displayBootstrapInfoToUser(RaceHandle handle, const std::string &data,
                                           RaceEnums::UserDisplayType displayType,
                                           RaceEnums::BootstrapActionType actionType) override;
};

#endif
