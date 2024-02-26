
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

#include "ComponentManager.h"

#include "helper.h"

using namespace CMTypes;

static ChannelResponse error() {
    ChannelResponse resp;
    resp.status = CM_ERROR;
    resp.handle = NULL_RACE_HANDLE;
    return resp;
}

static ChannelResponse ok(RaceHandle handle = NULL_RACE_HANDLE) {
    ChannelResponse resp;
    resp.status = CM_OK;
    resp.handle = handle;
    return resp;
}

static ChannelResponse sdkToCMResponse(SdkResponse sdkResp) {
    ChannelResponse resp;
    resp.status = (sdkResp.status == SDK_OK) ? CM_OK : CM_ERROR;
    resp.handle = sdkResp.handle;
    return resp;
}

ComponentManager::ComponentManager(
    IRaceSdkComms &sdk, const Composition &composition, IComponentPlugin &transportPlugin,
    IComponentPlugin &usermodelPlugin,
    const std::unordered_map<std::string, IComponentPlugin *> &encodingPlugins) :
    sdk(sdk),
    handler("component-mananger-thread", 1 << 20, 1 << 20),
    manager(this, sdk, composition, transportPlugin, usermodelPlugin, encodingPlugins) {
    handler.create_queue("wait queue", std::numeric_limits<int>::min());
    handler.start();
}

ComponentManager::~ComponentManager() {
    TRACE_METHOD();
    try {
        shutdown();
    } catch (std::exception &e) {
        helper::logError(logPrefix + "Threw exception: " + std::string(e.what()));
        std::terminate();
    }
}

template <typename T, typename... Args>
ChannelResponse ComponentManager::post(const std::string &logPrefix, T &&function,
                                       Args &&... args) {
    ComponentWrapperHandle postHandle{nextPostId++};
    std::string postId = std::to_string(postHandle.handle);
    helper::logDebug(logPrefix + "Posting postId: " + postId);

    try {
        auto workFunc = [logPrefix, postHandle, postId, function, this](auto &&... args) mutable {
            helper::logDebug(logPrefix + "Calling postId: " + postId);
            CmInternalStatus status;
            try {
                status = std::mem_fn(function)(manager, postHandle, std::move(args)...);
            } catch (std::exception &e) {
                helper::logError(logPrefix + "Threw exception: " + std::string(e.what()));
                status = CMTypes::FATAL;
            } catch (...) {
                helper::logError(logPrefix + "Threw unknown exception");
                status = CMTypes::FATAL;
            }

            if (status != CMTypes::OK) {
                helper::logError(logPrefix + "Returned " + cmInternalStatusToString(status) +
                                 ", postId: " + postId);
                PluginResponse errorStatus = PLUGIN_ERROR;
                if (status == CMTypes::FATAL) {
                    errorStatus = PLUGIN_FATAL;
                    // mark the manager as failed so future calls also fail
                    manager.markFailed(postHandle);
                }
                sdk.asyncError(NULL_RACE_HANDLE, errorStatus);
            }

            return std::make_optional(status);
        };
        auto [success, queueSize, future] =
            handler.post("", 0, -1, std::bind(std::move(workFunc), std::forward<Args>(args)...));

        // this really shouldn't happen...
        if (success != Handler::PostStatus::OK) {
            helper::logError(logPrefix + "Post " + postId +
                             " failed with error: " + handlerPostStatusToString(success));
            return error();
        }

        (void)queueSize;
        (void)future;
        return ok(postHandle.handle);
    } catch (std::out_of_range &e) {
        helper::logError("default queue does not exist. This should never happen. what:" +
                         std::string(e.what()));
        return error();
    }
}

template <typename T, typename... Args>
PluginResponse ComponentManager::post_sync(const std::string &logPrefix, T &&function,
                                           Args &&... args) {
    ComponentWrapperHandle postHandle{nextPostId++};
    std::string postId = std::to_string(postHandle.handle);
    helper::logDebug(logPrefix + "Posting postId: " + postId);

    try {
        auto workFunc = [logPrefix, postHandle, postId, function, this](auto &&... args) mutable {
            helper::logDebug(logPrefix + "Calling postId: " + postId);
            PluginResponse status;
            try {
                status = std::mem_fn(function)(manager, postHandle, std::move(args)...);
            } catch (std::exception &e) {
                helper::logError(logPrefix + "Threw exception: " + std::string(e.what()));
                status = PLUGIN_FATAL;
            } catch (...) {
                helper::logError(logPrefix + "Threw unknown exception");
                status = PLUGIN_FATAL;
            }

            return std::make_optional(status);
        };
        auto [success, queueSize, future] =
            handler.post("", 0, -1, std::bind(std::move(workFunc), std::forward<Args>(args)...));

        // this really shouldn't happen...
        if (success != Handler::PostStatus::OK) {
            helper::logError(logPrefix + "Post " + postId +
                             " failed with error: " + handlerPostStatusToString(success));
            return PLUGIN_ERROR;
        }

        (void)queueSize;
        future.wait();
        return future.get();
    } catch (std::out_of_range &e) {
        helper::logError("default queue does not exist. This should never happen. what:" +
                         std::string(e.what()));
        return PLUGIN_ERROR;
    }
}

PluginResponse ComponentManager::init(const PluginConfig &pluginConfig) {
    TRACE_METHOD();
    auto resp = post(logPrefix, &ComponentManagerInternal::init, pluginConfig);
    return (resp.status == CM_OK) ? PLUGIN_OK : PLUGIN_ERROR;
}

PluginResponse ComponentManager::shutdown() {
    TRACE_METHOD();
    auto resp = post_sync(logPrefix, &ComponentManagerInternal::shutdown);
    return resp;
}

PluginResponse ComponentManager::sendPackage(RaceHandle handle, ConnectionID connectionId,
                                             EncPkg pkg, double timeoutTimestamp,
                                             uint64_t batchId) {
    TRACE_METHOD();
    auto resp =
        post_sync(logPrefix, &ComponentManagerInternal::sendPackage, PackageSdkHandle{handle},
                  connectionId, std::move(pkg), timeoutTimestamp, batchId);
    return resp;
}

PluginResponse ComponentManager::openConnection(RaceHandle handle, LinkType linkType, LinkID linkId,
                                                std::string linkHints, int32_t sendTimeout) {
    TRACE_METHOD();
    auto resp = post(logPrefix, &ComponentManagerInternal::openConnection,
                     ConnectionSdkHandle{handle}, linkType, linkId, linkHints, sendTimeout);
    return (resp.status == CM_OK) ? PLUGIN_OK : PLUGIN_ERROR;
}

PluginResponse ComponentManager::closeConnection(RaceHandle handle, ConnectionID connectionId) {
    TRACE_METHOD();
    auto resp = post(logPrefix, &ComponentManagerInternal::closeConnection,
                     ConnectionSdkHandle{handle}, connectionId);
    return (resp.status == CM_OK) ? PLUGIN_OK : PLUGIN_ERROR;
}

PluginResponse ComponentManager::destroyLink(RaceHandle handle, LinkID linkId) {
    TRACE_METHOD();
    auto resp =
        post(logPrefix, &ComponentManagerInternal::destroyLink, LinkSdkHandle{handle}, linkId);
    return (resp.status == CM_OK) ? PLUGIN_OK : PLUGIN_ERROR;
}

PluginResponse ComponentManager::createLink(RaceHandle handle, std::string channelGid) {
    TRACE_METHOD();
    auto resp =
        post(logPrefix, &ComponentManagerInternal::createLink, LinkSdkHandle{handle}, channelGid);
    return (resp.status == CM_OK) ? PLUGIN_OK : PLUGIN_ERROR;
}

PluginResponse ComponentManager::loadLinkAddress(RaceHandle handle, std::string channelGid,
                                                 std::string linkAddress) {
    TRACE_METHOD();
    auto resp = post(logPrefix, &ComponentManagerInternal::loadLinkAddress, LinkSdkHandle{handle},
                     channelGid, linkAddress);
    return (resp.status == CM_OK) ? PLUGIN_OK : PLUGIN_ERROR;
}

PluginResponse ComponentManager::loadLinkAddresses(RaceHandle handle, std::string channelGid,
                                                   std::vector<std::string> linkAddresses) {
    TRACE_METHOD();
    auto resp = post(logPrefix, &ComponentManagerInternal::loadLinkAddresses, LinkSdkHandle{handle},
                     channelGid, linkAddresses);
    return (resp.status == CM_OK) ? PLUGIN_OK : PLUGIN_ERROR;
}

PluginResponse ComponentManager::createLinkFromAddress(RaceHandle handle, std::string channelGid,
                                                       std::string linkAddress) {
    TRACE_METHOD();
    auto resp = post(logPrefix, &ComponentManagerInternal::createLinkFromAddress,
                     LinkSdkHandle{handle}, channelGid, linkAddress);
    return (resp.status == CM_OK) ? PLUGIN_OK : PLUGIN_ERROR;
}

PluginResponse ComponentManager::deactivateChannel(RaceHandle handle, std::string channelGid) {
    TRACE_METHOD();
    auto resp = post(logPrefix, &ComponentManagerInternal::deactivateChannel,
                     ChannelSdkHandle{handle}, channelGid);
    return (resp.status == CM_OK) ? PLUGIN_OK : PLUGIN_ERROR;
}

PluginResponse ComponentManager::activateChannel(RaceHandle handle, std::string channelGid,
                                                 std::string roleName) {
    TRACE_METHOD();
    auto resp = post(logPrefix, &ComponentManagerInternal::activateChannel,
                     ChannelSdkHandle{handle}, channelGid, roleName);
    return (resp.status == CM_OK) ? PLUGIN_OK : PLUGIN_ERROR;
}

PluginResponse ComponentManager::onUserInputReceived(RaceHandle handle, bool answered,
                                                     const std::string &response) {
    TRACE_METHOD();
    auto resp = post(logPrefix, &ComponentManagerInternal::onUserInputReceived,
                     UserSdkHandle{handle}, answered, response);
    return (resp.status == CM_OK) ? PLUGIN_OK : PLUGIN_ERROR;
}

PluginResponse ComponentManager::onUserAcknowledgementReceived(RaceHandle handle) {
    TRACE_METHOD();
    auto resp = post(logPrefix, &ComponentManagerInternal::onUserAcknowledgementReceived,
                     UserSdkHandle{handle});
    return (resp.status == CM_OK) ? PLUGIN_OK : PLUGIN_ERROR;
}

// Common Apis
ChannelResponse ComponentManager::requestPluginUserInput(std::string componentId,
                                                         const std::string &key,
                                                         const std::string &prompt, bool cache) {
    TRACE_METHOD();
    return post(logPrefix, &ComponentManagerInternal::requestPluginUserInput, componentId, key,
                prompt, cache);
}
ChannelResponse ComponentManager::requestCommonUserInput(std::string componentId,
                                                         const std::string &key) {
    TRACE_METHOD();
    return post(logPrefix, &ComponentManagerInternal::requestCommonUserInput, componentId, key);
}
ChannelResponse ComponentManager::updateState(std::string componentId, ComponentState state) {
    TRACE_METHOD();
    return post(logPrefix, &ComponentManagerInternal::updateState, componentId, state);
}

// IEncodingSdk APIs
ChannelResponse ComponentManager::onBytesEncoded(RaceHandle handle,
                                                 const std::vector<uint8_t> &bytes,
                                                 EncodingStatus status) {
    TRACE_METHOD(handle, bytes.size(), status);
    return post(logPrefix, &ComponentManagerInternal::onBytesEncoded, EncodingHandle{handle}, bytes,
                status);
}

ChannelResponse ComponentManager::onBytesDecoded(RaceHandle handle,
                                                 const std::vector<uint8_t> &bytes,
                                                 EncodingStatus status) {
    TRACE_METHOD(handle, bytes.size(), status);
    return post(logPrefix, &ComponentManagerInternal::onBytesDecoded, DecodingHandle{handle}, bytes,
                status);
}

// ITransportSdk APIs
ChannelResponse ComponentManager::onLinkStatusChanged(RaceHandle handle, const LinkID &linkId,
                                                      LinkStatus status,
                                                      const LinkParameters &params) {
    TRACE_METHOD(handle, linkId, status, params);
    return post(logPrefix, &ComponentManagerInternal::onLinkStatusChanged, LinkSdkHandle{handle},
                linkId, status, params);
}

ChannelResponse ComponentManager::onPackageStatusChanged(RaceHandle handle, PackageStatus status) {
    TRACE_METHOD(handle, status);
    return post(logPrefix, &ComponentManagerInternal::onPackageStatusChanged,
                PackageFragmentHandle{handle}, status);
}

ChannelResponse ComponentManager::onEvent(const Event &event) {
    TRACE_METHOD(event);
    return post(logPrefix, &ComponentManagerInternal::onEvent, event);
}

ChannelResponse ComponentManager::onReceive(const LinkID &linkId, const EncodingParameters &params,
                                            const std::vector<uint8_t> &bytes) {
    TRACE_METHOD(linkId, params, bytes.size());
    return post(logPrefix, &ComponentManagerInternal::onReceive, linkId, params, bytes);
}

// IUserModelSdk APIs
ChannelResponse ComponentManager::onTimelineUpdated() {
    TRACE_METHOD();
    return post(logPrefix, &ComponentManagerInternal::onTimelineUpdated);
}

// SDK interaction
std::string ComponentManager::getActivePersona() {
    TRACE_METHOD();
    return manager.sdk.getActivePersona();
}

ChannelResponse ComponentManager::makeDir(const std::string &directoryPath) {
    TRACE_METHOD(directoryPath);
    return sdkToCMResponse(manager.sdk.makeDir(directoryPath));
}

ChannelResponse ComponentManager::removeDir(const std::string &directoryPath) {
    TRACE_METHOD(directoryPath);
    return sdkToCMResponse(manager.sdk.removeDir(directoryPath));
}

std::vector<std::string> ComponentManager::listDir(const std::string &directoryPath) {
    TRACE_METHOD(directoryPath);
    return manager.sdk.listDir(directoryPath);
}

ChannelResponse ComponentManager::appendFile(const std::string &filepath,
                                             const std::vector<std::uint8_t> &data) {
    TRACE_METHOD(filepath, data.size());
    return sdkToCMResponse(manager.sdk.appendFile(filepath, data));
}

ChannelResponse ComponentManager::writeFile(const std::string &filepath,
                                            const std::vector<std::uint8_t> &data) {
    TRACE_METHOD(filepath, data.size());
    return sdkToCMResponse(manager.sdk.writeFile(filepath, data));
}

std::vector<std::uint8_t> ComponentManager::readFile(const std::string &filepath) {
    TRACE_METHOD(filepath);
    return manager.sdk.readFile(filepath);
}

ChannelProperties ComponentManager::getChannelProperties() {
    TRACE_METHOD();
    return manager.sdk.getChannelProperties(manager.lifetimeManager.composition.id);
}

void ComponentManager::waitForCallbacks() {
    TRACE_METHOD();
    post(logPrefix, &ComponentManagerInternal::waitForCallbacks);

    auto [success, queueSize, future] =
        handler.post("wait queue", 0, -1, [=] { return std::make_optional(true); });
    (void)success;
    (void)queueSize;
    future.wait();
}

void ComponentManager::markFailed() {
    TRACE_METHOD();
    post(logPrefix, &ComponentManagerInternal::markFailed);
}

std::string ComponentManager::toString() const {
    return "ComponentManager{ " + manager.toString() + "}";
}

#define EXPECT_STATE(expected_state, ...)                               \
    if ((lifetimeManager.state & expected_state) == 0) {                \
        helper::logError(logPrefix + "Failed due to unexpected state"); \
        return __VA_ARGS__;                                             \
    }

#define EXPECT_CHANNEL_MATCHES(inChannelId, ...)                         \
    if (inChannelId != this->lifetimeManager.composition.id) {           \
        helper::logError(logPrefix + "Failed due to invalid channelId"); \
        return __VA_ARGS__;                                              \
    }

ComponentManagerInternal::ComponentManagerInternal(
    ComponentManager *manager, IRaceSdkComms &sdk, const Composition &composition,
    IComponentPlugin &transportPlugin, IComponentPlugin &usermodelPlugin,
    const std::unordered_map<std::string, IComponentPlugin *> &encodingPlugins) :
    manager(manager),
    sdk(sdk),
    actionManager(*this),
    connectionManager(*this),
    lifetimeManager(*this, composition, transportPlugin, usermodelPlugin, encodingPlugins),
    linkManager(*this),
    packageManager(*this),
    receiveManager(*this) {
    TRACE_METHOD(composition.description());
}

ComponentManagerInternal::~ComponentManagerInternal() {
    TRACE_METHOD();
}

Link *ComponentManagerInternal::getLink(const LinkID &linkId) {
    return linkManager.links.at(linkId).get();
}

std::vector<Link *> ComponentManagerInternal::getLinks() {
    std::vector<Link *> links(linkManager.links.size());
    std::transform(linkManager.links.begin(), linkManager.links.end(), links.begin(),
                   [](auto &pair) { return pair.second.get(); });
    return links;
}

Connection *ComponentManagerInternal::getConnection(const ConnectionID &connId) {
    return connectionManager.connections.at(connId).get();
}

CmInternalStatus ComponentManagerInternal::init(ComponentWrapperHandle postId,
                                                const PluginConfig &argPluginConfig) {
    TRACE_METHOD(postId);
    EXPECT_STATE(INITIALIZING, FATAL);
    pluginConfig = argPluginConfig;
    lifetimeManager.init(postId, argPluginConfig);
    return OK;
}

PluginResponse ComponentManagerInternal::shutdown(ComponentWrapperHandle postId) {
    TRACE_METHOD(postId);
    lifetimeManager.shutdown(postId);
    return PLUGIN_OK;
}

PluginResponse ComponentManagerInternal::sendPackage(ComponentWrapperHandle postId,
                                                     PackageSdkHandle handle,
                                                     const ConnectionID &connId, EncPkg &&pkg,
                                                     double timeoutTimestamp, uint64_t batchId) {
    TRACE_METHOD(postId, handle, connId, pkg.getSize(), timeoutTimestamp, batchId);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, PLUGIN_ERROR);
    auto now = helper::currentTime();

    // call action manager to inform user model and update timeline if necessary
    actionManager.onSendPackage(now, connId, pkg);

    // call package manager to enqueue package
    PluginResponse response = packageManager.sendPackage(postId, now, handle, connId,
                                                         std::move(pkg), timeoutTimestamp, batchId);

    return response;
}

CmInternalStatus ComponentManagerInternal::openConnection(ComponentWrapperHandle postId,
                                                          ConnectionSdkHandle handle,
                                                          LinkType linkType, const LinkID &linkId,
                                                          const std::string &linkHints,
                                                          int32_t sendTimeout) {
    // TODO: link hints, link type(?)
    TRACE_METHOD(postId, handle, linkType, linkId, linkHints, sendTimeout);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    return connectionManager.openConnection(postId, handle, linkType, linkId, linkHints,
                                            sendTimeout);
}

CmInternalStatus ComponentManagerInternal::closeConnection(ComponentWrapperHandle postId,
                                                           ConnectionSdkHandle handle,
                                                           const ConnectionID &connId) {
    TRACE_METHOD(postId, handle, connId);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    return connectionManager.closeConnection(postId, handle, connId);
}

CmInternalStatus ComponentManagerInternal::createLink(ComponentWrapperHandle postId,
                                                      LinkSdkHandle handle,
                                                      const std::string &inChannelGid) {
    TRACE_METHOD(postId, handle, inChannelGid);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    EXPECT_CHANNEL_MATCHES(inChannelGid, ERROR);
    return linkManager.createLink(postId, handle, inChannelGid);
}

CmInternalStatus ComponentManagerInternal::loadLinkAddress(ComponentWrapperHandle postId,
                                                           LinkSdkHandle handle,
                                                           const std::string &inChannelGid,
                                                           const std::string &linkAddress) {
    TRACE_METHOD(postId, handle, inChannelGid, linkAddress);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    EXPECT_CHANNEL_MATCHES(inChannelGid, ERROR);
    return linkManager.loadLinkAddress(postId, handle, inChannelGid, linkAddress);
}

CmInternalStatus ComponentManagerInternal::loadLinkAddresses(
    ComponentWrapperHandle postId, LinkSdkHandle handle, const std::string &inChannelGid,
    const std::vector<std::string> &linkAddresses) {
    TRACE_METHOD(postId, handle, inChannelGid);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    EXPECT_CHANNEL_MATCHES(inChannelGid, ERROR);
    return linkManager.loadLinkAddresses(postId, handle, inChannelGid, linkAddresses);
}

CmInternalStatus ComponentManagerInternal::createLinkFromAddress(ComponentWrapperHandle postId,
                                                                 LinkSdkHandle handle,
                                                                 const std::string &inChannelGid,
                                                                 const std::string &linkAddress) {
    TRACE_METHOD(postId, handle, inChannelGid, linkAddress);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    EXPECT_CHANNEL_MATCHES(inChannelGid, ERROR);
    return linkManager.createLinkFromAddress(postId, handle, inChannelGid, linkAddress);
}

CmInternalStatus ComponentManagerInternal::destroyLink(ComponentWrapperHandle postId,
                                                       LinkSdkHandle handle, const LinkID &linkId) {
    TRACE_METHOD(postId, handle, linkId);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    return linkManager.destroyLink(postId, handle, linkId);
}

CmInternalStatus ComponentManagerInternal::deactivateChannel(ComponentWrapperHandle postId,
                                                             ChannelSdkHandle handle,
                                                             const std::string &inChannelGid) {
    TRACE_METHOD(postId, handle, inChannelGid);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(WAITING_FOR_COMPONENTS | ACTIVATED, ERROR);
    EXPECT_CHANNEL_MATCHES(inChannelGid, ERROR);

    lifetimeManager.deactivateChannel(postId, handle, inChannelGid);

    lock.unlock();
    actionManager.joinActionThread();
    return OK;
}

CmInternalStatus ComponentManagerInternal::activateChannel(ComponentWrapperHandle postId,
                                                           ChannelSdkHandle handle,
                                                           const std::string &inChannelGid,
                                                           const std::string &roleName) {
    TRACE_METHOD(postId, handle, inChannelGid, roleName);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(UNACTIVATED, ERROR);
    EXPECT_CHANNEL_MATCHES(inChannelGid, ERROR);
    return lifetimeManager.activateChannel(postId, handle, inChannelGid, roleName);
}

CmInternalStatus ComponentManagerInternal::onUserInputReceived(ComponentWrapperHandle postId,
                                                               UserSdkHandle handle, bool answered,
                                                               const std::string &response) {
    TRACE_METHOD(postId, handle, answered, response);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE((CREATING_COMPONENTS | WAITING_FOR_COMPONENTS | ACTIVATED), ERROR);
    auto it = userInputMap.find(handle);
    if (it != userInputMap.end()) {
        auto [componentHandle, componentId] = it->second;
        userInputMap.erase(it);
        try {
            lifetimeManager.idComponentMap.at(componentId)
                ->onUserInputReceived(componentHandle, answered, response);
            return OK;
        } catch (std::exception &e) {
            helper::logError(logPrefix + "Could not find component with id: " + componentId);
            return ERROR;
        }
    }

    helper::logError(logPrefix + "No mapping found for handle: " + handle.to_string());
    return ERROR;
}

CmInternalStatus ComponentManagerInternal::onUserAcknowledgementReceived(
    ComponentWrapperHandle postId, UserSdkHandle handle) {
    TRACE_METHOD(postId, handle);
    return OK;
}

CmInternalStatus ComponentManagerInternal::requestPluginUserInput(ComponentWrapperHandle postId,
                                                                  const std::string &componentId,
                                                                  const std::string &key,
                                                                  const std::string &prompt,
                                                                  bool cache) {
    TRACE_METHOD(postId, key, prompt, cache);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE((CREATING_COMPONENTS | WAITING_FOR_COMPONENTS | ACTIVATED), ERROR);
    auto sdkResp = sdk.requestPluginUserInput(key, prompt, cache);
    userInputMap[{sdkResp.handle}] = {UserComponentHandle{postId.handle}, componentId};
    return OK;
}

CmInternalStatus ComponentManagerInternal::requestCommonUserInput(ComponentWrapperHandle postId,
                                                                  const std::string &componentId,
                                                                  const std::string &key) {
    TRACE_METHOD(postId, key);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE((CREATING_COMPONENTS | WAITING_FOR_COMPONENTS | ACTIVATED), ERROR);
    auto sdkResp = sdk.requestCommonUserInput(key);
    userInputMap[{sdkResp.handle}] = {UserComponentHandle{postId.handle}, componentId};
    return OK;
}

CmInternalStatus ComponentManagerInternal::updateState(ComponentWrapperHandle postId,
                                                       const std::string &componentId,
                                                       ComponentState updatedState) {
    TRACE_METHOD(postId, updatedState);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE((CREATING_COMPONENTS | WAITING_FOR_COMPONENTS | ACTIVATED), ERROR);
    return lifetimeManager.updateState(postId, componentId, updatedState);
}

CmInternalStatus ComponentManagerInternal::onBytesEncoded(ComponentWrapperHandle postId,
                                                          EncodingHandle handle,
                                                          std::vector<uint8_t> &&bytes,
                                                          EncodingStatus status) {
    TRACE_METHOD(postId, handle, bytes.size(), status);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    return packageManager.onBytesEncoded(postId, handle, std::move(bytes), status);
}

CmInternalStatus ComponentManagerInternal::onBytesDecoded(ComponentWrapperHandle postId,
                                                          DecodingHandle handle,
                                                          std::vector<uint8_t> &&bytes,
                                                          EncodingStatus status) {
    TRACE_METHOD(postId, handle, bytes.size(), status);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    return receiveManager.onBytesDecoded(postId, handle, std::move(bytes), status);
}

CmInternalStatus ComponentManagerInternal::onLinkStatusChanged(ComponentWrapperHandle postId,
                                                               LinkSdkHandle handle,
                                                               const LinkID &linkId,
                                                               LinkStatus status,
                                                               const LinkParameters &params) {
    TRACE_METHOD(postId, handle, linkId, status);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    packageManager.onLinkStatusChanged(postId, handle, linkId, status, params);
    actionManager.onLinkStatusChanged(postId, handle, linkId, status, params);
    return linkManager.onLinkStatusChanged(postId, handle, linkId, status, params);
}

CmInternalStatus ComponentManagerInternal::onPackageStatusChanged(ComponentWrapperHandle postId,
                                                                  PackageFragmentHandle handle,
                                                                  PackageStatus status) {
    TRACE_METHOD(postId, handle, status);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    return packageManager.onPackageStatusChanged(postId, handle, status);
}

CmInternalStatus ComponentManagerInternal::onEvent(ComponentWrapperHandle postId,
                                                   const Event &event) {
    TRACE_METHOD(postId);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    lifetimeManager.usermodel->onTransportEvent(event);
    return OK;
}

CmInternalStatus ComponentManagerInternal::onReceive(ComponentWrapperHandle postId,
                                                     const LinkID &linkId,
                                                     const EncodingParameters &params,
                                                     std::vector<uint8_t> &&bytes) {
    TRACE_METHOD(postId, linkId, bytes.size());
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    return receiveManager.onReceive(postId, linkId, params, std::move(bytes));
}

CmInternalStatus ComponentManagerInternal::onTimelineUpdated(ComponentWrapperHandle postId) {
    TRACE_METHOD(postId);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    EXPECT_STATE(ACTIVATED, ERROR);
    return actionManager.onTimelineUpdated(postId);
}

CmInternalStatus ComponentManagerInternal::markFailed(ComponentWrapperHandle postId) {
    TRACE_METHOD(postId);
    std::unique_lock<std::recursive_mutex> lock(dataMutex);
    lifetimeManager.fail(postId);
    return OK;
}

const std::string &ComponentManagerInternal::getCompositionId() {
    return lifetimeManager.composition.id;
}

CMTypes::State ComponentManagerInternal::getState() {
    return lifetimeManager.state;
}

EncodingComponentWrapper *ComponentManagerInternal::encodingComponentFromEncodingParams(
    const EncodingParameters &params) {
    TRACE_METHOD();
    return lifetimeManager.encodingComponentFromEncodingParams(params);
}

TransportComponentWrapper *ComponentManagerInternal::getTransport() {
    return &*lifetimeManager.transport;
}

UserModelComponentWrapper *ComponentManagerInternal::getUserModel() {
    return &*lifetimeManager.usermodel;
}

double ComponentManagerInternal::getMaxEncodingTime() {
    return actionManager.getMaxEncodingTime();
}

void ComponentManagerInternal::updatedActions() {
    packageManager.updatedActions();
}

void ComponentManagerInternal::encodeForAction(ActionInfo *info) {
    packageManager.encodeForAction(info);
}

std::vector<PackageFragmentHandle> ComponentManagerInternal::getPackageHandlesForAction(
    ActionInfo *info) {
    return packageManager.getPackageHandlesForAction(info);
}

void ComponentManagerInternal::actionDone(ActionInfo *info) {
    packageManager.actionDone(info);
}

void ComponentManagerInternal::teardown() {
    TRACE_METHOD();

    actionManager.teardown();
    connectionManager.teardown();
    lifetimeManager.teardown();
    linkManager.teardown();
    packageManager.teardown();
    receiveManager.teardown();

    userInputMap.clear();
}

void ComponentManagerInternal::setup() {
    TRACE_METHOD();

    channelProps = sdk.getChannelProperties(lifetimeManager.composition.id);

    lifetimeManager.setup();
    actionManager.setup();
    connectionManager.setup();
    linkManager.setup();
    packageManager.setup();
    receiveManager.setup();
}

CmInternalStatus ComponentManagerInternal::waitForCallbacks(ComponentWrapperHandle postId) {
    TRACE_METHOD(postId);
    if (lifetimeManager.transport) {
        lifetimeManager.transport->waitForCallbacks();
    }
    if (lifetimeManager.usermodel) {
        lifetimeManager.usermodel->waitForCallbacks();
    }
    for (auto &encoding : lifetimeManager.encodings) {
        encoding.second.waitForCallbacks();
    }
    return OK;
}

template <typename First, typename... Rest>
void printAll(std::ostream &out, const First &first, const Rest &... rest) {
    out << first;
    if constexpr (sizeof...(rest) > 0) {
        out << ", ";
        printAll(out, rest...);
    }
}

template <typename... A, size_t... I>
void tuplePrint(std::ostream &out, const std::tuple<A...> &tup, std::index_sequence<I...>) {
    printAll(out, std::get<I>(tup)...);
}

template <typename... A>
std::ostream &operator<<(std::ostream &out, const std::tuple<A...> &tup) {
    static constexpr auto size = std::tuple_size<std::tuple<A...>>::value;
    out << "Tuple<";
    tuplePrint(out, tup, std::make_index_sequence<size>{});
    return out << ">";
}

template <typename A, typename B>
std::ostream &operator<<(std::ostream &out, const std::pair<A, B> &pair) {
    return out << "Pair<" << pair.first << ", " << pair.second << ">";
}

template <typename A>
std::ostream &operator<<(std::ostream &out, const std::unique_ptr<A> &ptr) {
    if (ptr) {
        return out << *ptr;
    } else {
        return out << "nullptr";
    }
}

template <class C>
std::string listString(const C &container) {
    std::stringstream ss;
    ss << "[";
    for (auto &e : container) {
        ss << e << ", ";
    }
    ss << "]";
    return ss.str();
}

template <class T>
std::string listString(const std::vector<std::unique_ptr<T>> &container) {
    std::stringstream ss;
    ss << "[";
    for (auto &e : container) {
        ss << *e << ", ";
    }
    ss << "]";
    return ss.str();
}

template <class C>
std::string mapString(const C &container) {
    // use map for ordering
    std::map<std::string, std::string> kvs;
    for (auto &[k, v] : container) {
        std::stringstream ss;
        ss << k;
        std::stringstream ss2;
        ss2 << v;
        kvs[ss.str()] = ss2.str();
    }

    std::stringstream ss;
    ss << "{";
    for (auto &[k, v] : kvs) {
        ss << k << ": " << v << ", ";
    }
    ss << "}";
    return ss.str();
}

template <class K, class V>
std::string mapString(const std::unordered_map<K, std::unique_ptr<V>> &container) {
    // use map for ordering
    std::map<std::string, V *> kvs;
    for (auto &[k, v] : container) {
        kvs[k] = v.get();
    }

    std::stringstream ss;
    ss << "{";
    for (auto &[k, v] : kvs) {
        ss << k << ": " << *v << ", ";
    }
    ss << "}";
    return ss.str();
}

std::ostream &operator<<(std::ostream &out, IComponentBase *component) {
    if (dynamic_cast<ITransportComponent *>(component)) {
        out << "<Transport Component>";
    } else if (dynamic_cast<IUserModelComponent *>(component)) {
        out << "<User Model Component>";
    } else if (dynamic_cast<IEncodingComponent *>(component)) {
        out << "<Encoding Component>";
    } else if (component == nullptr) {
        out << "nullptr";
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const std::shared_ptr<IComponentBase> &component) {
    return out << component.get();
}

template <typename T>
std::ostream &operator<<(std::ostream &out, const std::optional<T> &option) {
    if (option) {
        return out << *option;
    } else {
        return out << "nullopt";
    }
}

std::string ComponentManagerInternal::toString() const {
    std::stringstream ss;
    ss << "ComponentManagerInternal {\n";
    ss << "\t state: " << lifetimeManager.state << "\n";
    ss << "\t composition: " << lifetimeManager.composition.description() << "\n";
    ss << "\t encodings: " << mapString(lifetimeManager.encodings) << "\n";
    ss << "\t transport: " << lifetimeManager.transport << "\n";
    ss << "\t usermodel: " << lifetimeManager.usermodel << "\n";
    ss << "\t wrappers: " << listString(lifetimeManager.wrappers) << "\n";
    ss << "\t componentStates: " << mapString(lifetimeManager.componentStates) << "\n";
    ss << "\t activateHandle: " << lifetimeManager.activateHandle << "\n";
    ss << "\t pendingEncodings: " << mapString(packageManager.pendingEncodings) << "\n";
    ss << "\t actions: " << listString(actionManager.actions) << "\n";
    ss << "\t links: " << mapString(linkManager.links) << "\n";
    ss << "\t connections: " << mapString(connectionManager.connections) << "\n";
    ss << "\t userInputMap: " << mapString(userInputMap) << "\n";
    ss << "}";
    return ss.str();
}

std::ostream &operator<<(std::ostream &out, const ComponentManagerInternal &manager) {
    return out << manager.toString();
}

std::ostream &operator<<(std::ostream &out, const ComponentManager &manager) {
    return out << manager.toString();
}
