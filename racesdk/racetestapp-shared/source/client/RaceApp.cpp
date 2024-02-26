
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

#include "racetestapp/RaceApp.h"

#include <OpenTracingHelpers.h>

#include <chrono>
#include <optional>
#include <set>
#include <stdexcept>  // std::out_of_range
#include <thread>

#include "PluginStatus.h"
#include "racetestapp/ReceivedMessage.h"
#include "racetestapp/UserInputResponseCache.h"
#include "racetestapp/UserInputResponseParser.h"
#include "racetestapp/raceTestAppHelpers.h"

// cppcheck-suppress useInitializationList
RaceApp::RaceApp(IRaceTestAppOutput &_appOutput, IRaceSdkApp &_raceSdk,
                 std::shared_ptr<opentracing::Tracer> _tracer) :
    appOutput(_appOutput),
    raceSdk(_raceSdk),
    tracer(_tracer),
    ready(false),
    responseCache(std::make_unique<UserInputResponseCache>(_raceSdk)),
    responseParser(
        std::make_unique<UserInputResponseParser>(_raceSdk.getAppConfig().userResponsesFilePath)) {
    responseCache->readCache();
}

void RaceApp::addMessageToUI(const ClrMsg &) {
    // Do Nothing, this function is just required for passing send messages
    // generated by RaceTestApp to the UI
    // Android should override this implementation
}

void RaceApp::handleReceivedMessage(ClrMsg msg) {
    auto ctx = spanContextFromClrMsg(msg);
    std::shared_ptr<opentracing::Span> span =
        tracer->StartSpan("receiveMessage", {opentracing::FollowsFrom(ctx.get())});

    rtah::outputMessage(appOutput, ReceivedMessage(msg));

    span->SetTag("source", "racetestapp");
    span->SetTag("file", __FILE__);
    span->SetTag("messageSize", std::to_string(msg.getMsg().size()));
    span->SetTag("messageHash", rtah::getMessageSignature(msg));
    span->SetTag("messageFrom", msg.getFrom());
    span->SetTag("messageTo", msg.getTo());
    span->SetTag("messageTestId", rtah::testIdFromClrMsg(msg));
}

void RaceApp::onMessageStatusChanged(RaceHandle handle, MessageStatus status) {
    appOutput.writeOutput("RaceApp::onMessageStatusChanged: called with handle: " +
                          std::to_string(handle) + " status: " + messageStatusToString(status));
}

void RaceApp::onSdkStatusChanged(const nlohmann::json &sdkStatus) {
    rtah::logInfo("onSdkStatusChanged: called");
    rtah::logDebug("sdkStatus: " + sdkStatus.dump());
    currentSdkStatus = sdkStatus;

    bool newStatus = sdkStatus["network-manager-status"] == "PLUGIN_READY";
    // Only print a message if ready changed
    if (newStatus and not ready) {
        appOutput.writeOutput("App is ready to send.");
    } else if (not newStatus and ready) {
        appOutput.writeOutput("App is unready.");
    }
    ready = newStatus;

    rtah::logInfo("onSdkStatusChanged: return");
}

nlohmann::json RaceApp::getSdkStatus() {
    return currentSdkStatus;
}

SdkResponse RaceApp::requestUserInput(RaceHandle handle, const std::string &pluginId,
                                      const std::string &key, const std::string &prompt,
                                      bool cache) {
    rtah::logDebug("Looking up user response, pluginId: " + pluginId + " key: " + key +
                   " prompt: " + prompt);

    bool cachedResponse = false;
    bool answered = false;
    std::string response;

    if (cache) {
        std::tie(answered, response) = getCachedResponse(pluginId, key);
        cachedResponse = answered;
    }

    if (not answered) {
        std::tie(answered, response) = getAutoResponse(pluginId, key);
    }

    // If we should cache this and didn't get it from the cache initially,
    // cache the response (but only if we actually got an answer)
    if (cache && not cachedResponse && answered) {
        setCachedResponse(pluginId, key, response);
    }

    rtah::logDebug("UserInput responding, answered: " + std::to_string(answered) +
                   " response: " + response);
    return raceSdk.onUserInputReceived(handle, answered, response);
}

std::tuple<bool, std::string> RaceApp::getCachedResponse(const std::string &pluginId,
                                                         const std::string &key) {
    rtah::logDebug("Looking up cached user response, pluginId: " + pluginId + " key: " + key);

    try {
        std::string response = responseCache->getResponse(pluginId, key);
        rtah::logDebug("Using cached user response: " + response);
        return {true, response};
    } catch (std::out_of_range &) {
        rtah::logDebug("No cached user response found");
    }
    return {false, ""};
}

std::tuple<bool, std::string> RaceApp::getAutoResponse(const std::string &pluginId,
                                                       const std::string &key) {
    rtah::logDebug("Looking up automated user response, pluginId: " + pluginId + " key: " + key);

    UserInputResponseParser::UserResponse response;

    try {
        response = responseParser->getResponse(pluginId, key);
        rtah::logDebug("Using auto user response: " + response.response);
    } catch (std::exception &error) {
        rtah::logWarning(std::string(error.what()));
    }

    if (response.delay_ms > 0) {
        rtah::logDebug("Delaying " + std::to_string(response.delay_ms) +
                       "ms before responding with user input");
        std::this_thread::sleep_for(std::chrono::milliseconds(response.delay_ms));
    }

    return {response.answered, response.response};
}

bool RaceApp::setCachedResponse(const std::string &pluginId, const std::string &key,
                                const std::string &response) {
    rtah::logDebug("Caching user input response, pluginId: " + pluginId + " key: " + key +
                   " response: " + response);
    if (not responseCache->cacheResponse(pluginId, key, response)) {
        rtah::logError("Unable to cache user input response");
        return false;
    }
    return true;
}

SdkResponse RaceApp::displayInfoToUser(RaceHandle handle, const std::string &data,
                                       RaceEnums::UserDisplayType /*displayType*/) {
    rtah::logDebug("displayInfoToUser: called with data:  " + data);
    raceSdk.onUserAcknowledgementReceived(handle);
    return SDK_OK;
}

SdkResponse RaceApp::displayBootstrapInfoToUser(RaceHandle handle, const std::string &data,
                                                RaceEnums::UserDisplayType /*displayType*/,
                                                RaceEnums::BootstrapActionType /*actionType*/) {
    rtah::logDebug("displayBootstrapInfoToUser: called with data:  " + data);
    raceSdk.onUserAcknowledgementReceived(handle);
    return SDK_OK;
}