
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

#ifndef __RACE_SDK_H__
#define __RACE_SDK_H__

#include <AppConfig.h>
#include <IRaceApp.h>
#include <IRacePluginComms.h>
#include <IRacePluginNM.h>
#include <IRaceSdkComms.h>
#include <IRaceSdkNM.h>
#include <IRaceSdkTestApp.h>
#include <RaceLog.h>
#include <StorageEncryption.h>

#include <cstdint>
#include <future>
#include <list>
#include <memory>        // std::unique_ptr
#include <mutex>         // std::mutex, std::lock_guard
#include <shared_mutex>  // std::shared_mutex
#include <unordered_map>
#include <utility>

#include "AppConfig.h"
#include "BootstrapManager.h"
#include "IRaceApp.h"
#include "IRacePluginComms.h"
#include "IRacePluginNM.h"
#include "IRaceSdkComms.h"
#include "IRaceSdkNM.h"
#include "IRaceSdkTestApp.h"
#include "OpenTracingForwardDeclarations.h"
#include "PersonaForwardDeclarations.h"
#include "PluginLoader.h"
#include "RaceChannels.h"
#include "RaceConfig.h"
#include "RaceLinks.h"
#include "RaceLog.h"
#include "WrapperForwardDeclarations.h"

class RaceSdk : public IRaceSdkTestApp {
public:
    struct PendingBootstrap {
        RaceHandle prepareBootstrapHandle = NULL_RACE_HANDLE;
        RaceHandle createdLinkHandle = NULL_RACE_HANDLE;
        RaceHandle connectionHandle = NULL_RACE_HANDLE;
        DeviceInfo deviceInfo;
        std::string passphrase;
        std::string bootstrapPath;
        std::vector<std::string> commsPlugins;
        LinkID bootstrapLink;
        ConnectionID bootstrapConnection;
    };

    // RaceSdk

    /**
     * @brief Constructor. This API should ONLY BE USED FOR TESTING. It provides a way to mock out
     * the plugin creation for testing. Should NOT be used in any production code.
     *
     * @param appConfig The application configuration object.
     * @param raceConfig The race configuration object.
     * @param pluginLoader The plugin loader, in this case a mock for testing without real plugins.
     */
    explicit RaceSdk(
        const AppConfig &appConfig, const RaceConfig &raceConfig,
        IPluginLoader &pluginLoader = IPluginLoader::factoryDefault("/usr/local/lib/"),
        std::shared_ptr<FileSystemHelper> fileSystemHelper = std::make_shared<FileSystemHelper>());

    /**
     * @brief Constructor.
     *
     * @param appConfig The application configuration object.
     * @param passphrase A user provided passphrase used for encrypting sensitive files.
     */
    explicit RaceSdk(const AppConfig &appConfig, const std::string &passphrase);

    // Delete the copy and move constructors.
    RaceSdk(const RaceSdk &) = delete;
    RaceSdk(RaceSdk &&) = delete;
    // Delete the copy and move assignment operators.
    RaceSdk &operator=(const RaceSdk &) = delete;
    RaceSdk &operator=(RaceSdk &&) = delete;

    virtual ~RaceSdk();

    const AppConfig &getAppConfig() const override {
        return appConfig;
    }
    const RaceConfig &getRaceConfig() const {
        return raceConfig;
    }
    const std::shared_ptr<opentracing::Tracer> &getTracer() const {
        return tracer;
    }

    ArtifactManager *getArtifactManager() {
        return artifactManager.get();
    }

    /**
     * @brief Get the comms wrapper for a plugin. This does not prevent the comms wrapper from being
     * deleted after returning, so it should be used carefully. Its primary purpose is for testing.
     *
     * @param name The name of the plugin to return
     * @return A pointer to the plugin wrapper
     */
    CommsWrapper *getCommsWrapper(const std::string &name) {
        std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
        return commsWrappers.at(name).get();
    }

    /**
     * @brief Get the network manager wrapper for a plugin. This does not prevent the network
     * manager wrapper from being deleted after returning, so it should be used carefully. Its
     * primary purpose is for testing.
     *
     * @return A pointer to the plugin wrapper
     */
    virtual NMWrapper *getNM() {
        return networkManagerWrapper.get();
    }

    /**
     * @brief Shutdown a specific comms plugin. Caller should should make sure the argument exists
     * during the call, but must not have it locked for writing.
     *
     * @param plugin A reference to the wrapper to shut down
     */
    virtual void shutdownPluginAsync(CommsWrapper &plugin);

    /**
     * @brief Generate opentracing tags for a given span and Link
     *
     * @param  span An opentracing span.
     * @param linkID the ID for the link.
     */
    virtual void traceLinkStatus(std::shared_ptr<opentracing::Span> span, LinkID linkId);

    // IRaceSdkCommon
    virtual RawData getEntropy(std::uint32_t numBytes) override;
    virtual std::string getActivePersona() override;

    // IRaceSdkApp
    virtual bool initRaceSystem(IRaceApp *_app) override;
    virtual SdkResponse onUserInputReceived(RaceHandle handle, bool answered,
                                            const std::string &response) override;
    virtual SdkResponse onUserAcknowledgementReceived(RaceHandle handle) override;

    /**
     * @brief Get the plugin storage instance. This is used by the plugin wrappers for reading and
     * writing to encrypted storage.
     *
     * @return StorageEncryption& Reference to the plugin storage instance.
     */
    virtual StorageEncryption &getPluginStorage();

    virtual SdkResponse asyncError(RaceHandle handle, PluginResponse status) override;

    /**
     * @brief Get the ChannelProperties for a particular channel
     *
     * @param channelGid The name of the channel
     * @return ChannelProperties the properties for the channel
     */
    virtual ChannelProperties getChannelProperties(std::string channelGid) override;

    /**
     * @brief Get Channel Properties for all channels. This may be used instead of
     * getSupportedChannels to get channels regardless of what state they're in
     * (getSupportedChannels only returns channels in the AVAILABLE state).
     *
     * @return std::vector<ChannelProperties> the properties for all channels
     */
    virtual std::vector<ChannelProperties> getAllChannelProperties() override;

    /**
     * @brief Create the directory of directoryPath, including any directories in the path that do
     * not yet exist
     * @param directoryPath the path of the directory to create.
     *
     * @return SdkResponse indicator of success or failure of the create
     */
    virtual SdkResponse makeDir(const std::string &directoryPath) override;

    /**
     * @brief Recurively remove the directory of directoryPath
     * @param directoryPath the path of the directory to remove.
     *
     * @return SdkResponse indicator of success or failure of the removal
     */
    virtual SdkResponse removeDir(const std::string &directoryPath) override;

    /**
     * @brief List the contents (directories and files) of the directory path
     * @param directoryPath the path of the directory to list.
     *
     * @return std::vector<std::string> list of directories and files
     */
    virtual std::vector<std::string> listDir(const std::string &directoryPath) override;

    virtual std::vector<std::uint8_t> readFile(const std::string &filename) override;

    virtual SdkResponse appendFile(const std::string &filename,
                                   const std::vector<std::uint8_t> &data) override;

    virtual SdkResponse writeFile(const std::string &filename,
                                  const std::vector<std::uint8_t> &data) override;

    virtual bool addVoaRules(const nlohmann::json &payload) override;
    virtual bool deleteVoaRules(const nlohmann::json &payload) override;
    virtual void setVoaActiveState(bool state) override;

    virtual bool setEnabledChannels(const std::vector<std::string> &channelGids) override;
    virtual bool enableChannel(const std::string &channelGid) override;
    virtual bool disableChannel(const std::string &channelGid) override;

    // TestApp
    virtual void sendNMBypassMessage(ClrMsg msg, const std::string &route) override;
    virtual void openNMBypassReceiveConnection(const std::string &persona,
                                               const std::string &route) override;
    virtual void rpcDeactivateChannel(const std::string &channelGid) override;
    virtual void rpcDestroyLink(const std::string &linkId) override;
    virtual void rpcCloseConnection(const std::string &connectionId) override;
    virtual void rpcNotifyEpoch(const std::string &data) override;
    virtual std::vector<std::string> getInitialEnabledChannels() override;

    // network manager
    virtual std::vector<LinkID> getLinksForPersonas(std::vector<std::string> recipientPersonas,
                                                    LinkType linkType);
    virtual std::vector<LinkID> getLinksForChannel(std::string channelGid);
    virtual LinkProperties getLinkProperties(LinkID linkId);
    virtual std::map<std::string, ChannelProperties> getSupportedChannels();
    virtual std::vector<std::string> getPersonasForLink(std::string linkId);
    virtual LinkID getLinkForConnection(ConnectionID connectionId);

    // network manager (requires wrapper)
    virtual SdkResponse sendEncryptedPackage(NMWrapper &plugin, EncPkg ePkg,
                                             ConnectionID connectionId, uint64_t batchId,
                                             int32_t timeout);
    virtual SdkResponse shipPackage(RaceHandle handle, EncPkg ePkg, ConnectionID connectionId,
                                    int32_t timeout, bool isTestHarness, uint64_t batchId);
    virtual SdkResponse shipVoaItems(RaceHandle handle,
                                     std::list<std::pair<EncPkg, double>> voaPkgQueue,
                                     ConnectionID connectionId, int32_t timeout, bool isTestHarness,
                                     uint64_t batchId);

    virtual SdkResponse presentCleartextMessage(NMWrapper &plugin, ClrMsg msg);
    virtual SdkResponse onPluginStatusChanged(NMWrapper &plugin, PluginStatus status);
    virtual SdkResponse deactivateChannel(NMWrapper &plugin, std::string channelGid,
                                          std::int32_t timeout);
    virtual SdkResponse activateChannel(NMWrapper &plugin, const std::string &channelGid,
                                        const std::string &roleName, std::int32_t timeout);
    virtual SdkResponse destroyLink(NMWrapper &plugin, LinkID linkId, std::int32_t timeout);
    virtual SdkResponse createLink(NMWrapper &plugin, std::string channelGid,
                                   std::vector<std::string> personas, std::int32_t timeout);
    virtual SdkResponse loadLinkAddress(NMWrapper &plugin, std::string channelGid,
                                        std::string linkAddress, std::vector<std::string> personas,
                                        std::int32_t timeout);
    virtual SdkResponse loadLinkAddresses(NMWrapper &plugin, std::string channelGid,
                                          std::vector<std::string> linkAddresses,
                                          std::vector<std::string> personas, std::int32_t timeout);

    virtual SdkResponse createLinkFromAddress(NMWrapper &plugin, std::string channelGid,
                                              std::string linkAddress,
                                              std::vector<std::string> personas,
                                              std::int32_t timeout);

    virtual SdkResponse bootstrapDevice(NMWrapper &plugin, RaceHandle handle,
                                        std::vector<std::string> commsChannels);
    virtual SdkResponse bootstrapFailed(RaceHandle handle);
    void bootstrapFailed(const PendingBootstrap &failedBootstrap);

    virtual SdkResponse setPersonasForLink(NMWrapper &plugin, std::string linkId,
                                           std::vector<std::string> personas);
    virtual SdkResponse openConnectionInternal(RaceHandle handle, LinkType linkType, LinkID linkId,
                                               std::string linkHints, int32_t priority,
                                               int32_t sendTimeout, int32_t timeout);
    virtual SdkResponse openConnection(NMWrapper &plugin, LinkType linkType, LinkID linkId,
                                       std::string linkHints, int32_t priority, int32_t sendTimeout,
                                       int32_t timeout);
    virtual SdkResponse closeConnection(NMWrapper &plugin, ConnectionID connectionId,
                                        int32_t timeout);
    virtual SdkResponse onMessageStatusChanged(RaceHandle handle, MessageStatus status);

    virtual SdkResponse flushChannel(NMWrapper &plugin, std::string channelGid, uint64_t batchId,
                                     int32_t timeout);

    virtual SdkResponse sendBootstrapPkg(NMWrapper &plugin, ConnectionID connectionId,
                                         const std::string &persona, const RawData &key,
                                         int32_t timeout);

    // comms (requires wrapper)
    virtual SdkResponse onPackageStatusChanged(CommsWrapper &plugin, RaceHandle handle,
                                               PackageStatus status, int32_t timeout);
    virtual SdkResponse onConnectionStatusChanged(CommsWrapper &plugin, RaceHandle handle,
                                                  ConnectionID connId, ConnectionStatus status,
                                                  LinkProperties properties, int32_t timeout);
    virtual SdkResponse onLinkStatusChanged(CommsWrapper &plugin, RaceHandle handle, LinkID linkId,
                                            LinkStatus status, LinkProperties properties,
                                            int32_t timeout);
    virtual SdkResponse onChannelStatusChanged(CommsWrapper &plugin, RaceHandle handle,
                                               const std::string &channelGid, ChannelStatus status,
                                               const ChannelProperties &properties,
                                               int32_t timeout);
    virtual SdkResponse updateLinkProperties(CommsWrapper &plugin, const LinkID &linkId,
                                             const LinkProperties &properties, int32_t timeout);
    virtual ConnectionID generateConnectionId(CommsWrapper &plugin, LinkID linkId);
    virtual LinkID generateLinkId(CommsWrapper &plugin, const std::string &channelGid);
    virtual SdkResponse receiveEncPkg(CommsWrapper &plugin, const EncPkg &pkg,
                                      const std::vector<ConnectionID> &connIDs, int32_t timeout);

    virtual SdkResponse serveFiles(LinkID linkId, const std::string &path, int32_t timeout);

    // Client
    virtual RaceHandle sendClientMessage(ClrMsg msg) override;
    virtual RaceHandle prepareToBootstrap(DeviceInfo deviceInfo, std::string passphrase,
                                          std::string bootstrapChannelId) override;
    virtual bool cancelBootstrap(RaceHandle handle) override;
    virtual bool onBootstrapFinished(RaceHandle bootstrapHandle, BootstrapState state);
    virtual std::vector<std::string> getContacts() override;
    virtual bool isConnected() override;

    // Server
    // TODO: can these be consolidated to a single function?
    virtual void cleanShutdown() override;
    virtual void notifyShutdown(int numSeconds) override;

    // Used if network manager returns plugin fatal. Race can't continue, but we don't have any way
    // to shut down the app cleanly right now
    // TODO: Shutdown properly
    void shutdownCommsAndCrash();

    // View internal state for tests
    std::vector<PendingBootstrap> getPendingBootstraps();

    virtual SdkResponse requestPluginUserInput(const std::string &pluginId, bool isTestHarness,
                                               const std::string &key, const std::string &prompt,
                                               bool cache);
    virtual SdkResponse requestCommonUserInput(const std::string &pluginId, bool isTestHarness,
                                               const std::string &key);
    virtual SdkResponse displayInfoToUser(const std::string &pluginId, const std::string &data,
                                          RaceEnums::UserDisplayType displayType);
    virtual SdkResponse displayBootstrapInfoToUser(const std::string &pluginId,
                                                   const std::string &data,
                                                   RaceEnums::UserDisplayType displayType,
                                                   RaceEnums::BootstrapActionType actionType);

    // AMP
    virtual std::string getAppPath(const std::string &pluginId);
    virtual SdkResponse sendAmpMessage(const std::string &pluginId, const std::string &destination,
                                       const std::string &message);

    /**
     * @brief Returns the network manager plugin to be notified with a callback for the given
     * handle. If the handle is for calls related to the test harness, the most significant bit will
     * be set to 1. All others are for the real network manager plugin.
     *
     * @param handle Callback handle
     * @return network manager plugin or test harness pointer (no ownership transfer)
     */
    virtual NMWrapper *getNM(RaceHandle handle) const;

    /**
     * @brief Create a unique handle.
     *
     * If the handle is for calls related to the test harness, the most significant bit will be set
     * to 1. This means that all handles above 2^63 belong to the test harness, while all those
     * below that are used for the real network manager plugin.
     *
     * @param testHarness Handle belongs to calls to be routed to the test harness, rather than the
     *      real network manager plugin
     * @return RACE handle
     */
    RaceHandle generateHandle(bool testHarness);

    virtual bool createBootstrapLink(RaceHandle handle, const std::string &passphrase,
                                     const std::string &bootstrapChannelId);

protected:
    explicit RaceSdk(const AppConfig &appConfig, IPluginLoader &pluginLoader,
                     const std::string &passphrase);

    bool setChannelEnabled(const std::string &channelName, bool enabled);
    void initializeRaceChannels();

    void logConfigFiles();
    void loadArtifactManagerPlugins(std::vector<PluginDef> pluginsToLoad);
    void initArtifactManagerPlugins();
    void loadNMPlugin(std::vector<PluginDef> pluginsToLoad);
    void initNMPlugin();
    void loadCommsPlugins();
    void initCommsPlugins();

    bool getSdkUserResponses();
    bool setAllowedEnvironmentTags();
    void shutdownPlugins();
    void destroyPlugins();
    void cleanupChannels(CommsWrapper &plugin);
    /**
     * @brief Shutdown a specific comms plugin. Caller should have the commsWrapper lock locked for
     * writing. This method is synchronous as opposed to the asynchronous public version.
     *
     * @param plugin A reference to the wrapper to shut down
     */
    void shutdownPluginInternal(CommsWrapper &plugin);

    static bool doesLinkPropertiesContainUndef(const LinkProperties &props,
                                               const std::string &logPrefix = "");

    EncPkg createBootstrapPkg(const std::string &persona, const RawData &key);
    bool validateDeviceInfo(const DeviceInfo &deviceInfo);
    bool validateBootstrapConfigPath(const std::string &configPath);

    void handleBootstrapLinkCreated(LinkID linkId, PendingBootstrap &bootstrapInfo);

    /**
     * @brief Initialize the configs from the configs.tar.gz file. This includes extracting the
     * files, encrypting them, and moving them to the final location to be read by the SDK and
     * plugins.
     *
     * @param configTarGzDir The path to configs.tar.gz.
     * @param destDir The path to extract the tar to.
     *
     * NOTE: this is a DEBUG feature and is not expected to exist in a production system.
     *
     */
    void initializeConfigsFromTarGz(const std::string &configTarGz, const std::string &destDir);

protected:
    StorageEncryption pluginStorageEncryption;
    const AppConfig appConfig;
    RaceConfig raceConfig;
    std::mutex userInputHandlesLock;
    // Map of handles to plugin IDs
    std::unordered_map<RaceHandle, std::string> userInputHandles;
    std::unique_ptr<NMWrapper> networkManagerWrapper;
    std::unique_ptr<TestHarnessWrapper> networkManagerTestHarness;
    std::unordered_map<std::string, std::unique_ptr<CommsWrapper>> commsWrappers;
    std::unique_ptr<AppWrapper> appWrapper;
    std::unique_ptr<ArtifactManager> artifactManager;
    std::unique_ptr<VoaThread> voaThread;
    IPluginLoader &pluginLoader;
    std::shared_ptr<opentracing::Tracer> tracer;
    std::mutex traceIdLock;
    std::atomic<bool> isShuttingDown;

    // use commsWrapperReadWriteLock to lock commsWrappers
    // commsWrapperReadWriteLock must not be locked after connectionsReadWriteLock
    std::shared_mutex commsWrapperReadWriteLock;
    std::shared_mutex connectionsReadWriteLock;
    bool isReady;
    nlohmann::json statusJson;
    std::unordered_set<std::string> channelsActivateRequested;
    std::unordered_set<std::string> channelsDisableRequested;

    std::mutex sdkUserResponseLock;
    std::unordered_map<RaceHandle, std::promise<std::optional<std::string>>> sdkUserInputRequests;

    BootstrapManager bootstrapManager;
    virtual BootstrapManager &getBootstrapManager() {
        return bootstrapManager;
    }

    std::atomic<RaceHandle> networkManagerPluginHandleCount;
    std::atomic<RaceHandle> testHarnessHandleCount;

public:
    std::unique_ptr<RaceLinks> links;
    std::unique_ptr<RaceChannels> channels;
};

#endif
