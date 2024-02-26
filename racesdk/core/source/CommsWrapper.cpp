
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

#include "CommsWrapper.h"

#include <jaegertracing/Tracer.h>

#include <future>  // std::promise, std::future

#include "OpenTracingHelpers.h"
#include "filesystem.h"
#include "helper.h"

CommsWrapper::CommsWrapper(RaceSdk &sdk, const std::string &name) :
    raceSdk(sdk),
    mTracer(sdk.getTracer()),
    mThreadHandler(name + "-thread", sdk.getRaceConfig().wrapperQueueMaxSize,
                   sdk.getRaceConfig().wrapperTotalMaxSize) {
    createQueue("lifecycle", std::numeric_limits<int>::max());
    createQueue("wait queue", std::numeric_limits<int>::min());
}

CommsWrapper::CommsWrapper(std::shared_ptr<IRacePluginComms> plugin, std::string id,
                           std::string description, RaceSdk &sdk, const std::string &configPath) :
    raceSdk(sdk),
    mTracer(sdk.getTracer()),
    mThreadHandler(id + "-thread", sdk.getRaceConfig().wrapperQueueMaxSize,
                   sdk.getRaceConfig().wrapperTotalMaxSize),
    mPlugin(std::move(plugin)),
    mId(std::move(id)),
    mDescription(std::move(description)),
    mConfigPath(configPath) {
    createQueue("lifecycle", std::numeric_limits<int>::max());
    createQueue("wait queue", std::numeric_limits<int>::min());
}

CommsWrapper::~CommsWrapper() {
    TRACE_METHOD(getId());
}

void CommsWrapper::startHandler() {
    TRACE_METHOD(getId());
    mThreadHandler.start();
}

void CommsWrapper::stopHandler() {
    TRACE_METHOD(getId());
    mThreadHandler.stop();
}

void CommsWrapper::waitForCallbacks() {
    auto [success, queueSize, future] =
        mThreadHandler.post("wait queue", 0, -1, [=] { return std::make_optional(true); });
    (void)success;
    (void)queueSize;
    future.wait();
}

void CommsWrapper::createQueue(const std::string &name, int priority) {
    TRACE_METHOD(getId(), name, priority);
    mThreadHandler.create_queue(name, priority);
}

void CommsWrapper::removeQueue(const std::string &name) {
    TRACE_METHOD(getId(), name);
    mThreadHandler.remove_queue(name);
}

bool CommsWrapper::init(const PluginConfig &pluginConfig) {
    TRACE_METHOD(getId());
    PluginResponse response = mPlugin->init(pluginConfig);
    state = INITIALIZED;

    if (response != PLUGIN_OK) {
        helper::logError("IRacePluginComms::init() returned status: " +
                         helper::pluginResponseToString(response));

        // tell the caller to close the plugin
        return false;
    }

    return true;
}

bool CommsWrapper::shutdown() {
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

bool CommsWrapper::shutdown(std::int32_t timeoutInSeconds) {
    TRACE_METHOD(getId(), timeoutInSeconds);
    // shutdown is only valid to call in INITIALIZED state
    // if we've only been constructed, but not initialized, then we don't need to call init
    // (specifically, the handler may not be started yet)
    if (state != INITIALIZED) {
        // set state to shutdown even if we don't call the shutdown method. This is a terminal state
        // and indicates that nothing more should be done with this plugin
        state = SHUTDOWN;
        return false;
    }

    std::string postId = std::to_string(nextPostId++);
    state = SHUTDOWN;
    helper::logDebug("Posting IRacePluginComms::shutdown(), postId: " + postId);

    try {
        auto [success, queueSize, future] = mThreadHandler.post("lifecycle", 0, -1, [=] {
            helper::logDebug("Calling IRacePluginComms::shutdown(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->shutdown();
                helper::logDebug("IRacePluginComms::shutdown() returned " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::shutdown() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::shutdown() threw exception: ");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::shutdown() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::shutdown() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
                // I, uh, guess we're already closing
                return std::make_optional(false);
            }

            return std::make_optional(true);
        });

        if (timeoutInSeconds == WAIT_FOREVER) {
            future.wait();
        } else {
            std::future_status status = future.wait_for(std::chrono::seconds(timeoutInSeconds));
            if (status != std::future_status::ready) {
                helper::logError("IRacePluginComms::shutdown() timed out, took longer than " +
                                 std::to_string(timeoutInSeconds) + " seconds");
            }
        }

        // this really shouldn't happen...
        if (success == Handler::PostStatus::INVALID_STATE) {
            helper::logError("Shutting down " + mId + " failed with error INVALID_STATE");
        } else if (success == Handler::PostStatus::QUEUE_FULL) {
            helper::logError("Shutting down " + mId + " failed with error QUEUE_FULL");
        } else if (success == Handler::PostStatus::HANDLER_FULL) {
            helper::logError("Shutting down " + mId + " failed with error HANDLER_FULL");
        }

        (void)queueSize;
        return success == Handler::PostStatus::OK;
    } catch (std::out_of_range &error) {
        helper::logError("Lifecycle queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return false;
}

SdkResponse CommsWrapper::sendPackage(RaceHandle handle, const ConnectionID &connectionId,
                                      const EncPkg &pkg, int32_t postTimeout, uint64_t batchId) {
    TRACE_METHOD(getId(), handle, connectionId, postTimeout, batchId);
    if (state == SHUTDOWN) {
        return SDK_SHUTTING_DOWN;
    }

    double sendTimeout;
    try {
        std::lock_guard<std::mutex> lock(mConnectionSendTimeoutMapMutex);
        sendTimeout = mConnectionSendTimeoutMap[connectionId];
    } catch (std::out_of_range &error) {
        helper::logError("Queue for connection '" + connectionId +
                         "' does not exist. what:" + std::string(error.what()));
        return SDK_INVALID_ARGUMENT;
    }

    std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
    double timeoutTimestamp = now.count() + sendTimeout;

    std::string postId = std::to_string(nextPostId++);

    auto pkgSpanContext = spanContextFromEncryptedPackage(pkg);
    std::shared_ptr<opentracing::Span> span =
        mTracer->StartSpan("sendPackage", {opentracing::ChildOf(pkgSpanContext.get())});

    span->SetTag("source", "racesdk");
    span->SetTag("file", __FILE__);
    span->SetTag("pluginId", mId);
    span->SetTag("connectionId", connectionId);

    EncPkg newPkg = pkg;
    newPkg.setTraceId(traceIdFromContext(span->context()));
    newPkg.setSpanId(spanIdFromContext(span->context()));

    helper::logInfo("Posting IRacePluginComms::sendPackage(), postId: " + postId +
                    " traceId: " + helper::convertToHexString(newPkg.getTraceId()) +
                    " spanId: " + helper::convertToHexString(newPkg.getSpanId()) +
                    " postTimeout: " + std::to_string(postTimeout) +
                    " batchId: " + std::to_string(batchId));

    uint32_t pkgSize = newPkg.getSize();

    try {
        auto [success, queueSize, future] = mThreadHandler.post(
            connectionId, pkgSize, postTimeout,
            [=]() -> std::optional<bool> {
                if (state == SHUTDOWN) {
                    onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, 0);
                    return std::make_optional(false);
                }

                helper::logInfo("Calling IRacePluginComms::sendPackage(), postId: " + postId +
                                " traceId: " + helper::convertToHexString(newPkg.getTraceId()) +
                                " spanId: " + helper::convertToHexString(newPkg.getSpanId()));
                PluginResponse response;
                try {
                    response = mPlugin->sendPackage(handle, connectionId, newPkg, timeoutTimestamp,
                                                    batchId);
                    helper::logDebug("IRacePluginComms::sendPackage() returned " +
                                     helper::pluginResponseToString(response) +
                                     ", postId: " + postId);
                } catch (std::exception &e) {
                    helper::logError("IRacePluginComms::sendPackage() threw exception: " +
                                     std::string(e.what()));
                    response = PLUGIN_FATAL;
                } catch (...) {
                    helper::logError("IRacePluginComms::sendPackage() threw exception: ");
                    response = PLUGIN_FATAL;
                }
                span->Finish();
                helper::logInfo("IRacePluginComms::sendPackage() returned " +
                                helper::pluginResponseToString(response) + ", postId: " + postId +
                                " traceId: " + helper::convertToHexString(newPkg.getTraceId()) +
                                " spanId: " + helper::convertToHexString(newPkg.getSpanId()));

                // We still expect a callback later, so no need to remove from connection map
                if (response == PLUGIN_ERROR) {  // NOLINT(bugprone-branch-clone)
                    helper::logError("IRacePluginComms::sendPackage() returned status: " +
                                     helper::pluginResponseToString(response) +
                                     ", postId: " + postId);
                } else if (response == PLUGIN_TEMP_ERROR) {
                    helper::logDebug("IRacePluginComms::sendPackage(): blocking queue");
                    helper::logInfo("IRacePluginComms::sendPackage() returned status: " +
                                    helper::pluginResponseToString(response) +
                                    ", postId: " + postId);

                    // this will cause this queue to block
                    return std::nullopt;
                } else if (response == PLUGIN_FATAL) {
                    helper::logError("IRacePluginComms::sendPackage() returned status: " +
                                     helper::pluginResponseToString(response) +
                                     ", postId: " + postId);
                    // shut down and remove this plugin
                    raceSdk.shutdownPluginAsync(*this);
                } else if (response != PLUGIN_OK) {
                    helper::logError("IRacePluginComms::sendPackage() returned status: " +
                                     helper::pluginResponseToString(response) +
                                     ", postId: " + postId);
                }

                return std::make_optional(true);
            },
            timeoutTimestamp,
            [=] {
                helper::logWarning("Package timed out. Handle: " + std::to_string(handle) +
                                   ", Connection: " + connectionId);
                SdkResponse response =
                    raceSdk.onPackageStatusChanged(*this, handle, PACKAGE_FAILED_TIMEOUT, 0);
                if (response.status != SDK_OK) {
                    helper::logWarning(
                        "Failed to post package timeout callback to network manager: " +
                        std::to_string(response.status));
                }
            });

        if (success != Handler::PostStatus::OK) {
            if (success == Handler::PostStatus::INVALID_STATE) {
                helper::logError("Post on connection " + connectionId +
                                 " failed with error INVALID_STATE");
            } else if (success == Handler::PostStatus::QUEUE_FULL) {
                helper::logError("Post on connection " + connectionId +
                                 " failed with error QUEUE_FULL");
            } else if (success == Handler::PostStatus::HANDLER_FULL) {
                helper::logError("Post on connection " + connectionId +
                                 " failed with error HANDLER_FULL");
            }
        }

        (void)future;
        return makeResponse(__func__, success == Handler::PostStatus::OK, queueSize, handle);
    } catch (std::out_of_range &error) {
        helper::logError("Queue for connection '" + connectionId +
                         "' does not exist. what:" + std::string(error.what()));
    }

    return SDK_INVALID_ARGUMENT;
}

SdkResponse CommsWrapper::openConnection(RaceHandle handle, LinkType linkType, const LinkID &linkId,
                                         const std::string &linkHints, int32_t priority,
                                         int32_t sendTimeout, int32_t timeout) {
    TRACE_METHOD(getId(), handle, linkType, linkId, linkHints, priority, sendTimeout);
    if (state == SHUTDOWN) {
        return SDK_SHUTTING_DOWN;
    }

    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Posting IRacePluginComms::openConnection(), postId: " + postId);

    {
        std::lock_guard<std::mutex> lock(mHandlePriorityTimeoutMapMutex);
        if (sendTimeout == RACE_UNLIMITED) {
            mHandlePriorityTimeoutMap[handle] = {priority, std::numeric_limits<double>::infinity()};
        } else {
            mHandlePriorityTimeoutMap[handle] = {priority, sendTimeout};
        }
    }

    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            if (state == SHUTDOWN) {
                onConnectionStatusChanged(handle, generateConnectionId(linkId), CONNECTION_CLOSED,
                                          {}, 0);
                return std::make_optional(false);
            }

            helper::logDebug("Calling IRacePluginComms::openConnection(), postId: " + postId);
            PluginResponse response;
            try {
                response =
                    mPlugin->openConnection(handle, linkType, linkId, linkHints, sendTimeout);
                helper::logDebug("IRacePluginComms::openConnection() returned " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::openConnection() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::openConnection() threw exception: ");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::openConnection() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            // We could block the queue for opening connections, but how would we know to unblock
            // it? if we did block it, shutdown would have to be moved to a different queue. We
            // still expect a callback later, so no need to remove from connection map
            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::openConnection() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        if (success != Handler::PostStatus::OK) {
            if (success == Handler::PostStatus::INVALID_STATE) {
                helper::logError("Opening connection on " + mId +
                                 " failed with error INVALID_STATE");
            } else if (success == Handler::PostStatus::QUEUE_FULL) {
                helper::logError("Opening connection on " + mId + " failed with error QUEUE_FULL");
            } else if (success == Handler::PostStatus::HANDLER_FULL) {
                helper::logError("Opening connection on " + mId +
                                 " failed with error HANDLER_FULL");
            }
            std::lock_guard<std::mutex> lock(mHandlePriorityTimeoutMapMutex);
            mHandlePriorityTimeoutMap.erase(handle);
        }

        (void)future;
        return makeResponse(__func__, success == Handler::PostStatus::OK, queueSize, handle);
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));

        std::lock_guard<std::mutex> lock(mHandlePriorityTimeoutMapMutex);
        mHandlePriorityTimeoutMap.erase(handle);
    }

    return SDK_INVALID;
}

SdkResponse CommsWrapper::closeConnection(RaceHandle handle, const ConnectionID &connectionId,
                                          int32_t timeout) {
    TRACE_METHOD(getId(), handle, connectionId);
    if (state == SHUTDOWN) {
        return SDK_SHUTTING_DOWN;
    }

    std::string postId = std::to_string(nextPostId++);

    helper::logDebug("Posting IRacePluginComms::closeConnection(), postId: " + postId);

    try {
        auto [success, queueSize, future] = mThreadHandler.post(connectionId, 0, timeout, [=] {
            if (state == SHUTDOWN) {
                // plugin should take care of any open connection in shutdown and issue
                // onConnectionStatusChanged there
                return std::make_optional(false);
            }

            helper::logDebug("Calling IRacePluginComms::closeConnection(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->closeConnection(handle, connectionId);
                helper::logDebug("IRacePluginComms::closeConnection() returned " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::closeConnection() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::closeConnection() threw exception: ");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::closeConnection() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::closeConnection() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        if (success == Handler::PostStatus::INVALID_STATE) {
            helper::logError("Closing connection on " + mId + " failed with error INVALID_STATE");
        } else if (success == Handler::PostStatus::QUEUE_FULL) {
            helper::logError("Closing connection on " + mId + " failed with error QUEUE_FULL");
        } else if (success == Handler::PostStatus::HANDLER_FULL) {
            helper::logError("Closing connection on " + mId + " failed with error HANDLER_FULL");
        }

        (void)future;
        return makeResponse(__func__, success == Handler::PostStatus::OK, queueSize, handle);
    } catch (std::out_of_range &error) {
        helper::logError("Queue for connection '" + connectionId +
                         "' does not exist. what:" + std::string(error.what()));
    }

    return SDK_INVALID_ARGUMENT;
}

SdkResponse CommsWrapper::deactivateChannel(RaceHandle handle, const std::string &channelGid,
                                            std::int32_t timeout) {
    TRACE_METHOD(getId(), handle, channelGid);
    if (state == SHUTDOWN) {
        return SDK_SHUTTING_DOWN;
    }

    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Posting IRacePluginComms::deactivateChannel(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            if (state == SHUTDOWN) {
                return std::make_optional(false);
            }

            helper::logDebug("Calling IRacePluginComms::deactivateChannel(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->deactivateChannel(handle, channelGid);
                helper::logDebug("IRacePluginComms::deactivateChannel() returned " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::deactivateChannel() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::deactivateChannel() threw exception: ");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::deactivateChannel() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::deactivateChannel() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        if (success != Handler::PostStatus::OK) {
            if (success == Handler::PostStatus::INVALID_STATE) {
                helper::logError("deactivateChannel " + mId + " failed with error INVALID_STATE");
            } else if (success == Handler::PostStatus::QUEUE_FULL) {
                helper::logError("deactivateChannel " + mId + " failed with error QUEUE_FULL");
            } else if (success == Handler::PostStatus::HANDLER_FULL) {
                helper::logError("deactivateChannel " + mId + " failed with error HANDLER_FULL");
            }
        }

        (void)future;
        return makeResponse(__func__, success == Handler::PostStatus::OK, queueSize, handle);
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return SDK_INVALID;
}

SdkResponse CommsWrapper::activateChannel(RaceHandle handle, const std::string &channelGid,
                                          const std::string &roleName, std::int32_t timeout) {
    TRACE_METHOD(getId(), handle, channelGid, roleName);
    if (state == SHUTDOWN) {
        return SDK_SHUTTING_DOWN;
    }

    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Posting IRacePluginComms::activateChannel(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            if (state == SHUTDOWN) {
                return std::make_optional(false);
            }

            helper::logDebug("Calling IRacePluginComms::activateChannel(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->activateChannel(handle, channelGid, roleName);
                helper::logDebug("IRacePluginComms::activateChannel() returned " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::activateChannel() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::activateChannel() threw exception: ");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::activateChannel() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::activateChannel() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        if (success != Handler::PostStatus::OK) {
            if (success == Handler::PostStatus::INVALID_STATE) {
                helper::logError("activateChannel " + mId + " failed with error INVALID_STATE");
            } else if (success == Handler::PostStatus::QUEUE_FULL) {
                helper::logError("activateChannel " + mId + " failed with error QUEUE_FULL");
            } else if (success == Handler::PostStatus::HANDLER_FULL) {
                helper::logError("activateChannel " + mId + " failed with error HANDLER_FULL");
            }
            // Set Channel Status to failed to notify network manager
            ChannelProperties defaultProperties;
            defaultProperties.channelStatus = CHANNEL_FAILED;
            onChannelStatusChanged(handle, channelGid, CHANNEL_FAILED, defaultProperties, timeout);
        }

        (void)future;
        return makeResponse(__func__, success == Handler::PostStatus::OK, queueSize, handle);
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return SDK_INVALID;
}

SdkResponse CommsWrapper::destroyLink(RaceHandle handle, const LinkID &linkId,
                                      std::int32_t timeout) {
    TRACE_METHOD(getId(), handle, linkId);
    if (state == SHUTDOWN) {
        return SDK_SHUTTING_DOWN;
    }

    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Posting IRacePluginComms::destroyLink(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            if (state == SHUTDOWN) {
                return std::make_optional(false);
            }

            helper::logDebug("Calling IRacePluginComms::destroyLink(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->destroyLink(handle, linkId);
                helper::logDebug("IRacePluginComms::destroyLink() returned " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::destroyLink() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::destroyLink() threw exception: ");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::destroyLink() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::destroyLink() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        if (success != Handler::PostStatus::OK) {
            if (success == Handler::PostStatus::INVALID_STATE) {
                helper::logError("Destroy link on " + mId + " failed with error INVALID_STATE");
            } else if (success == Handler::PostStatus::QUEUE_FULL) {
                helper::logError("Destroy link on " + mId + " failed with error QUEUE_FULL");
            } else if (success == Handler::PostStatus::HANDLER_FULL) {
                helper::logError("Destroy link on " + mId + " failed with error HANDLER_FULL");
            }
        }

        (void)future;
        return makeResponse(__func__, success == Handler::PostStatus::OK, queueSize, handle);
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return SDK_INVALID;
}

SdkResponse CommsWrapper::createLink(RaceHandle handle, const std::string &channelGid,
                                     std::int32_t timeout) {
    TRACE_METHOD(getId(), handle, channelGid);
    if (state == SHUTDOWN) {
        return SDK_SHUTTING_DOWN;
    }

    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Posting IRacePluginComms::createLink(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            if (state == SHUTDOWN) {
                onLinkStatusChanged(handle, generateLinkId(channelGid), LINK_DESTROYED, {}, 0);
                return std::make_optional(false);
            }

            helper::logDebug("Calling IRacePluginComms::createLink(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->createLink(handle, channelGid);
                helper::logDebug("IRacePluginComms::createLink() returned " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::createLink() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::createLink() threw exception: ");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::createLink() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::createLink() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        if (success != Handler::PostStatus::OK) {
            if (success == Handler::PostStatus::INVALID_STATE) {
                helper::logError("Creating link on " + mId + " failed with error INVALID_STATE");
            } else if (success == Handler::PostStatus::QUEUE_FULL) {
                helper::logError("Creating link on " + mId + " failed with error QUEUE_FULL");
            } else if (success == Handler::PostStatus::HANDLER_FULL) {
                helper::logError("Creating link on " + mId + " failed with error HANDLER_FULL");
            }
        }

        (void)future;
        return makeResponse(__func__, success == Handler::PostStatus::OK, queueSize, handle);
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return SDK_INVALID;
}

SdkResponse CommsWrapper::createBootstrapLink(RaceHandle handle, const std::string &channelGid,
                                              const std::string &passphrase, std::int32_t timeout) {
    TRACE_METHOD(getId(), handle, channelGid, passphrase);
    if (state == SHUTDOWN) {
        return SDK_SHUTTING_DOWN;
    }

    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Posting IRacePluginComms::createBootstrapLink(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            if (state == SHUTDOWN) {
                onLinkStatusChanged(handle, generateLinkId(channelGid), LINK_DESTROYED, {}, 0);
                return std::make_optional(false);
            }

            helper::logDebug("Calling IRacePluginComms::createBootstrapLink(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->createBootstrapLink(handle, channelGid, passphrase);
                helper::logDebug("IRacePluginComms::createBootstrapLink() returned " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::createBootstrapLink() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::createBootstrapLink() threw exception: ");
                response = PLUGIN_FATAL;
            }

            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::createBootstrapLink() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        if (success != Handler::PostStatus::OK) {
            if (success == Handler::PostStatus::INVALID_STATE) {
                helper::logError("Creating link on " + mId + " failed with error INVALID_STATE");
            } else if (success == Handler::PostStatus::QUEUE_FULL) {
                helper::logError("Creating link on " + mId + " failed with error QUEUE_FULL");
            } else if (success == Handler::PostStatus::HANDLER_FULL) {
                helper::logError("Creating link on " + mId + " failed with error HANDLER_FULL");
            }
        }

        (void)future;
        return makeResponse(__func__, success == Handler::PostStatus::OK, queueSize, handle);
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return SDK_INVALID;
}

SdkResponse CommsWrapper::loadLinkAddress(RaceHandle handle, const std::string &channelGid,
                                          const std::string &linkAddress, std::int32_t timeout) {
    TRACE_METHOD(getId(), handle, channelGid, linkAddress);
    if (state == SHUTDOWN) {
        return SDK_SHUTTING_DOWN;
    }

    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Calling IRacePluginComms::loadLinkAddress(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            if (state == SHUTDOWN) {
                onLinkStatusChanged(handle, generateLinkId(channelGid), LINK_DESTROYED, {}, 0);
                return std::make_optional(false);
            }

            helper::logDebug("Calling IRacePluginComms::loadLinkAddress(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->loadLinkAddress(handle, channelGid, linkAddress);
                helper::logDebug("IRacePluginComms::loadLinkAddress() returned " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::loadLinkAddress() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::loadLinkAddress() threw exception: ");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::loadLinkAddress() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::loadLinkAddress() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        if (success != Handler::PostStatus::OK) {
            if (success == Handler::PostStatus::INVALID_STATE) {
                helper::logError("Loading link address on " + mId +
                                 " failed with error INVALID_STATE");
            } else if (success == Handler::PostStatus::QUEUE_FULL) {
                helper::logError("Loading link address on " + mId +
                                 " failed with error QUEUE_FULL");
            } else if (success == Handler::PostStatus::HANDLER_FULL) {
                helper::logError("Loading link address on " + mId +
                                 " failed with error HANDLER_FULL");
            }
        }

        (void)future;
        return makeResponse(__func__, success == Handler::PostStatus::OK, queueSize, handle);
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return SDK_INVALID;
}
SdkResponse CommsWrapper::loadLinkAddresses(RaceHandle handle, const std::string &channelGid,
                                            std::vector<std::string> linkAddresses,
                                            std::int32_t timeout) {
    TRACE_METHOD(getId(), handle, channelGid);
    if (state == SHUTDOWN) {
        return SDK_SHUTTING_DOWN;
    }

    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Calling IRacePluginComms::loadLinkAddresses(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            if (state == SHUTDOWN) {
                onLinkStatusChanged(handle, generateLinkId(channelGid), LINK_DESTROYED, {}, 0);
                return std::make_optional(false);
            }

            helper::logDebug("Calling IRacePluginComms::loadLinkAddresses(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->loadLinkAddresses(handle, channelGid, linkAddresses);
                helper::logDebug("IRacePluginComms::loadLinkAddresses() returned " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::loadLinkAddresses() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::loadLinkAddresses() threw exception: ");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::loadLinkAddresses() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::loadLinkAddresses() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        if (success != Handler::PostStatus::OK) {
            if (success == Handler::PostStatus::INVALID_STATE) {
                helper::logError("Loading link addresses on " + mId +
                                 " failed with error INVALID_STATE");
            } else if (success == Handler::PostStatus::QUEUE_FULL) {
                helper::logError("Loading link addresses on " + mId +
                                 " failed with error QUEUE_FULL");
            } else if (success == Handler::PostStatus::HANDLER_FULL) {
                helper::logError("Loading link addresses on " + mId +
                                 " failed with error HANDLER_FULL");
            }
        }

        (void)future;
        return makeResponse(__func__, success == Handler::PostStatus::OK, queueSize, handle);
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return SDK_INVALID;
}

SdkResponse CommsWrapper::createLinkFromAddress(RaceHandle handle, const std::string &channelGid,
                                                const std::string &linkAddress,
                                                std::int32_t timeout) {
    TRACE_METHOD(getId(), handle, channelGid);
    if (state == SHUTDOWN) {
        return SDK_SHUTTING_DOWN;
    }

    std::string postId = std::to_string(nextPostId++);

    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            if (state == SHUTDOWN) {
                onLinkStatusChanged(handle, generateLinkId(channelGid), LINK_DESTROYED, {}, 0);
                return std::make_optional(false);
            }

            helper::logDebug("Calling IRacePluginComms::createLinkFromAddress(), postId: " +
                             postId);
            helper::logDebug("ChannelId: " + channelGid + " LinkAddress: " + linkAddress);
            PluginResponse response;
            try {
                response = mPlugin->createLinkFromAddress(handle, channelGid, linkAddress);
                helper::logDebug("IRacePluginComms::createLinkFromAddress() returned " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::createLinkFromAddress() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::createLinkFromAddress() threw exception: ");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::createLinkFromAddress() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::createLinkFromAddress() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });

        if (success != Handler::PostStatus::OK) {
            if (success == Handler::PostStatus::INVALID_STATE) {
                helper::logError("Creating link from address on " + mId +
                                 " failed with error INVALID_STATE");
            } else if (success == Handler::PostStatus::QUEUE_FULL) {
                helper::logError("Creating link from address on " + mId +
                                 " failed with error QUEUE_FULL");
            } else if (success == Handler::PostStatus::HANDLER_FULL) {
                helper::logError("Creating link from address on " + mId +
                                 " failed with error HANDLER_FULL");
            }
        }

        (void)future;
        return makeResponse(__func__, success == Handler::PostStatus::OK, queueSize, handle);
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }

    return SDK_INVALID;
}

SdkResponse CommsWrapper::serveFiles(LinkID linkId, std::string path, int32_t timeout) {
    TRACE_METHOD(getId(), linkId, path);
    if (state == SHUTDOWN) {
        return SDK_SHUTTING_DOWN;
    }

    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Calling IRacePluginComms::serveFiles(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            if (state == SHUTDOWN) {
                return std::make_optional(false);
            }
            helper::logDebug("Calling IRacePluginComms::serveFiles(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->serveFiles(linkId, path);
                helper::logDebug("IRacePluginComms::serveFiles() returned " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::serveFiles() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::serveFiles() threw exception: ");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::serveFiles() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::serveFiles() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });
        if (success != Handler::PostStatus::OK) {
            if (success == Handler::PostStatus::INVALID_STATE) {
                helper::logError("Loading link addresses on " + mId +
                                 " failed with error INVALID_STATE");
            } else if (success == Handler::PostStatus::QUEUE_FULL) {
                helper::logError("Loading link addresses on " + mId +
                                 " failed with error QUEUE_FULL");
            } else if (success == Handler::PostStatus::HANDLER_FULL) {
                helper::logError("Loading link addresses on " + mId +
                                 " failed with error HANDLER_FULL");
            }
        }

        (void)future;
        return makeResponse(__func__, success == Handler::PostStatus::OK, queueSize,
                            NULL_RACE_HANDLE);
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }
    return SDK_INVALID;
}

SdkResponse CommsWrapper::flushChannel(RaceHandle handle, std::string channelGid, uint64_t batchId,
                                       int32_t timeout) {
    TRACE_METHOD(getId(), handle, channelGid, batchId);
    if (state == SHUTDOWN) {
        return SDK_SHUTTING_DOWN;
    }

    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Calling IRacePluginComms::flushChannel(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            if (state == SHUTDOWN) {
                return std::make_optional(false);
            }
            helper::logDebug("Calling IRacePluginComms::flushChannel(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->flushChannel(handle, channelGid, batchId);
                helper::logDebug("IRacePluginComms::flushChannel() returned " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::flushChannel() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::flushChannel() threw exception: ");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::flushChannel() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::flushChannel() returned status: " +
                                 helper::pluginResponseToString(response) + ", postId: " + postId);
            }

            return std::make_optional(true);
        });
        if (success != Handler::PostStatus::OK) {
            if (success == Handler::PostStatus::INVALID_STATE) {
                helper::logError("CommsWrapper::flushChannel: Loading link addresses on " + mId +
                                 " failed with error INVALID_STATE");
            } else if (success == Handler::PostStatus::QUEUE_FULL) {
                helper::logError("CommsWrapper::flushChannel: Loading link addresses on " + mId +
                                 " failed with error QUEUE_FULL");
            } else if (success == Handler::PostStatus::HANDLER_FULL) {
                helper::logError("CommsWrapper::flushChannel: Loading link addresses on " + mId +
                                 " failed with error HANDLER_FULL");
            }
        }

        (void)future;
        return makeResponse(__func__, success == Handler::PostStatus::OK, queueSize,
                            NULL_RACE_HANDLE);
    } catch (std::out_of_range &error) {
        helper::logError(
            "CommsWrapper::flushChannel: Default queue does not exist. This should never happen. "
            "what:" +
            std::string(error.what()));
    }
    return SDK_INVALID;
}

std::tuple<bool, double> CommsWrapper::onUserInputReceived(RaceHandle handle, bool answered,
                                                           const std::string &userResponse,
                                                           int32_t timeout) {
    TRACE_METHOD(getId(), handle, answered, userResponse);
    if (state == SHUTDOWN) {
        return {false, 0};
    }

    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Posting IRacePluginComms::onUserInputReceived(), postId: " + postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            if (state == SHUTDOWN) {
                return std::make_optional(false);
            }

            helper::logDebug("Calling IRacePluginComms::onUserInputReceived(), postId: " + postId);
            PluginResponse response;
            try {
                response = mPlugin->onUserInputReceived(handle, answered, userResponse);
            } catch (std::exception &e) {
                helper::logError("IRacePluginComms::onUserInputReceived() threw exception: " +
                                 std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError("IRacePluginComms::onUserInputReceived() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::onUserInputReceived() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError("IRacePluginComms::onUserInputReceived() returned status: " +
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

std::tuple<bool, double> CommsWrapper::onUserAcknowledgementReceived(RaceHandle handle,
                                                                     int32_t timeout) {
    TRACE_METHOD(getId(), handle);
    if (state == SHUTDOWN) {
        return {false, 0};
    }

    std::string postId = std::to_string(nextPostId++);
    helper::logDebug("Posting IRacePluginComms::onUserAcknowledgementReceived(), postId: " +
                     postId);
    try {
        auto [success, queueSize, future] = mThreadHandler.post("", 0, timeout, [=] {
            if (state == SHUTDOWN) {
                return std::make_optional(false);
            }

            helper::logDebug("Calling IRacePluginComms::onUserAcknowledgementReceived(), postId: " +
                             postId);
            PluginResponse response;
            try {
                response = mPlugin->onUserAcknowledgementReceived(handle);
            } catch (std::exception &e) {
                helper::logError(
                    "IRacePluginComms::onUserAcknowledgementReceived() threw exception: " +
                    std::string(e.what()));
                response = PLUGIN_FATAL;
            } catch (...) {
                helper::logError(
                    "IRacePluginComms::onUserAcknowledgementReceived() threw an exception");
                response = PLUGIN_FATAL;
            }
            helper::logDebug("IRacePluginComms::onUserAcknowledgementReceived() returned " +
                             helper::pluginResponseToString(response) + ", postId: " + postId);

            if (response == PLUGIN_FATAL) {
                // shut down and remove this plugin
                raceSdk.shutdownPluginAsync(*this);
            }
            if (response != PLUGIN_OK) {
                helper::logError(
                    "IRacePluginComms::onUserAcknowledgementReceived() returned status: " +
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

// Comms SDK wrapping

// IRaceSdkCommon
RawData CommsWrapper::getEntropy(std::uint32_t numBytes) {
    TRACE_METHOD(getId(), numBytes);
    return raceSdk.getEntropy(numBytes);
}

std::string CommsWrapper::getActivePersona() {
    TRACE_METHOD(getId());
    return raceSdk.getActivePersona();
}

SdkResponse CommsWrapper::asyncError(RaceHandle handle, PluginResponse status) {
    TRACE_METHOD(getId(), handle, status);
    if (PLUGIN_FATAL == status) {
        raceSdk.shutdownPluginAsync(*this);
    }
    return SDK_OK;
}

ChannelProperties CommsWrapper::getChannelProperties(std::string channelGid) {
    TRACE_METHOD(getId(), channelGid);
    ChannelProperties response = raceSdk.getChannelProperties(channelGid);
    return response;
}

std::vector<ChannelProperties> CommsWrapper::getAllChannelProperties() {
    TRACE_METHOD(getId());
    std::vector<ChannelProperties> response = raceSdk.getAllChannelProperties();
    return response;
}

/**
 * @brief Create the directory of directoryPath, including any directories in the path that do not
 * yet exist
 * @param directoryPath the path of the directory to create.
 *
 * @return SdkResponse indicator of success or failure of the create
 */
SdkResponse CommsWrapper::makeDir(const std::string &directoryPath) {
    TRACE_METHOD(getId(), directoryPath);
    if (!helper::makeDir(directoryPath, getId(), raceSdk.getAppConfig().baseConfigPath)) {
        return SDK_INVALID_ARGUMENT;
    }
    return SDK_OK;
}

/**
 * @brief Recurively remove the directory of directoryPath
 * @param directoryPath the path of the directory to remove.
 *
 * @return SdkResponse indicator of success or failure of the removal
 */
SdkResponse CommsWrapper::removeDir(const std::string &directoryPath) {
    TRACE_METHOD(getId(), directoryPath);
    if (!helper::removeDir(directoryPath, getId(), raceSdk.getAppConfig().baseConfigPath)) {
        return SDK_INVALID_ARGUMENT;
    }
    return SDK_OK;
}

/**
 * @brief List the contents (directories and files) of the directory path
 * @param directoryPath the path of the directory to list.
 *
 * @return std::vector<std::string> list of directories and files
 */
std::vector<std::string> CommsWrapper::listDir(const std::string &directoryPath) {
    TRACE_METHOD(getId(), directoryPath);
    std::vector<std::string> contents =
        helper::listDir(directoryPath, getId(), raceSdk.getAppConfig().baseConfigPath);
    return contents;
}

/**
 * @brief Read the contents of a file in this plugin's storage.
 * @param filename The string name of the file to be read.
 *
 * @return std::vector<uint8_t> of the file contents
 * error occurs
 */
std::vector<std::uint8_t> CommsWrapper::readFile(const std::string &filename) {
    TRACE_METHOD(getId(), filename);
    std::vector<std::uint8_t> data = helper::readFile(
        filename, getId(), raceSdk.getAppConfig().baseConfigPath, raceSdk.getPluginStorage());
    return data;
}

/**
 * @brief Append the contents of data to filename in this plugin's storage.
 * @param filname The string name of the file to be appended to (or written).
 * @param data The string of data to append to the file.
 *
 * @return SdkResponse indicator of success or failure of the append.
 */
SdkResponse CommsWrapper::appendFile(const std::string &filename,
                                     const std::vector<std::uint8_t> &data) {
    TRACE_METHOD(getId(), filename);
    if (!helper::appendFile(filename, getId(), raceSdk.getAppConfig().baseConfigPath, data,
                            raceSdk.getPluginStorage())) {
        return SDK_INVALID_ARGUMENT;
    }
    return SDK_OK;
}

/**
 * @brief Write the contents of data to filename in this plugin's storage (overwriting if file
 * exists)
 * @param filname The string name of the file to be written.
 * @param data The string of data to write to the file.
 *
 * @return SdkResponse indicator of success or failure of the write.
 */
SdkResponse CommsWrapper::writeFile(const std::string &filename,
                                    const std::vector<std::uint8_t> &data) {
    TRACE_METHOD(getId(), filename);
    if (!helper::writeFile(filename, getId(), raceSdk.getAppConfig().baseConfigPath, data,
                           raceSdk.getPluginStorage())) {
        return SDK_INVALID_ARGUMENT;
    }
    return SDK_OK;
}

SdkResponse CommsWrapper::requestPluginUserInput(const std::string &key, const std::string &prompt,
                                                 bool cache) {
    TRACE_METHOD(getId(), key, prompt, cache);
    SdkResponse response = raceSdk.requestPluginUserInput(getId(), false, key, prompt, cache);
    return response;
}

SdkResponse CommsWrapper::requestCommonUserInput(const std::string &key) {
    TRACE_METHOD(getId(), key);
    SdkResponse response = raceSdk.requestCommonUserInput(getId(), false, key);
    return response;
}

SdkResponse CommsWrapper::displayInfoToUser(const std::string &data,
                                            RaceEnums::UserDisplayType displayType) {
    TRACE_METHOD(getId(), data, displayType);
    SdkResponse response = raceSdk.displayInfoToUser(getId(), data, displayType);
    return response;
}

SdkResponse CommsWrapper::displayBootstrapInfoToUser(const std::string &data,
                                                     RaceEnums::UserDisplayType displayType,
                                                     RaceEnums::BootstrapActionType actionType) {
    TRACE_METHOD(getId(), data, displayType, actionType);
    SdkResponse response =
        raceSdk.displayBootstrapInfoToUser(getId(), data, displayType, actionType);
    return response;
}

SdkResponse CommsWrapper::unblockQueue(ConnectionID connId) {
    TRACE_METHOD(getId(), connId);
    mThreadHandler.unblock_queue(connId);
    return SDK_OK;
}

// IRaceSdkComms
SdkResponse CommsWrapper::onPackageStatusChanged(RaceHandle handle, PackageStatus status,
                                                 int32_t timeout) {
    TRACE_METHOD(getId(), handle, status);
    SdkResponse response = raceSdk.onPackageStatusChanged(*this, handle, status, timeout);
    return response;
}

SdkResponse CommsWrapper::onConnectionStatusChanged(RaceHandle handle, ConnectionID connId,
                                                    ConnectionStatus status,
                                                    LinkProperties properties, int32_t timeout) {
    TRACE_METHOD(getId(), handle, connId, status);

    if (status == CONNECTION_OPEN) {
        std::lock_guard<std::mutex> lock(mHandlePriorityTimeoutMapMutex);
        auto it = mHandlePriorityTimeoutMap.find(handle);
        if (it != mHandlePriorityTimeoutMap.end()) {
            try {
                // use priority from map
                createQueue(connId, it->second.first);

                // use sendTimeout from map
                std::lock_guard<std::mutex> connectionSendTimeoutMapLock(
                    mConnectionSendTimeoutMapMutex);
                mConnectionSendTimeoutMap[connId] = it->second.second;
            } catch (std::exception &error) {
                // There's already a queue for this connection id. This shouldn't be possible
                // because the handle shouldn't be reused.
                helper::logError("onConnectionStatusChanged: received exception opening queue: " +
                                 std::string(error.what()));
                return SDK_INVALID_ARGUMENT;
            }
            mHandlePriorityTimeoutMap.erase(it);
        } else {
            helper::logError("onConnectionStatusChanged: unexpected CONNECTION_OPEN received");
            return SDK_INVALID_ARGUMENT;
        }
    } else if (status == CONNECTION_CLOSED) {
        std::lock_guard<std::mutex> lock(mHandlePriorityTimeoutMapMutex);
        auto it = mHandlePriorityTimeoutMap.find(handle);
        if (it == mHandlePriorityTimeoutMap.end()) {
            // if the handle is not in the priority map, the connection should have been created in
            // the past. Schedule the queue to be removed.
            try {
                removeQueue(connId);
            } catch (std::exception &error) {
                helper::logWarning("onConnectionStatusChanged: received exception closing queue: " +
                                   std::string(error.what()));
            }
        } else {
            // if the handle is in the map, then the connection hasn't actually been opened. Just
            // remove from the map.
            mHandlePriorityTimeoutMap.erase(it);
        }
        mConnectionSendTimeoutMap.erase(connId);
    }

    SdkResponse response =
        raceSdk.onConnectionStatusChanged(*this, handle, connId, status, properties, timeout);
    return response;
}

SdkResponse CommsWrapper::onLinkStatusChanged(RaceHandle handle, LinkID linkId, LinkStatus status,
                                              LinkProperties properties, int32_t timeout) {
    TRACE_METHOD(getId(), handle, linkId, status);

    SdkResponse response =
        raceSdk.onLinkStatusChanged(*this, handle, linkId, status, properties, timeout);
    return response;
}

SdkResponse CommsWrapper::onChannelStatusChanged(RaceHandle handle, std::string channelGid,
                                                 ChannelStatus status, ChannelProperties properties,
                                                 int32_t timeout) {
    TRACE_METHOD(getId(), handle, channelGid, status);

    SdkResponse response =
        raceSdk.onChannelStatusChanged(*this, handle, channelGid, status, properties, timeout);
    return response;
}

SdkResponse CommsWrapper::updateLinkProperties(LinkID linkId, LinkProperties properties,
                                               int32_t timeout) {
    TRACE_METHOD(getId(), linkId);
    SdkResponse response = raceSdk.updateLinkProperties(*this, linkId, properties, timeout);
    return response;
}

ConnectionID CommsWrapper::generateConnectionId(LinkID linkId) {
    TRACE_METHOD(getId(), linkId);
    ConnectionID response = raceSdk.generateConnectionId(*this, linkId);
    return response;
}

LinkID CommsWrapper::generateLinkId(std::string channelGid) {
    TRACE_METHOD(getId(), channelGid);
    LinkID response = raceSdk.generateLinkId(*this, channelGid);
    helper::logDebug("generateLinkId: returned " + response);
    return response;
}

SdkResponse CommsWrapper::receiveEncPkg(const EncPkg &pkg, const std::vector<ConnectionID> &connIDs,
                                        int32_t timeout) {
    TRACE_METHOD(getId());
    auto ctx = spanContextFromEncryptedPackage(pkg);
    std::shared_ptr<opentracing::Span> span =
        mTracer->StartSpan("receiveEncPkg", {opentracing::ChildOf(ctx.get())});

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

    EncPkg newPkg = pkg;
    newPkg.setTraceId(traceIdFromContext(span->context()));
    newPkg.setSpanId(spanIdFromContext(span->context()));

    SdkResponse response = raceSdk.receiveEncPkg(*this, newPkg, connIDs, timeout);
    return response;
}

SdkResponse CommsWrapper::makeResponse(const std::string &functionName, bool success,
                                       size_t queueSize, RaceHandle handle) {
    double queueUtilization = (static_cast<double>(queueSize) / mThreadHandler.max_queue_size);
    SdkStatus status = SDK_OK;

    if (!success) {
        if (queueUtilization == 0) {
            helper::logWarning(functionName + " returning SDK_INVALID_ARGUMENT");
            status = SDK_INVALID_ARGUMENT;
        } else {
            helper::logWarning(functionName + " returning SDK_QUEUE_FULL");
            status = SDK_QUEUE_FULL;
        }
    }

    return SdkResponse(status, queueUtilization, handle);
}
