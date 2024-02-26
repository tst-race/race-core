
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

#include "AppWrapper.h"

#include <jaegertracing/Tracer.h>

#include "OpenTracingHelpers.h"
#include "helper.h"

static std::set<std::string> VALID_KEYS = {"hostname", "env"};

void AppWrapper::startHandler() {
    helper::logDebug("Start Client handler");
    mThreadHandler.start();
    helper::logDebug("Client handler started");
}

void AppWrapper::stopHandler() {
    helper::logDebug("Stop Client handler");
    mThreadHandler.stop();
    helper::logDebug("Client handler stopped");
}

void AppWrapper::handleReceivedMessage(ClrMsg msg) {
    helper::logDebug("AppWrapper::handleReceivedMessage: decoding traceId");

    auto ctx = spanContextFromClrMsg(msg);

    std::shared_ptr<opentracing::Span> span =
        mTracer->StartSpan("handleReceivedMessage", {opentracing::ChildOf(ctx.get())});

    span->SetTag("source", "racesdk");
    span->SetTag("file", __FILE__);
    span->SetTag("messageSize", std::to_string(msg.getMsg().size()));
    span->SetTag("messageHash", helper::getMessageSignature(msg));
    span->SetTag("messageFrom", msg.getFrom());
    span->SetTag("messageTo", msg.getTo());

    std::string message = msg.getMsg();
    std::size_t length = message.size();
    if (length > 256) {
        message.resize(256 - 3);
        message += "...";
    }
    helper::logInfo("Received Message:");
    helper::logDebug("    Message: " + message);
    helper::logInfo("    length = " + std::to_string(length) +
                    ", hash = " + helper::getMessageSignature(msg));
    helper::logInfo("    from: " + msg.getFrom() + ", to: " + msg.getTo());

    std::string postId = std::to_string(nextPostId++);

    ClrMsg newMsg = msg;
    newMsg.setTraceId(traceIdFromContext(span->context()));
    newMsg.setSpanId(spanIdFromContext(span->context()));

    helper::logDebug("Posting IRaceApp::handleReceivedMessage(), postId: " + postId +
                     " traceId: " + helper::convertToHexString(newMsg.getTraceId()) +
                     " spanId: " + helper::convertToHexString(newMsg.getSpanId()));

    mThreadHandler.post("", 0, 0, [=] {
        helper::logDebug("Calling IRaceApp::handleReceivedMessage(), postId: " + postId +
                         " traceId: " + helper::convertToHexString(newMsg.getTraceId()) +
                         " spanId: " + helper::convertToHexString(newMsg.getSpanId()));
        mClient->handleReceivedMessage(newMsg);
        helper::logDebug("IRaceApp::handleReceivedMessage() returned, postId: " + postId +
                         " traceId: " + helper::convertToHexString(newMsg.getTraceId()) +
                         " spanId: " + helper::convertToHexString(newMsg.getSpanId()));
        span->Finish();

        return std::make_optional(true);
    });
}

void AppWrapper::onMessageStatusChanged(RaceHandle handle, MessageStatus status) {
    const std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting IRaceApp::onMessageStatusChanged(), postId: " + postId + " handle: " +
                     std::to_string(handle) + " status: " + messageStatusToString(status));

    mThreadHandler.post("", 0, 0, [=] {
        helper::logDebug("Calling IRaceApp::onMessageStatusChanged(), postId: " + postId +
                         " handle: " + std::to_string(handle) +
                         " status: " + messageStatusToString(status));
        mClient->onMessageStatusChanged(handle, status);
        helper::logDebug("IRaceApp::onMessageStatusChanged() returned, postId: " + postId +
                         " handle: " + std::to_string(handle) +
                         " status: " + messageStatusToString(status));
        return std::make_optional(true);
    });
}

nlohmann::json AppWrapper::getSdkStatus() {
    return {{}};
}

void AppWrapper::onSdkStatusChanged(const nlohmann::json &sdkStatus) {
    helper::logDebug("AppWrapper::onSdkStatusChanged called");
    std::string postId = std::to_string(nextPostId++);
    mThreadHandler.post("", 0, 0, [=] {
        helper::logDebug("Calling IRaceApp::onSdkStatusChanged(), postId: " + postId);
        mClient->onSdkStatusChanged(sdkStatus);
        helper::logDebug("IRaceApp::onSdkStatusChanged() returned, postId: " + postId);
        return std::make_optional(true);
    });
    helper::logDebug("AppWrapper::onSdkStatusChanged returned");
}

void AppWrapper::waitForCallbacks() {
    helper::logDebug("AppWrapper::waitForCallbacks called");
    mThreadHandler.create_queue("wait queue", std::numeric_limits<int>::min());
    auto [success, queueSize, future] =
        mThreadHandler.post("wait queue", 0, -1, [=] { return std::make_optional(true); });
    (void)success;
    (void)queueSize;
    future.wait();
    mThreadHandler.remove_queue("wait queue");
    helper::logDebug("AppWrapper::waitForCallbacks returned");
}

bool AppWrapper::isValidCommonKey(const std::string &key) const {
    helper::logDebug("AppWrapper::isValidCommonKey called");
    return VALID_KEYS.find(key) != VALID_KEYS.end();
}

SdkResponse AppWrapper::requestUserInput(RaceHandle handle, const std::string &pluginId,
                                         const std::string &key, const std::string &prompt,
                                         bool cache) {
    helper::logDebug("AppWrapper::requestUserInput called");
    try {
        std::string postId = std::to_string(nextPostId++);
        helper::logInfo("Posting IRaceApp::requestUserInput(), postId: " + postId);
        auto [success, _, __] = mThreadHandler.post("", 0, -1, [=] {
            helper::logDebug("Calling IRaceApp::requestUserInput()");
            mClient->requestUserInput(handle, pluginId, key, prompt, cache);
            return std::make_optional(true);
        });

        // Since we aren't using posted work sizes, the only reason this would fail
        // is because of an invalid state, rather than a full queue
        SdkStatus sdkStatus = success == Handler::PostStatus::OK ? SDK_OK : SDK_SHUTTING_DOWN;
        return {sdkStatus, 0, handle};
    } catch (std::out_of_range &error) {
        helper::logWarning("Default queue does not exist. This should never happen. what:" +
                           std::string(error.what()));
    }
    return {SDK_INVALID_ARGUMENT, 0, handle};
}

SdkResponse AppWrapper::displayInfoToUser(RaceHandle handle, const std::string &data,
                                          RaceEnums::UserDisplayType displayType) {
    helper::logDebug("AppWrapper::displayInfoToUser called");
    try {
        std::string postId = std::to_string(nextPostId++);
        helper::logInfo("Posting IRaceApp::displayInfoToUser(), postId: " + postId);
        auto [success, queueSize, future] = mThreadHandler.post("", 0, -1, [=] {
            helper::logDebug("Calling IRaceApp::displayInfoToUser()");
            mClient->displayInfoToUser(handle, data, displayType);
            return std::make_optional(true);
        });
        (void)queueSize;
        (void)future;

        // Since we aren't using posted work sizes, the only reason this would fail
        // is because of an invalid state, rather than a full queue
        SdkStatus sdkStatus = success == Handler::PostStatus::OK ? SDK_OK : SDK_SHUTTING_DOWN;
        return {sdkStatus, 0, handle};
    } catch (std::out_of_range &error) {
        helper::logWarning("Default queue does not exist. This should never happen. what:" +
                           std::string(error.what()));
    }
    return SDK_INVALID_ARGUMENT;
}

SdkResponse AppWrapper::displayBootstrapInfoToUser(RaceHandle handle, const std::string &data,
                                                   RaceEnums::UserDisplayType displayType,
                                                   RaceEnums::BootstrapActionType actionType) {
    helper::logDebug("AppWrapper::displayBootstrapInfoToUser called");
    try {
        std::string postId = std::to_string(nextPostId++);
        helper::logInfo("Posting IRaceApp::displayBootstrapInfoToUser(), postId: " + postId);
        auto [success, queueSize, future] = mThreadHandler.post("", 0, -1, [=] {
            helper::logDebug("Calling IRaceApp::displayBootstrapInfoToUser()");
            mClient->displayBootstrapInfoToUser(handle, data, displayType, actionType);
            return std::make_optional(true);
        });
        (void)queueSize;
        (void)future;

        // Since we aren't using posted work sizes, the only reason this would fail
        // is because of an invalid state, rather than a full queue
        SdkStatus sdkStatus = success == Handler::PostStatus::OK ? SDK_OK : SDK_SHUTTING_DOWN;
        return {sdkStatus, 0, handle};
    } catch (std::out_of_range &error) {
        helper::logWarning("Default queue does not exist. This should never happen. what:" +
                           std::string(error.what()));
    }
    return SDK_INVALID_ARGUMENT;
}
