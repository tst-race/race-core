
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

#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <set>
#include <thread>

#include "ComponentActionManager.h"
#include "ComponentConnectionManager.h"
#include "ComponentLifetimeManager.h"
#include "ComponentLinkManager.h"
#include "ComponentManagerTypes.h"
#include "ComponentPackageManager.h"
#include "ComponentReceivePackageManager.h"
#include "ComponentWrappers.h"
#include "Composition.h"
#include "IRacePluginComms.h"
#include "SdkWrappers.h"
#include "plugin-loading/ComponentPlugin.h"
#include "plugin-loading/IComponentPlugin.h"

class ComponentManager;

class ComponentManagerInternal {
public:
    explicit ComponentManagerInternal(
        ComponentManager *manager, IRaceSdkComms &sdk, const Composition &composition,
        IComponentPlugin &transportPlugin, IComponentPlugin &usermodelPlugin,
        const std::unordered_map<std::string, IComponentPlugin *> &encodingPlugins);
    virtual ~ComponentManagerInternal();

    // Comms Plugin APIs
    virtual CMTypes::CmInternalStatus init(CMTypes::ComponentWrapperHandle postId,
                                           const PluginConfig &pluginConfig);

    virtual PluginResponse shutdown(CMTypes::ComponentWrapperHandle postId);

    virtual PluginResponse sendPackage(CMTypes::ComponentWrapperHandle postId,
                                       CMTypes::PackageSdkHandle handle,
                                       const ConnectionID &connectionId, EncPkg &&pkg,
                                       double timeoutTimestamp, uint64_t batchId);

    virtual CMTypes::CmInternalStatus openConnection(CMTypes::ComponentWrapperHandle postId,
                                                     CMTypes::ConnectionSdkHandle handle,
                                                     LinkType linkType, const LinkID &linkId,
                                                     const std::string &linkHints,
                                                     int32_t sendTimeout);

    virtual CMTypes::CmInternalStatus closeConnection(CMTypes::ComponentWrapperHandle postId,
                                                      CMTypes::ConnectionSdkHandle handle,
                                                      const ConnectionID &connectionId);

    virtual CMTypes::CmInternalStatus destroyLink(CMTypes::ComponentWrapperHandle postId,
                                                  CMTypes::LinkSdkHandle handle,
                                                  const LinkID &linkId);

    virtual CMTypes::CmInternalStatus createLink(CMTypes::ComponentWrapperHandle postId,
                                                 CMTypes::LinkSdkHandle handle,
                                                 const std::string &channelGid);

    virtual CMTypes::CmInternalStatus loadLinkAddress(CMTypes::ComponentWrapperHandle postId,
                                                      CMTypes::LinkSdkHandle handle,
                                                      const std::string &channelGid,
                                                      const std::string &linkAddress);

    virtual CMTypes::CmInternalStatus loadLinkAddresses(
        CMTypes::ComponentWrapperHandle postId, CMTypes::LinkSdkHandle handle,
        const std::string &channelGid, const std::vector<std::string> &linkAddresses);

    virtual CMTypes::CmInternalStatus createLinkFromAddress(CMTypes::ComponentWrapperHandle postId,
                                                            CMTypes::LinkSdkHandle handle,
                                                            const std::string &channelGid,
                                                            const std::string &linkAddress);

    virtual CMTypes::CmInternalStatus deactivateChannel(CMTypes::ComponentWrapperHandle postId,
                                                        CMTypes::ChannelSdkHandle handle,
                                                        const std::string &channelGid);

    virtual CMTypes::CmInternalStatus activateChannel(CMTypes::ComponentWrapperHandle postId,
                                                      CMTypes::ChannelSdkHandle handle,
                                                      const std::string &channelGid,
                                                      const std::string &roleName);

    virtual CMTypes::CmInternalStatus onUserInputReceived(CMTypes::ComponentWrapperHandle postId,
                                                          CMTypes::UserSdkHandle handle,
                                                          bool answered,
                                                          const std::string &response);

    virtual CMTypes::CmInternalStatus onUserAcknowledgementReceived(
        CMTypes::ComponentWrapperHandle postId, CMTypes::UserSdkHandle handle);

    // Common Apis
    virtual CMTypes::CmInternalStatus requestPluginUserInput(CMTypes::ComponentWrapperHandle postId,
                                                             const std::string &componentId,
                                                             const std::string &key,
                                                             const std::string &prompt, bool cache);
    virtual CMTypes::CmInternalStatus requestCommonUserInput(CMTypes::ComponentWrapperHandle postId,
                                                             const std::string &componentId,
                                                             const std::string &key);
    virtual CMTypes::CmInternalStatus updateState(CMTypes::ComponentWrapperHandle postId,
                                                  const std::string &componentId,
                                                  ComponentState state);

    // IEncodingSdk APIs
    virtual CMTypes::CmInternalStatus onBytesEncoded(CMTypes::ComponentWrapperHandle postId,
                                                     CMTypes::EncodingHandle handle,
                                                     std::vector<uint8_t> &&bytes,
                                                     EncodingStatus status);

    virtual CMTypes::CmInternalStatus onBytesDecoded(CMTypes::ComponentWrapperHandle postId,
                                                     CMTypes::DecodingHandle handle,
                                                     std::vector<uint8_t> &&bytes,
                                                     EncodingStatus status);

    // ITransportSdk APIs
    virtual CMTypes::CmInternalStatus onLinkStatusChanged(CMTypes::ComponentWrapperHandle postId,
                                                          CMTypes::LinkSdkHandle handle,
                                                          const LinkID &linkId, LinkStatus status,
                                                          const LinkParameters &params);

    virtual CMTypes::CmInternalStatus onPackageStatusChanged(CMTypes::ComponentWrapperHandle postId,
                                                             CMTypes::PackageFragmentHandle handle,
                                                             PackageStatus status);

    virtual CMTypes::CmInternalStatus onEvent(CMTypes::ComponentWrapperHandle postId,
                                              const Event &event);

    virtual CMTypes::CmInternalStatus onReceive(CMTypes::ComponentWrapperHandle postId,
                                                const LinkID &linkId,
                                                const EncodingParameters &params,
                                                std::vector<uint8_t> &&bytes);

    // IUserModelSdk APIs
    virtual CMTypes::CmInternalStatus onTimelineUpdated(CMTypes::ComponentWrapperHandle postId);

    // other
    CMTypes::CmInternalStatus markFailed(CMTypes::ComponentWrapperHandle postId);

    // methods for sub-managers
    virtual void teardown();
    virtual void setup();

    virtual CMTypes::State getState();
    virtual const std::string &getCompositionId();

    virtual EncodingComponentWrapper *encodingComponentFromEncodingParams(
        const EncodingParameters &params);
    virtual TransportComponentWrapper *getTransport();
    virtual UserModelComponentWrapper *getUserModel();

    virtual CMTypes::Link *getLink(const LinkID &linkId);
    virtual std::vector<CMTypes::Link *> getLinks();
    virtual CMTypes::Connection *getConnection(const ConnectionID &connId);

    virtual double getMaxEncodingTime();
    virtual void updatedActions();
    virtual void encodeForAction(CMTypes::ActionInfo *info);
    virtual std::vector<CMTypes::PackageFragmentHandle> getPackageHandlesForAction(
        CMTypes::ActionInfo *info);
    virtual void actionDone(CMTypes::ActionInfo *info);

    // Testing
    std::string toString() const;
    CMTypes::CmInternalStatus waitForCallbacks(CMTypes::ComponentWrapperHandle postId);

public:
    ComponentManager *manager;
    IRaceSdkComms &sdk;

    ComponentActionManager actionManager;
    ComponentConnectionManager connectionManager;
    ComponentLifetimeManager lifetimeManager;
    ComponentLinkManager linkManager;
    ComponentPackageManager packageManager;
    ComponentReceivePackageManager receiveManager;

    PluginConfig pluginConfig;
    ChannelProperties channelProps;

    std::unordered_map<CMTypes::UserSdkHandle, std::pair<CMTypes::UserComponentHandle, std::string>>
        userInputMap;

    // This should be locked while in any CMInternal function
    // TODO: currently, the action thread in action manager does stuff with member variables. If
    // instead it posted to the CMInternal thread, this mutex wouldn't be necessary.
    std::recursive_mutex dataMutex;

    CMTypes::EncodingMode mode = CMTypes::EncodingMode::FRAGMENT_SINGLE_PRODUCER;
};

class ComponentManager : public IRacePluginComms {
public:
    explicit ComponentManager(
        IRaceSdkComms &sdk, const Composition &composition, IComponentPlugin &transportPlugin,
        IComponentPlugin &usermodelPlugin,
        const std::unordered_map<std::string, IComponentPlugin *> &encodingPlugins);
    virtual ~ComponentManager();

    // Comms Plugin APIs
    virtual PluginResponse init(const PluginConfig &pluginConfig) override;

    virtual PluginResponse shutdown() override;

    virtual PluginResponse sendPackage(RaceHandle handle, ConnectionID connectionId, EncPkg pkg,
                                       double timeoutTimestamp, uint64_t batchId) override;

    virtual PluginResponse openConnection(RaceHandle handle, LinkType linkType, LinkID linkId,
                                          std::string linkHints, int32_t sendTimeout) override;

    virtual PluginResponse closeConnection(RaceHandle handle, ConnectionID connectionId) override;

    virtual PluginResponse destroyLink(RaceHandle handle, LinkID linkId) override;

    virtual PluginResponse createLink(RaceHandle handle, std::string channelGid) override;

    virtual PluginResponse loadLinkAddress(RaceHandle handle, std::string channelGid,
                                           std::string linkAddress) override;

    virtual PluginResponse loadLinkAddresses(RaceHandle handle, std::string channelGid,
                                             std::vector<std::string> linkAddresses) override;

    virtual PluginResponse createLinkFromAddress(RaceHandle handle, std::string channelGid,
                                                 std::string linkAddress) override;

    virtual PluginResponse deactivateChannel(RaceHandle handle, std::string channelGid) override;

    virtual PluginResponse activateChannel(RaceHandle handle, std::string channelGid,
                                           std::string roleName) override;

    virtual PluginResponse onUserInputReceived(RaceHandle handle, bool answered,
                                               const std::string &response) override;

    virtual PluginResponse onUserAcknowledgementReceived(RaceHandle handle) override;

    virtual PluginResponse serveFiles(LinkID /*linkId*/, std::string /*path*/) override {
        return PLUGIN_ERROR;
    };

    virtual PluginResponse createBootstrapLink(RaceHandle /*handle*/, std::string /*channelGid*/,
                                               std::string /*passphrase*/) override {
        return PLUGIN_ERROR;
    };

    virtual PluginResponse flushChannel(RaceHandle /* handle */, std::string /* channelGid */,
                                        uint64_t /* batchId */) override {
        return PLUGIN_OK;
    };

    // Common Apis
    virtual ChannelResponse requestPluginUserInput(std::string componentId, const std::string &key,
                                                   const std::string &prompt, bool cache);
    virtual ChannelResponse requestCommonUserInput(std::string componentId, const std::string &key);
    virtual ChannelResponse updateState(std::string componentId, ComponentState state);

    // IEncodingSdk APIs
    virtual ChannelResponse onBytesEncoded(RaceHandle handle, const std::vector<uint8_t> &bytes,
                                           EncodingStatus status);

    virtual ChannelResponse onBytesDecoded(RaceHandle handle, const std::vector<uint8_t> &bytes,
                                           EncodingStatus status);

    // ITransportSdk APIs
    virtual ChannelResponse onLinkStatusChanged(RaceHandle handle, const LinkID &linkId,
                                                LinkStatus status, const LinkParameters &params);

    virtual ChannelResponse onPackageStatusChanged(RaceHandle handle, PackageStatus status);

    virtual ChannelResponse onEvent(const Event &event);

    virtual ChannelResponse onReceive(const LinkID &linkId, const EncodingParameters &params,
                                      const std::vector<uint8_t> &bytes);

    // IUserModelSdk APIs
    virtual ChannelResponse onTimelineUpdated();

    // Misc
    std::string toString() const;

    // Sdk interaction
    virtual std::string getActivePersona();
    virtual ChannelProperties getChannelProperties();
    virtual ChannelResponse makeDir(const std::string &directoryPath);
    virtual ChannelResponse removeDir(const std::string &directoryPath);
    virtual std::vector<std::string> listDir(const std::string &directoryPath);
    virtual std::vector<std::uint8_t> readFile(const std::string &filepath);
    virtual ChannelResponse appendFile(const std::string &filepath,
                                       const std::vector<std::uint8_t> &data);
    virtual ChannelResponse writeFile(const std::string &filepath,
                                      const std::vector<std::uint8_t> &data);

    void waitForCallbacks();
    void markFailed();

protected:
    template <typename T, typename... Args>
    ChannelResponse post(const std::string &logPrefix, T &&function, Args &&... args);

    template <typename T, typename... Args>
    PluginResponse post_sync(const std::string &logPrefix, T &&function, Args &&... args);

public:
    IRaceSdkComms &sdk;

protected:
    Handler handler;
    ComponentManagerInternal manager;
    std::atomic<uint64_t> nextPostId = 1;
};

std::ostream &operator<<(std::ostream &out, const ComponentManager &manager);
std::ostream &operator<<(std::ostream &out, const ComponentManagerInternal &manager);
