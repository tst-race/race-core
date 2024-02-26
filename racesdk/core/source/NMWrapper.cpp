
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

#include "NMWrapper.h"

#include <jaegertracing/Tracer.h>

#include <future>  // std::promise, std::future

#include "OpenTracingHelpers.h"
#include "filesystem.h"
#include "helper.h"

NMWrapper::NMWrapper(std::shared_ptr<IRacePluginNM> plugin, std::string id, std::string description,
                     RaceSdk &sdk, const std::string &configPath) :
    raceSdk(sdk),
    mTracer(sdk.getTracer()),
    mThreadHandler(id + "-thread", sdk.getRaceConfig().wrapperQueueMaxSize,
                   sdk.getRaceConfig().wrapperTotalMaxSize),
    mPlugin(std::move(plugin)),
    mId(std::move(id)),
    mDescription(std::move(description)),
    mConfigPath(configPath) {
    construct();
}

NMWrapper::NMWrapper(RaceSdk &sdk, const std::string &name) :
    raceSdk(sdk),
    mTracer(sdk.getTracer()),
    mThreadHandler(name + "-thread", sdk.getRaceConfig().wrapperQueueMaxSize,
                   sdk.getRaceConfig().wrapperTotalMaxSize) {
    construct();
}

void NMWrapper::construct() {
    TRACE_METHOD(getId());
    createQueue("receive", -2);
    createQueue("callback", -1);
    createQueue("wait queue", std::numeric_limits<int>::min());
}

NMWrapper::~NMWrapper() {
    TRACE_METHOD(getId());
}

void NMWrapper::startHandler() {
    TRACE_METHOD(getId());
    mThreadHandler.start();
}

void NMWrapper::stopHandler() {
    TRACE_METHOD(getId());
    mThreadHandler.stop();
}

void NMWrapper::waitForCallbacks() {
    auto [success, queueSize, future] =
        mThreadHandler.post("wait queue", 0, -1, [=] { return std::make_optional(true); });
    (void)success;
    (void)queueSize;
    future.wait();
}

void NMWrapper::createQueue(const std::string &name, int priority) {
    TRACE_METHOD(getId(), name, priority);
    mThreadHandler.create_queue(name, priority);
}

void NMWrapper::removeQueue(const std::string &name) {
    TRACE_METHOD(getId(), name);
    mThreadHandler.remove_queue(name);
}

bool NMWrapper::init(const PluginConfig &pluginConfig) {
    TRACE_METHOD(getId());

    PluginResponse response;
    try {
        response = mPlugin->init(pluginConfig);
        helper::logDebug("IRacePluginNM::init() returned " +
                         helper::pluginResponseToString(response));
    } catch (std::exception &e) {
        helper::logError("IRacePluginNM::init() threw exception: " + std::string(e.what()));
        response = PLUGIN_FATAL;
    } catch (...) {
        helper::logError("IRacePluginNM::init() threw an exception");
        response = PLUGIN_FATAL;
    }

    if (response != PLUGIN_OK) {
        helper::logError("IRacePluginNM::init() returned status: " +
                         helper::pluginResponseToString(response));

        // tell the caller to close the app
        return false;
    }

    return true;
}

std::tuple<bool, double> NMWrapper::shutdown() {
    // By default, wait with a 30 second timeout for shutdown to complete. Based on internal
    // discussions this _should_ be ample time for the plugins to shutdown. However, this value
    // may be adjusted after testing with actual plugins.
    // Note that if the work queue grows large when this is called (e.g. shutdown gets called in the
    // middle of a large scale stress test) the timeout may get hit before shutdown even gets
    // called. Leaving this as-is for now as this seems to be a corner case, and should be easy to
    // diagnose from the logs.
    const std::int32_t defaultSecondsToWaitForShutdownToComplete = 30;
    return shutdown(defaultSecondsToWaitForShutdownToComplete);
}

std::tuple<bool, double> NMWrapper::shutdown(std::int32_t timeoutInSeconds) {
    TRACE_METHOD(getId(), timeoutInSeconds);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting IRacePluginNM::shutdown(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, -1, [=] {
            helper::logDebug("Calling IRacePluginNM::shutdown(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->shutdown();
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::shutdown() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::shutdown() threw an exception");
                response = PLUGIN_FATAL;
            }

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::shutdown() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
                return std::make_optional(false);
            }

            helper::logDebug("IRacePluginNM::shutdown() returned, postId: " + postId);
            return std::make_optional(true);
        });

        auto status = future.wait_for(std::chrono::seconds(timeoutInSeconds));
        if (status != std::future_status::ready) {
            helper::logError("IRacePluginNM::shutdown() timed out, took longer than " +
                             std::to_string(timeoutInSeconds) + " seconds");
        }

        double queueUtilization = (static_cast<double>(queueSize) / mThreadHandler.max_queue_size);
        return {success == Handler::PostStatus::OK, queueUtilization};
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return {false, 0};
}

std::tuple<bool, double> NMWrapper::processClrMsg(RaceHandle handle, const ClrMsg &msg,
                                                  int32_t timeout) {
    TRACE_METHOD(getId(), handle);
    std::string postId = std::to_string(nextPostId++);
    std::string message = msg.getMsg();
    std::size_t length = message.size();
    if (length > raceSdk.getRaceConfig().msgLogLength) {
        message.resize(raceSdk.getRaceConfig().msgLogLength);
        message += " [MESSAGE CLIPPED]";
    }
    helper::logInfo("Sending Message:");
    helper::logDebug("    Message: " + message);
    helper::logInfo("    length = " + std::to_string(length) +
                    ", hash = " + helper::getMessageSignature(msg));
    helper::logInfo("    from: " + msg.getFrom() + ", to: " + msg.getTo());

    helper::logDebug("NMWrapper::processClrMsg: decoding traceId");
    auto ctx = spanContextFromClrMsg(msg);

    std::shared_ptr<opentracing::Span> span =
        mTracer->StartSpan("processClrMsg", {opentracing::ChildOf(ctx.get())});

    span->SetTag("source", "racesdk");
    span->SetTag("file", __FILE__);
    span->SetTag("pluginId", mId);
    span->SetTag("messageSize", std::to_string(length));
    span->SetTag("messageHash", helper::getMessageSignature(msg));
    span->SetTag("messageFrom", msg.getFrom());
    span->SetTag("messageTo", msg.getTo());

    ClrMsg newMsg = msg;
    newMsg.setTraceId(traceIdFromContext(span->context()));
    newMsg.setSpanId(spanIdFromContext(span->context()));

    //                 message  from                   to                   ids
    uint32_t msgSize = length + msg.getFrom().size() + msg.getTo().size() + 16;

    helper::logDebug("Posting IRacePluginNM::processClrMsg(), postId: " + postId +
                     " traceId: " + helper::convertToHexString(newMsg.getTraceId()) +
                     " spanId: " + helper::convertToHexString(newMsg.getSpanId()));
    try {
        auto [success, queueSize, future] = mThreadHandler.post("receive", msgSize, timeout, [=] {
            helper::logDebug("Calling IRacePluginNM::processClrMsg(), postId: " + postId +
                             " traceId: " + helper::convertToHexString(newMsg.getTraceId()) +
                             " spanId: " + helper::convertToHexString(newMsg.getSpanId()));
            PluginResponse response;
            try {
                response = mPlugin->processClrMsg(handle, newMsg);
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::processClrMsg() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::processClrMsg() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::processClrMsg() returned, postId: " + postId +
                             " traceId: " + helper::convertToHexString(newMsg.getTraceId()) +
                             " spanId: " + helper::convertToHexString(newMsg.getSpanId()));
            span->Finish();

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::processClrMsg() returned status: " +
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

std::tuple<bool, double> NMWrapper::processEncPkg(RaceHandle handle, const EncPkg &ePkg,
                                                  const std::vector<ConnectionID> &connIDs,
                                                  int32_t timeout) {
    TRACE_METHOD(getId(), handle);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("NMWrapper::processEncPkg: decoding traceId");
    auto ctx = spanContextFromEncryptedPackage(ePkg);

    std::shared_ptr<opentracing::Span> span =
        mTracer->StartSpan("processEncPkg", {opentracing::ChildOf(ctx.get())});

    span->SetTag("source", "racesdk");
    span->SetTag("file", __FILE__);
    span->SetTag("pluginId", mId);

    std::string connections;
    if (!connIDs.empty()) {
        // Convert all but the last element to avoid a trailing ","
        for (auto it = connIDs.begin(); it != connIDs.end() - 1; it++) {
            connections += *it + ", ";
        }

        // Now add the last element with no delimiter
        connections += connIDs.back();
    }
    span->SetTag("connectionIds", connections);

    EncPkg newPkg = ePkg;
    newPkg.setTraceId(traceIdFromContext(span->context()));
    newPkg.setSpanId(spanIdFromContext(span->context()));

    helper::logDebug("Posting IRacePluginNM::processEncPkg(), postId: " + postId +
                     " traceId: " + helper::convertToHexString(newPkg.getTraceId()) +
                     " spanId: " + helper::convertToHexString(newPkg.getSpanId()) +
                     " parent spanId: " + helper::convertToHexString(ePkg.getSpanId()));

    uint32_t pkgSize = newPkg.getSize();

    try {
        auto [success, queueSize, future] = mThreadHandler.post("receive", pkgSize, timeout, [=] {
            helper::logDebug("Calling IRacePluginNM::processEncPkg(), postId: " + postId +
                             " traceId: " + helper::convertToHexString(newPkg.getTraceId()) +
                             " spanId: " + helper::convertToHexString(newPkg.getSpanId()));

            PluginResponse response;
            try {
                response = mPlugin->processEncPkg(handle, newPkg, connIDs);
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::processEncPkg() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::processEncPkg() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::processEncPkg() returned, postId: " + postId +
                             " traceId: " + helper::convertToHexString(newPkg.getTraceId()) +
                             " spanId: " + helper::convertToHexString(newPkg.getSpanId()));
            span->Finish();

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::processEncPkg() returned status: " +
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

std::tuple<bool, double> NMWrapper::prepareToBootstrap(RaceHandle handle, LinkID linkId,
                                                       std::string configPath,
                                                       DeviceInfo deviceInfo, int32_t timeout) {
    TRACE_METHOD(getId(), handle, linkId, configPath);
    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Posting IRacePluginNM::prepareToBootstrap(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("receive", 0, timeout, [=] {
            helper::logDebug("Calling IRacePluginNM::prepareToBootstrap(), postId: " + postId);

            PluginResponse response;
            try {
                response = mPlugin->prepareToBootstrap(handle, linkId, configPath, deviceInfo);
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::prepareToBootstrap() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::prepareToBootstrap() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::prepareToBootstrap() returned, postId: " + postId);

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::prepareToBootstrap() returned status: " +
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

std::tuple<bool, double> NMWrapper::onBootstrapPkgReceived(std::string persona, RawData pkg,
                                                           int32_t timeout) {
    TRACE_METHOD(getId(), persona);
    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Posting IRacePluginNM::onBootstrapPkgReceived(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("callback", 0, timeout, [=] {
            helper::logDebug("Calling IRacePluginNM::onBootstrapPkgReceived(), postId: " + postId);

            PluginResponse response;
            try {
                response = mPlugin->onBootstrapPkgReceived(persona, pkg);
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::onBootstrapPkgReceived() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::onBootstrapPkgReceived() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::onBootstrapPkgReceived() returned, postId: " + postId);

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::onBootstrapPkgReceived() returned status: " +
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

bool NMWrapper::onBootstrapFinished(RaceHandle bootstrapHandle, BootstrapState state) {
    TRACE_METHOD(getId());
    std::string postId = std::to_string(nextPostId++);
    helper::logDebug(logPrefix + "Posting postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("callback", 0, 60, [=] {
            helper::logDebug("Calling IRacePluginNM::onBootstrapFinished(), postId: " + postId);

            PluginResponse response;
            try {
                response = mPlugin->onBootstrapFinished(bootstrapHandle, state);
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::onBootstrapFinished() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::onBootstrapFinished() unhandled exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::onBootstrapFinished() returned, postId: " + postId);

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::onBootstrapFinished() returned status: " +
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
        return success == Handler::PostStatus::OK;
    } catch (std::exception &ex) {
        helper::logError(logPrefix + " exception: " + std::string(ex.what()));
    }

    return false;
}

std::tuple<bool, double> NMWrapper::onPackageStatusChanged(RaceHandle handle, PackageStatus status,
                                                           int32_t timeout) {
    TRACE_METHOD(getId(), handle, status);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting IRacePluginNM::onPackageStatusChanged(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("callback", 0, timeout, [=] {
            helper::logDebug("Calling IRacePluginNM::onPackageStatusChanged(), postId: " + postId);

            PluginResponse response;
            try {
                response = mPlugin->onPackageStatusChanged(handle, status);
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::onPackageStatusChanged() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::onPackageStatusChanged() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::onPackageStatusChanged() returned, postId: " + postId);

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::onPackageStatusChanged() returned status: " +
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

std::tuple<bool, double> NMWrapper::onConnectionStatusChanged(
    RaceHandle handle, const ConnectionID &connId, ConnectionStatus status, const LinkID &linkId,
    const LinkProperties &properties, int32_t timeout) {
    TRACE_METHOD(getId(), handle, connId, status, linkId);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting IRacePluginNM::onConnectionStatusChanged(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("callback", 0, timeout, [=] {
            helper::logDebug("Calling IRacePluginNM::onConnectionStatusChanged(), postId: " +
                             postId);
            PluginResponse response;
            try {
                response =
                    mPlugin->onConnectionStatusChanged(handle, connId, status, linkId, properties);
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::onConnectionStatusChanged() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::onConnectionStatusChanged() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::onConnectionStatusChanged() returned, postId: " +
                             postId);

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::onConnectionStatusChanged() returned status: " +
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

std::tuple<bool, double> NMWrapper::onLinkStatusChanged(RaceHandle handle, LinkID linkId,
                                                        LinkStatus status,
                                                        LinkProperties properties,
                                                        int32_t timeout) {
    TRACE_METHOD(getId(), handle, linkId, status);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting IRacePluginNM::onLinkStatusChanged(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("callback", 0, timeout, [=] {
            helper::logDebug("Calling IRacePluginNM::onLinkStatusChanged(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->onLinkStatusChanged(handle, linkId, status, properties);
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::onLinkStatusChanged() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::onLinkStatusChanged() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::onLinkStatusChanged() returned, postId: " + postId);

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::onLinkStatusChanged() returned status: " +
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

std::tuple<bool, double> NMWrapper::onChannelStatusChanged(RaceHandle handle,
                                                           const std::string &channelGid,
                                                           ChannelStatus status,
                                                           const ChannelProperties &properties,
                                                           int32_t timeout) {
    TRACE_METHOD(getId(), handle, channelGid, status);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting IRacePluginNM::onChannelStatusChanged(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("callback", 0, timeout, [=] {
            helper::logDebug("Calling IRacePluginNM::onChannelStatusChanged(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->onChannelStatusChanged(handle, channelGid, status, properties);
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::onChannelStatusChanged() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::onChannelStatusChanged() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::onChannelStatusChanged() returned, postId: " + postId);

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::onChannelStatusChanged() returned status: " +
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

std::tuple<bool, double> NMWrapper::onLinkPropertiesChanged(LinkID linkId,
                                                            const LinkProperties &linkProperties,
                                                            int32_t timeout) {
    TRACE_METHOD(getId(), linkId);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting IRacePluginNM::onLinkPropertiesChanged(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("callback", 0, timeout, [=] {
            helper::logDebug("Calling IRacePluginNM::onLinkPropertiesChanged(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->onLinkPropertiesChanged(linkId, linkProperties);
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::onLinkPropertiesChanged() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::onLinkPropertiesChanged() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::onLinkPropertiesChanged() returned, postId: " +
                             postId);

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::onLinkPropertiesChanged() returned status: " +
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

std::tuple<bool, double> NMWrapper::onPersonaLinksChanged(std::string recipientPersona,
                                                          LinkType linkType,
                                                          const std::vector<LinkID> &links,
                                                          int32_t timeout) {
    TRACE_METHOD(getId(), recipientPersona, linkType);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting IRacePluginNM::onPersonaLinksChanged(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("callback", 0, timeout, [=] {
            helper::logDebug("Calling IRacePluginNM::onPersonaLinksChanged(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->onPersonaLinksChanged(recipientPersona, linkType, links);
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::onPersonaLinksChanged() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::onPersonaLinksChanged() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::onPersonaLinksChanged() returned, postId: " + postId);

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::onPersonaLinksChanged() returned status: " +
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

std::tuple<bool, double> NMWrapper::onUserInputReceived(RaceHandle handle, bool answered,
                                                        const std::string &userResponse,
                                                        int32_t timeout) {
    TRACE_METHOD(getId(), handle, answered, userResponse);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting IRacePluginNM::onUserInputReceived(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("callback", 0, timeout, [=] {
            helper::logDebug("Calling IRacePluginNM::onUserInputReceived(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->onUserInputReceived(handle, answered, userResponse);
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::onUserInputReceived() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::onUserInputReceived() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::onUserInputReceived() returned, postId: " + postId);

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::onUserInputReceived() returned status: " +
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

std::tuple<bool, double> NMWrapper::onUserAcknowledgementReceived(RaceHandle handle,
                                                                  int32_t timeout) {
    TRACE_METHOD(getId(), handle);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting IRacePluginNM::onUserAcknowledgementReceived(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("callback", 0, timeout, [=] {
            helper::logDebug("Calling IRacePluginNM::onUserAcknowledgementReceived(), postId: " +
                             postId);
            PluginResponse response;
            try {
                response = mPlugin->onUserAcknowledgementReceived(handle);
            } catch (std::exception &e) {
                helper::logError(
                    "IRacePluginNM::onUserAcknowledgementReceived() threw exception: " +
                    std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError(
                    "IRacePluginNM::onUserAcknowledgementReceived() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::onUserAcknowledgementReceived() returned, postId: " +
                             postId);

            if (response != PLUGIN_OK) {
                helper::logError(
                    "IRacePluginNM::onUserAcknowledgementReceived() returned status: " +
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

std::tuple<bool, double> NMWrapper::notifyEpoch(const std::string &data, int32_t timeout) {
    TRACE_METHOD(getId(), data);
    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting IRacePluginNM::notifyEpoch(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            helper::logDebug("Calling IRacePluginNM::notifyEpoch(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->notifyEpoch(data);
            } catch (std::exception &e) {
                helper::logError("IRacePluginNM::notifyEpoch() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginNM::notifyEpoch() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginNM::notifyEpoch() returned, postId: " + postId);

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginNM::notifyEpoch() returned status: " +
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

RawData NMWrapper::getEntropy(std::uint32_t numBytes) {
    TRACE_METHOD(getId(), numBytes);
    auto response = raceSdk.getEntropy(numBytes);
    return response;
}

std::string NMWrapper::getActivePersona() {
    TRACE_METHOD(getId());
    auto response = raceSdk.getActivePersona();
    return response;
}

SdkResponse NMWrapper::asyncError(RaceHandle handle, PluginResponse status) {
    TRACE_METHOD(getId(), handle, status);
    auto response = raceSdk.asyncError(handle, status);
    return response;
}

SdkResponse NMWrapper::makeDir(const std::string &directoryPath) {
    TRACE_METHOD(getId(), directoryPath);
    if (!helper::makeDir(directoryPath, getId(), raceSdk.getAppConfig().baseConfigPath)) {
        return SDK_INVALID_ARGUMENT;
    }
    return SDK_OK;
}

SdkResponse NMWrapper::removeDir(const std::string &directoryPath) {
    TRACE_METHOD(getId(), directoryPath);
    if (!helper::removeDir(directoryPath, getId(), raceSdk.getAppConfig().baseConfigPath)) {
        return SDK_INVALID_ARGUMENT;
    }
    return SDK_OK;
}

std::vector<std::string> NMWrapper::listDir(const std::string &directoryPath) {
    TRACE_METHOD(getId(), directoryPath);
    std::vector<std::string> contents =
        helper::listDir(directoryPath, getId(), raceSdk.getAppConfig().baseConfigPath);
    return contents;
}

std::vector<std::uint8_t> NMWrapper::readFile(const std::string &filename) {
    TRACE_METHOD(getId(), filename);
    std::vector<std::uint8_t> data = helper::readFile(
        filename, getId(), raceSdk.getAppConfig().baseConfigPath, raceSdk.getPluginStorage());
    return data;
}

SdkResponse NMWrapper::appendFile(const std::string &filename,
                                  const std::vector<std::uint8_t> &data) {
    TRACE_METHOD(getId(), filename);
    if (!helper::appendFile(filename, getId(), raceSdk.getAppConfig().baseConfigPath, data,
                            raceSdk.getPluginStorage())) {
        return SDK_INVALID_ARGUMENT;
    }
    return SDK_OK;
}

SdkResponse NMWrapper::writeFile(const std::string &filename,
                                 const std::vector<std::uint8_t> &data) {
    TRACE_METHOD(getId(), filename);
    if (!helper::writeFile(filename, getId(), raceSdk.getAppConfig().baseConfigPath, data,
                           raceSdk.getPluginStorage())) {
        return SDK_INVALID_ARGUMENT;
    }
    return SDK_OK;
}

SdkResponse NMWrapper::requestPluginUserInput(const std::string &key, const std::string &prompt,
                                              bool cache) {
    TRACE_METHOD(getId(), key, prompt, cache);
    SdkResponse response =
        raceSdk.requestPluginUserInput(getId(), isTestHarness(), key, prompt, cache);
    return response;
}

SdkResponse NMWrapper::requestCommonUserInput(const std::string &key) {
    TRACE_METHOD(getId(), key);
    SdkResponse response = raceSdk.requestCommonUserInput(getId(), isTestHarness(), key);
    return response;
}

SdkResponse NMWrapper::flushChannel(std::string channelGid, uint64_t batchId,
                                    std::int32_t timeout) {
    TRACE_METHOD(getId(), channelGid, batchId);
    SdkResponse response = raceSdk.flushChannel(*this, channelGid, batchId, timeout);
    return response;
}

SdkResponse NMWrapper::sendEncryptedPackage(EncPkg ePkg, ConnectionID connectionId,
                                            uint64_t batchId, int32_t timeout) {
    TRACE_METHOD(getId(), connectionId, batchId);
    SdkResponse response =
        raceSdk.sendEncryptedPackage(*this, ePkg, connectionId, batchId, timeout);
    return response;
}

SdkResponse NMWrapper::presentCleartextMessage(ClrMsg msg) {
    TRACE_METHOD(getId());
    SdkResponse response = raceSdk.presentCleartextMessage(*this, msg);
    return response;
}

SdkResponse NMWrapper::onPluginStatusChanged(PluginStatus status) {
    TRACE_METHOD(getId(), status);
    SdkResponse response = raceSdk.onPluginStatusChanged(*this, status);
    return response;
}

SdkResponse NMWrapper::openConnection(LinkType linkType, LinkID linkId, std::string linkHints,
                                      int32_t priority, int32_t sendTimeout, int32_t timeout) {
    TRACE_METHOD(getId(), linkType, linkId, linkHints, priority, sendTimeout);
    SdkResponse response =
        raceSdk.openConnection(*this, linkType, linkId, linkHints, priority, sendTimeout, timeout);
    return response;
}

SdkResponse NMWrapper::closeConnection(ConnectionID connectionId, int32_t timeout) {
    TRACE_METHOD(getId(), connectionId);
    SdkResponse response = raceSdk.closeConnection(*this, connectionId, timeout);
    return response;
}

std::vector<LinkID> NMWrapper::getLinksForPersonas(std::vector<std::string> recipientPersonas,
                                                   LinkType linkType) {
    TRACE_METHOD(getId());
    auto response = raceSdk.getLinksForPersonas(recipientPersonas, linkType);
    return response;
}

std::vector<LinkID> NMWrapper::getLinksForChannel(std::string channelGid) {
    TRACE_METHOD(getId(), channelGid);
    auto response = raceSdk.getLinksForChannel(channelGid);
    return response;
}

LinkID NMWrapper::getLinkForConnection(ConnectionID connectionId) {
    TRACE_METHOD(getId(), connectionId);
    LinkID response = raceSdk.getLinkForConnection(connectionId);
    return response;
}

LinkProperties NMWrapper::getLinkProperties(LinkID linkId) {
    TRACE_METHOD(getId(), linkId);
    LinkProperties response = raceSdk.getLinkProperties(linkId);
    return response;
}

std::map<std::string, ChannelProperties> NMWrapper::getSupportedChannels() {
    TRACE_METHOD(getId());
    auto response = raceSdk.getSupportedChannels();
    return response;
}

ChannelProperties NMWrapper::getChannelProperties(std::string channelGid) {
    TRACE_METHOD(getId(), channelGid);
    ChannelProperties response = raceSdk.getChannelProperties(channelGid);
    return response;
}

std::vector<ChannelProperties> NMWrapper::getAllChannelProperties() {
    TRACE_METHOD(getId());
    std::vector<ChannelProperties> response = raceSdk.getAllChannelProperties();
    return response;
}

SdkResponse NMWrapper::deactivateChannel(std::string channelGid, std::int32_t timeout) {
    TRACE_METHOD(getId(), channelGid);
    SdkResponse response = raceSdk.deactivateChannel(*this, channelGid, timeout);
    return response;
}

SdkResponse NMWrapper::activateChannel(std::string channelGid, std::string roleName,
                                       std::int32_t timeout) {
    TRACE_METHOD(getId(), channelGid, roleName);
    SdkResponse response = raceSdk.activateChannel(*this, channelGid, roleName, timeout);
    return response;
}

SdkResponse NMWrapper::destroyLink(LinkID linkId, std::int32_t timeout) {
    TRACE_METHOD(getId(), linkId);
    SdkResponse response = raceSdk.destroyLink(*this, linkId, timeout);
    return response;
}

SdkResponse NMWrapper::createLink(std::string channelGid, std::vector<std::string> personas,
                                  std::int32_t timeout) {
    TRACE_METHOD(getId(), channelGid);
    SdkResponse response = raceSdk.createLink(*this, channelGid, personas, timeout);
    return response;
}

SdkResponse NMWrapper::loadLinkAddress(std::string channelGid, std::string linkAddress,
                                       std::vector<std::string> personas, std::int32_t timeout) {
    TRACE_METHOD(getId(), channelGid, linkAddress);
    SdkResponse response =
        raceSdk.loadLinkAddress(*this, channelGid, linkAddress, personas, timeout);
    return response;
}

SdkResponse NMWrapper::createLinkFromAddress(std::string channelGid, std::string linkAddress,
                                             std::vector<std::string> personas,
                                             std::int32_t timeout) {
    TRACE_METHOD(getId(), channelGid, linkAddress);
    SdkResponse response =
        raceSdk.createLinkFromAddress(*this, channelGid, linkAddress, personas, timeout);
    return response;
}

SdkResponse NMWrapper::loadLinkAddresses(std::string channelGid,
                                         std::vector<std::string> linkAddresses,
                                         std::vector<std::string> personas, std::int32_t timeout) {
    TRACE_METHOD(getId(), channelGid);
    SdkResponse response =
        raceSdk.loadLinkAddresses(*this, channelGid, linkAddresses, personas, timeout);
    return response;
}

SdkResponse NMWrapper::bootstrapDevice(RaceHandle handle, std::vector<std::string> commsChannels) {
    TRACE_METHOD(getId(), handle);
    SdkResponse response = raceSdk.bootstrapDevice(*this, handle, commsChannels);
    return response;
}

SdkResponse NMWrapper::bootstrapFailed(RaceHandle handle) {
    TRACE_METHOD(getId(), handle);
    SdkResponse response = raceSdk.bootstrapFailed(handle);
    return response;
}

SdkResponse NMWrapper::setPersonasForLink(std::string linkId, std::vector<std::string> personas) {
    TRACE_METHOD(getId(), linkId);
    SdkResponse response = raceSdk.setPersonasForLink(*this, linkId, personas);
    return response;
}

std::vector<std::string> NMWrapper::getPersonasForLink(std::string linkId) {
    TRACE_METHOD(getId(), linkId);
    auto response = raceSdk.getPersonasForLink(linkId);
    return response;
}

SdkResponse NMWrapper::onMessageStatusChanged(RaceHandle handle, MessageStatus status) {
    TRACE_METHOD(getId(), handle, status);
    auto response = raceSdk.onMessageStatusChanged(handle, status);
    return response;
}

SdkResponse NMWrapper::sendBootstrapPkg(ConnectionID connectionId, std::string persona, RawData key,
                                        std::int32_t timeout) {
    TRACE_METHOD(getId(), connectionId, persona);
    auto response = raceSdk.sendBootstrapPkg(*this, connectionId, persona, key, timeout);
    return response;
}

SdkResponse NMWrapper::displayInfoToUser(const std::string &data,
                                         RaceEnums::UserDisplayType displayType) {
    TRACE_METHOD(getId(), data, displayType);
    auto response = raceSdk.displayInfoToUser(getId(), data, displayType);
    return response;
}
