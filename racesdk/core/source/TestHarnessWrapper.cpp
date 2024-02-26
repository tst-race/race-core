
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

#include "TestHarnessWrapper.h"

#include "OpenTracingHelpers.h"
#include "PluginNMTestHarness.h"
#include "RaceSdk.h"
#include "helper.h"

TestHarnessWrapper::TestHarnessWrapper(RaceSdk &sdk) : NMWrapper(sdk, "test-harness") {
    mTestHarness = std::make_shared<PluginNMTestHarness>(this);
    mPlugin = std::static_pointer_cast<IRacePluginNM>(mTestHarness);
    mId = "PluginNMTwoSixTestHarness";
    mDescription = PluginNMTestHarness::getDescription();
    createQueue("open", -3);
    createQueue("rpc", -2);
}

std::tuple<bool, double> TestHarnessWrapper::processNMBypassMsg(RaceHandle handle,
                                                                const ClrMsg &msg,
                                                                const std::string &route,
                                                                int32_t timeout) {
    TRACE_METHOD(getId(), handle, route);
    std::string postId = std::to_string(nextPostId++);
    std::string message = msg.getMsg();
    std::size_t length = message.size();
    if (length > raceSdk.getRaceConfig().msgLogLength) {
        message.resize(raceSdk.getRaceConfig().msgLogLength);
        message += " [MESSAGE CLIPPED]";
    }
    helper::logInfo("Sending network-manager-bypass Message:");
    helper::logInfo("    Route: " + route);
    helper::logDebug("    Message: " + message);
    helper::logInfo("    length = " + std::to_string(length) +
                    ", hash = " + helper::getMessageSignature(msg));
    helper::logInfo("    from: " + msg.getFrom() + ", to: " + msg.getTo());

    helper::logDebug("NMWrapper::processNMBypassMsg: decoding traceId");
    auto ctx = spanContextFromClrMsg(msg);

    std::shared_ptr<opentracing::Span> span =
        mTracer->StartSpan("processNMBypassMsg", {opentracing::ChildOf(ctx.get())});

    span->SetTag("source", "racesdk");
    span->SetTag("file", __FILE__);
    span->SetTag("pluginId", mId);
    span->SetTag("networkManagerBypassRoute", route);
    span->SetTag("messageSize", std::to_string(length));
    span->SetTag("messageHash", helper::getMessageSignature(msg));
    span->SetTag("messageFrom", msg.getFrom());
    span->SetTag("messageTo", msg.getTo());

    ClrMsg newMsg = msg;
    newMsg.setTraceId(traceIdFromContext(span->context()));
    newMsg.setSpanId(spanIdFromContext(span->context()));

    //                 message  from                   to                   ids
    uint32_t msgSize = length + msg.getFrom().size() + msg.getTo().size() + 16;

    helper::logDebug("Posting PluginNMTestHarness::processNMBypassMsg(), postId: " + postId +
                     " traceId: " + helper::convertToHexString(newMsg.getTraceId()) +
                     " spanId: " + helper::convertToHexString(newMsg.getSpanId()));
    try {
        auto [success, queueSize, future] = mThreadHandler.post("receive", msgSize, timeout, [=] {
            helper::logDebug(
                "Calling PluginNMTestHarness::processNMBypassMsg(), postId: " + postId +
                " traceId: " + helper::convertToHexString(newMsg.getTraceId()) +
                " spanId: " + helper::convertToHexString(newMsg.getSpanId()));
            PluginResponse response;
            try {
                response = mTestHarness->processNMBypassMsg(handle, route, newMsg);
            } catch (std::exception &e) {
                helper::logError("PluginNMTestHarness::processNMBypassMsg() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("PluginNMTestHarness::processNMBypassMsg() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug(
                "PluginNMTestHarness::processNMBypassMsg() returned, postId: " + postId +
                " traceId: " + helper::convertToHexString(newMsg.getTraceId()) +
                " spanId: " + helper::convertToHexString(newMsg.getSpanId()));
            span->Finish();

            if (response != PLUGIN_OK) {
                helper::logError("PluginNMTestHarness::processNMBypassMsg() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            if (response == PLUGIN_FATAL) {
                // NM can't continue. We have no way to cleanly handle it right now, so shutdown
                // what we can and bail
                raceSdk.shutdownCommsAndCrash();
            }

            return std::make_optional(true);
        });

        (void)future;
        double queueUtilization = (static_cast<double>(queueSize) / mThreadHandler.max_queue_size);
        return {success == Handler::PostStatus::OK, queueUtilization};
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return {false, 0};
}

std::tuple<bool, double> TestHarnessWrapper::openRecvConnection(RaceHandle handle,
                                                                const std::string &persona,
                                                                const std::string &route,
                                                                int32_t timeout) {
    TRACE_METHOD(handle, persona, route);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting PluginNMTestHarness::openRecvConnection(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("open", 0, timeout, [=] {
            helper::logDebug("Calling PluginNMTestHarness::openRecvConnection(), postId: " +
                             postId);
            PluginResponse response;
            try {
                response = mTestHarness->openRecvConnection(handle, persona, route);
            } catch (std::exception &e) {
                helper::logError("PluginNMTestHarness::openRecvConnection() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("PluginNMTestHarness::openRecvConnection() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("PluginNMTestHarness::openRecvConnection() returned, postId: " +
                             postId);

            if (response != PLUGIN_OK) {
                helper::logError("PluginNMTestHarness::openRecvConnection() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        (void)future;
        double queueUtilization = (static_cast<double>(queueSize) / mThreadHandler.max_queue_size);
        return {success == Handler::PostStatus::OK, queueUtilization};
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return {false, 0};
}

std::tuple<bool, double> TestHarnessWrapper::rpcDeactivateChannel(const std::string &channelGid,
                                                                  int32_t timeout) {
    TRACE_METHOD(channelGid);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting PluginNMTestHarness::rpcDeactivateChannel(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("rpc", 0, timeout, [=] {
            helper::logDebug("Calling PluginNMTestHarness::rpcDeactivateChannel(), postId: " +
                             postId);
            PluginResponse response;
            try {
                response = mTestHarness->rpcDeactivateChannel(channelGid);
            } catch (std::exception &e) {
                helper::logError("PluginNMTestHarness::rpcDeactivateChannel() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("PluginNMTestHarness::rpcDeactivateChannel() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("PluginNMTestHarness::rpcDeactivateChannel() returned, postId: " +
                             postId);

            if (response != PLUGIN_OK) {
                helper::logError("PluginNMTestHarness::rpcDeactivateChannel() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        (void)future;
        double queueUtilization = (static_cast<double>(queueSize) / mThreadHandler.max_queue_size);
        return {success == Handler::PostStatus::OK, queueUtilization};
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return {false, 0};
}

std::tuple<bool, double> TestHarnessWrapper::rpcDestroyLink(const std::string &linkId,
                                                            int32_t timeout) {
    TRACE_METHOD(linkId);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting PluginNMTestHarness::rpcDestroyLink(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("rpc", 0, timeout, [=] {
            helper::logDebug("Calling PluginNMTestHarness::rpcDestroyLink(), postId: " + postId);
            PluginResponse response;
            try {
                response = mTestHarness->rpcDestroyLink(linkId);
            } catch (std::exception &e) {
                helper::logError("PluginNMTestHarness::rpcDestroyLink() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("PluginNMTestHarness::rpcDestroyLink() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("PluginNMTestHarness::rpcDestroyLink() returned, postId: " + postId);

            if (response != PLUGIN_OK) {
                helper::logError("PluginNMTestHarness::rpcDestroyLink() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        (void)future;
        double queueUtilization = (static_cast<double>(queueSize) / mThreadHandler.max_queue_size);
        return {success == Handler::PostStatus::OK, queueUtilization};
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return {false, 0};
}

std::tuple<bool, double> TestHarnessWrapper::rpcCloseConnection(const std::string &connectionId,
                                                                int32_t timeout) {
    TRACE_METHOD(connectionId);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting PluginNMTestHarness::rpcCloseConnection(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("rpc", 0, timeout, [=] {
            helper::logDebug("Calling PluginNMTestHarness::rpcCloseConnection(), postId: " +
                             postId);
            PluginResponse response;
            try {
                response = mTestHarness->rpcCloseConnection(connectionId);
            } catch (std::exception &e) {
                helper::logError("PluginNMTestHarness::rpcCloseConnection() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("PluginNMTestHarness::rpcCloseConnection() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("PluginNMTestHarness::rpcCloseConnection() returned, postId: " +
                             postId);

            if (response != PLUGIN_OK) {
                helper::logError("PluginNMTestHarness::rpcCloseConnection() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        (void)future;
        double queueUtilization = (static_cast<double>(queueSize) / mThreadHandler.max_queue_size);
        return {success == Handler::PostStatus::OK, queueUtilization};
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return {false, 0};
}
