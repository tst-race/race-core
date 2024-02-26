
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

// Need to build against the local (not system) header.
#include "../include/RaceConfig.h"

// Need to build against the local (not system) header.
#include <IRaceApp.h>
#include <PluginNMTestHarness.h>
#include <RaceLog.h>
#include <base64.h>
#include <openssl/rand.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <random>  // std::random_device
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include "../include/RaceSdk.h"
#include "AppWrapper.h"
#include "ArtifactManager.h"
#include "ArtifactManagerWrapper.h"
#include "BootstrapThread.h"
#include "CommsWrapper.h"
#include "ConfigLogging.h"
#include "NMWrapper.h"
#include "OpenTracingHelpers.h"
#include "PluginLoader.h"
#include "TestHarnessWrapper.h"
#include "VoaThread.h"
#include "helper.h"

static constexpr RaceHandle START_TEST_HARNESS_HANDLE = 1ul << 63;

using namespace std::string_literals;

template <typename T>
static std::string vectorToString(const std::vector<T> &someVector) {
    if (someVector.size() == 0) {
        return "";
    }

    std::stringstream result;
    result << "{ " << someVector[0];
    for (size_t index = 1; index < someVector.size(); ++index) {
        result << ", " << someVector[index];
    }
    result << " }";

    return result.str();
}

template <class T>
static void runEachComms(
    std::unordered_map<std::string, std::unique_ptr<CommsWrapper>> &commsWrappers, T &&func) {
    for (auto &[name, commsWrapper] : commsWrappers) {
        (void)name;
        std::forward<T>(func)(commsWrapper);
    }
}

// This constructor API is used for testing, allowing the plugin loader to be swapped out for one
// that generates mock classes.
// TODO: we could probably accomplish the same thing by creating a test class that inherits from
// RaceSdk and updates the plugin loader that way since it is a protected member.
RaceSdk::RaceSdk(const AppConfig &_appConfig, const RaceConfig &_raceConfig,
                 IPluginLoader &_pluginLoader, std::shared_ptr<FileSystemHelper> fileSystemHelper) :
    appConfig(_appConfig),
    raceConfig(_raceConfig),
    voaThread(std::make_unique<VoaThread>(*this, appConfig.voaConfigPath)),
    pluginLoader(_pluginLoader),
    tracer(createTracer(appConfig.jaegerConfigPath, appConfig.persona)),
    isShuttingDown(false),
    isReady(false),
    statusJson({}),
    bootstrapManager(*this, fileSystemHelper),
    networkManagerPluginHandleCount(1u),
    testHarnessHandleCount(START_TEST_HARNESS_HANDLE),
    links(std::make_unique<RaceLinks>()),
    channels(std::make_unique<RaceChannels>(raceConfig.channels, this)) {
    TRACE_METHOD("_appConfig", "_raceConfig", "_pluginLoader", "fileSystemHelper");

    RaceLog::setLogFile(appConfig.logFilePath);
    RaceLog::setLogLevelFile(raceConfig.logLevel);
    RaceLog::setLogLevelStdout(raceConfig.logLevelStdout);

    initializeRaceChannels();
}

// TODO: this is a hack related to construction order of the config and the pluginStorageEncryption
// object required to read the config. Clean this mess up.
RaceSdk::RaceSdk(const AppConfig &_appConfig, IPluginLoader &_pluginLoader,
                 const std::string &passphrase) :
    appConfig(_appConfig),
    pluginLoader(_pluginLoader),
    isShuttingDown(false),
    isReady(false),
    bootstrapManager(*this),
    networkManagerPluginHandleCount(1u),
    testHarnessHandleCount(START_TEST_HARNESS_HANDLE),
    links(std::make_unique<RaceLinks>()) {
    TRACE_METHOD("_appConfig", "_pluginLoader");
    RaceLog::setLogFile(appConfig.logFilePath);

    helper::logInfo(logPrefix + "creating encryption key of type \"" +
                    storageEncryptionTypeToString(appConfig.encryptionType) + "\"...");
    pluginStorageEncryption.init(appConfig.encryptionType, passphrase, appConfig.etcDirectory);
    helper::logInfo(logPrefix + "created encryption key");

    initializeConfigsFromTarGz(appConfig.configTarPath, appConfig.baseConfigPath);

    tracer = createTracer(appConfig.jaegerConfigPath, appConfig.persona);

    voaThread = std::make_unique<VoaThread>(*this, appConfig.voaConfigPath);

    const std::string raceConfigPath =
        _appConfig.baseConfigPath + "/" + _appConfig.sdkFilePath + "/race.json";
    helper::logInfo("initializing RACE config from file: " + raceConfigPath);
    raceConfig = RaceConfig{
        _appConfig,
        helper::readFile("race.json", "", _appConfig.baseConfigPath + "/" + _appConfig.sdkFilePath,
                         pluginStorageEncryption)};

    RaceLog::setLogLevelFile(raceConfig.logLevel);
    RaceLog::setLogLevelStdout(raceConfig.logLevelStdout);

    channels = std::make_unique<RaceChannels>(raceConfig.channels, this);
    initializeRaceChannels();
}

RaceSdk::RaceSdk(const AppConfig &_appConfig, const std::string &passphrase) :
    RaceSdk(_appConfig, IPluginLoader::factoryDefault(_appConfig.pluginArtifactsBaseDir),
            passphrase) {
    TRACE_METHOD("_appConfig");
}

RaceSdk::~RaceSdk() {
    TRACE_METHOD();
    RaceSdk::cleanShutdown();
    destroyPlugins();
}

std::vector<std::string> RaceSdk::getInitialEnabledChannels() {
    TRACE_METHOD();
    if (not raceConfig.initialEnabledChannels.empty()) {
        return raceConfig.initialEnabledChannels;
    }

    // Default to enabling all supported channels
    std::vector<std::string> allChannels;
    for (auto &props : channels->getChannels()) {
        if (props.channelStatus != CHANNEL_UNSUPPORTED) {
            allChannels.push_back(props.channelGid);
        }
    }
    return allChannels;
}

bool RaceSdk::setEnabledChannels(const std::vector<std::string> &channelGids) {
    TRACE_METHOD(channelGids);

    if (isShuttingDown) {
        helper::logInfo(logPrefix + "sdk is shutting down");
        return false;
    }

    if (appWrapper != nullptr) {
        helper::logError(logPrefix +
                         "RACE system has already been initialized, this function can only be "
                         "called prior to calling initRaceSystem");
        return false;
    }

    channels->setUserEnabledChannels(channelGids);
    for (auto &channelGid : channelGids) {
        setChannelEnabled(channelGid, true);
    }
    return true;
}

bool RaceSdk::enableChannel(const std::string &channelGid) {
    TRACE_METHOD(channelGid);

    if (isShuttingDown) {
        helper::logInfo(logPrefix + "sdk is shutting down");
        return false;
    }

    std::string pluginName;
    try {
        pluginName = channels->getWrapperIdForChannel(channelGid);
    } catch (const std::out_of_range &) {
        // If it was an unsupported channel, it would get caught here.
        helper::logError(logPrefix + "Could not find plugin for channel: " + channelGid);
        return false;
    }

    channels->setUserEnabled(channelGid);
    if (channels->getStatus(channelGid) != CHANNEL_DISABLED) {
        helper::logInfo(logPrefix + "channel " + channelGid + " is already enabled");
        return true;
    }

    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError(logPrefix + pluginName + " comms plugin could not be found in RaceSdk.");
        return false;
    }

    const auto &commsWrapper = commsWrappers[pluginName];

    // Notify network manager that the channel is now enabled, let network manager activate the
    // channel
    auto channelProps = getChannelProperties(channelGid);
    SdkResponse response = onChannelStatusChanged(*commsWrapper, NULL_RACE_HANDLE, channelGid,
                                                  CHANNEL_ENABLED, channelProps, RACE_BLOCKING);
    return response.status == SDK_OK;
}

bool RaceSdk::disableChannel(const std::string &channelGid) {
    TRACE_METHOD(channelGid);

    if (isShuttingDown) {
        helper::logInfo(logPrefix + "sdk is shutting down");
        return false;
    }

    std::string pluginName;
    try {
        pluginName = channels->getWrapperIdForChannel(channelGid);
    } catch (const std::out_of_range &) {
        // If it was an unsupported channel, it would get caught here.
        helper::logError(logPrefix + "Could not find plugin for channel: " + channelGid);
        return false;
    }

    channels->setUserDisabled(channelGid);
    auto status = channels->getStatus(channelGid);
    if (status == CHANNEL_DISABLED) {
        helper::logInfo(logPrefix + "channel " + channelGid + " is already disabled");
        return true;
    }

    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError(logPrefix + pluginName + " comms plugin could not be found in RaceSdk.");
        return false;
    }
    auto &commsWrapper = commsWrappers[pluginName];

    if (status == CHANNEL_ENABLED or status == CHANNEL_UNAVAILABLE) {
        helper::logInfo(logPrefix + "channel " + channelGid + " is already deactivated");
        // Notify network manager that channel is now disabled
        auto channelProps = getChannelProperties(channelGid);
        SdkResponse response =
            onChannelStatusChanged(*commsWrapper, NULL_RACE_HANDLE, channelGid, CHANNEL_DISABLED,
                                   channelProps, RACE_BLOCKING);
        return response.status == SDK_OK;
    }

    // Instruct comms plugin to deactivate the channel

    // Will update status to DISABLED when we get the channel status change callback from the comms
    // plugin
    auto insertSuccess = channelsDisableRequested.insert(channelGid);
    if (not insertSuccess.second) {
        helper::logInfo(logPrefix + "channel " + channelGid +
                        " disable requested again after unsuccessful attempt");
    }

    RaceHandle handle = generateHandle(false);
    SdkResponse response = commsWrapper->deactivateChannel(handle, channelGid, RACE_BLOCKING);
    return response.status == SDK_OK;
}

bool RaceSdk::setChannelEnabled(const std::string &channel, bool enabled) {
    TRACE_METHOD(channel, enabled);

    // direct channel invalid on client node
    bool success = false;

    try {
        if (!(appConfig.nodeType == RaceEnums::NT_CLIENT &&
              channels->getChannelProperties(channel).connectionType == CT_DIRECT)) {
            channels->setStatus(channel, enabled ? CHANNEL_ENABLED : CHANNEL_DISABLED);
            success = true;
        } else {
            std::string nodeType = RaceEnums::nodeTypeToString(appConfig.nodeType);
            std::string connectionType =
                connectionTypeToString(channels->getChannelProperties(channel).connectionType);
            helper::logInfo(logPrefix + "not enabling " + channel + " for node type " + nodeType +
                            " connection type " + connectionType);
        }
    } catch (const std::out_of_range &) {
        // So RaceChannels is populated by the "channels" list in race.json. This is limited to the
        // channels that you explicitly enable via RiB.
        //
        // This section of the code loops over plugins definitions in race.json, which includes all
        // channels for a plugin.
        //
        // So if this failure case gets hit, it could be that you're not using any bootstrap
        // channels, but they get included in this iteration, but don't exist in the RaceChannels
        // object because you never enabled them via RiB. Which is fine.
        //
        // Not sure if this is a problem. It's how it already has worked, so I guess it's okay.
        helper::logDebug(logPrefix + "channel either does not exist or is not enabled: " + channel +
                         ". If this is not expected check your race.json. Otherwise you can "
                         "probably just ignore this message.");
    }
    return success;
}

void RaceSdk::initializeRaceChannels() {
    TRACE_METHOD();
    for (PluginDef &pluginDef : raceConfig.getCommsPluginDefs()) {
        for (std::string channel : pluginDef.channels) {
            channels->setPluginsForChannel(channel, {pluginDef.filePath});
            channels->setWrapperIdForChannel(channel, pluginDef.filePath);
            setChannelEnabled(channel, channels->isUserEnabled(channel));
        }
    }

    for (auto &composition : raceConfig.compositions) {
        std::vector<std::string> plugins;
        for (auto &pluginDef : composition.plugins) {
            plugins.push_back(pluginDef.filePath);
        }
        channels->setPluginsForChannel(composition.id, plugins);
        channels->setWrapperIdForChannel(composition.id, composition.id);
        setChannelEnabled(composition.id, channels->isUserEnabled(composition.id));
    }

    for (auto &props : channels->getChannels()) {
        helper::logInfo(logPrefix + props.channelGid + ": " +
                        channelStatusToString(props.channelStatus));
    }
}

RawData RaceSdk::getEntropy(std::uint32_t numBytes) {
    std::random_device rand_dev;
    unsigned int value;
    RawData randomness;
    randomness.reserve(numBytes);

    // Use bit shifts to fully utilize the entropy returned by random_device
    for (std::size_t index = 0u; index < numBytes; ++index) {
        if (index % sizeof(unsigned int) == 0) {
            value = rand_dev();
        }
        randomness.push_back(static_cast<std::uint8_t>(value));
        value >>= CHAR_BIT;
    }

    return randomness;
}

StorageEncryption &RaceSdk::getPluginStorage() {
    return pluginStorageEncryption;
}

std::string RaceSdk::getActivePersona() {
    TRACE_METHOD();
    return appConfig.persona;
}

bool RaceSdk::getSdkUserResponses() {
    TRACE_METHOD();
    std::promise<std::optional<std::string>> envPromise;
    auto envFuture = envPromise.get_future();

    {
        std::lock_guard<std::mutex> lock(sdkUserResponseLock);
        SdkResponse sdkResponse = requestCommonUserInput("sdk", false, "env");
        if (sdkResponse.status != SDK_OK) {
            helper::logDebug(logPrefix + "requestCommonUserInput failed");
            return false;
        }
        sdkUserInputRequests[sdkResponse.handle] = std::move(envPromise);
    }

    raceConfig.env = helper::stringToLowerCase(envFuture.get().value_or(""));
    return true;
}

bool RaceSdk::setAllowedEnvironmentTags() {
    TRACE_METHOD(raceConfig.env);

    if (raceConfig.environmentTags.size() > 0) {
        helper::logDebug(logPrefix + "printing raceConfig.environmentTags keys");
        for (auto iter = raceConfig.environmentTags.begin();
             iter != raceConfig.environmentTags.end(); ++iter) {
            helper::logDebug(logPrefix + "raceConfig.environmentTags key = " + iter->first);
        }
    } else {
        helper::logWarning(logPrefix + "raceConfig.environmentTags is empty");
    }

    auto it = raceConfig.environmentTags.find(raceConfig.env);
    if (it == raceConfig.environmentTags.end()) {
        helper::logError(logPrefix + "No environment tags entry matching enviroment \"" +
                         raceConfig.env + "\"");
        return false;
    }
    channels->setAllowedTags(raceConfig.environmentTags.at(raceConfig.env));

    return true;
}

bool RaceSdk::initRaceSystem(IRaceApp *_app) {
    TRACE_METHOD();

    logConfigFiles();

    if (raceConfig.isVoaEnabled) {
        voaThread->startThread();
    } else {
        helper::logInfo("VoA processing is disabled.");
    }

    if (_app == nullptr) {
        const std::string errorMessage = "Value for app can't be nullptr";
        helper::logError(errorMessage);
        throw std::invalid_argument(errorMessage);
    }

    appWrapper = std::make_unique<AppWrapper>(_app, *this);
    appWrapper->startHandler();

    if (raceConfig.env == "") {
        // If real user input is disabled, the raceConfig will not contain env
        if (!getSdkUserResponses()) {
            helper::logDebug("initRaceSystem: getSdkUserResponses failed, returning early");
            return false;
        }
    }
    if (!setAllowedEnvironmentTags()) {
        helper::logDebug("initRaceSystem: setAllowedEnvironmentTags failed, returning early");
        return false;
    }

    networkManagerTestHarness = std::make_unique<TestHarnessWrapper>(*this);

    loadArtifactManagerPlugins(raceConfig.getArtifactManagerPluginDefs());

    initArtifactManagerPlugins();

    loadNMPlugin(raceConfig.getNMPluginDefs());

    loadCommsPlugins();

    initNMPlugin();

    initCommsPlugins();

    networkManagerTestHarness->startHandler();

    if (commsWrappers.empty()) {
        const std::string errorMessage = "No Commss succeeded in starting. initRaceSystem failed.";
        helper::logError(errorMessage);
        cleanShutdown();
        destroyPlugins();
        throw std::runtime_error(errorMessage);
    }

    return true;
}

void RaceSdk::logConfigFiles() {
    helper::logDebug("    sdk configs: " + appConfig.baseConfigPath + "/" + appConfig.sdkFilePath);

    if (raceConfig.logRaceConfig) {
        helper::logInfo("Logging RaceConfig...");
        raceConfig.log();
        helper::logInfo(appConfig.to_string());
        logDirectoryTree(appConfig.baseConfigPath + "/" + appConfig.sdkFilePath,
                         pluginStorageEncryption);
    }

    helper::logInfo("Done Logging configuration");
}

void RaceSdk::loadArtifactManagerPlugins(std::vector<PluginDef> pluginsToLoad) {
    TRACE_METHOD();
    helper::logDebug("Loading ArtifactManager plugin candidates");
    auto wrappers = pluginLoader.loadArtifactManagerPlugins(*this, pluginsToLoad);
    if (wrappers.size() > 0) {
        helper::logInfo("ArtifactManager plugins loaded:");
        std::vector<std::unique_ptr<ArtifactManagerWrapper>> plugins;
        for (auto &wrapper : wrappers) {
            helper::logInfo("    ID: " + wrapper->getId() +
                            ", description: " + wrapper->getDescription());
            fs::create_directory(fs::path(getAppConfig().baseConfigPath) / wrapper->getId());
            plugins.push_back(std::move(wrapper));
        }
        artifactManager = std::make_unique<ArtifactManager>(std::move(plugins));
    } else {
        std::string message = "No ArtifactManager plugins loaded";
        if (raceConfig.isPluginFetchOnStartEnabled) {
            helper::logError(message);
            throw std::runtime_error(message);
        } else {
            helper::logWarning(message);
        }
    }
}

void RaceSdk::initArtifactManagerPlugins() {
    TRACE_METHOD();
    if (artifactManager) {
        helper::logDebug("Initializing ArtifactManager");
        if (!artifactManager->init(appConfig)) {
            std::string message = "Unable to initialize ArtifactManager";
            helper::logError(message);
            throw std::runtime_error(message);
        }
        helper::logDebug("ArtifactManager initialized");
    } else {
        helper::logDebug("No ArtifactManager in use, no need to initialize");
    }
}

void RaceSdk::loadNMPlugin(std::vector<PluginDef> pluginsToLoad) {
    TRACE_METHOD();
    helper::logDebug("Loading network manager plugin candidates");
    // TODO: pass in plugin info from global config file.
    auto networkManagers = pluginLoader.loadNMPlugins(*this, pluginsToLoad);
    if (networkManagers.size() == 0) {
        helper::logError("No valid network manager plugin found");
        throw std::runtime_error("No valid network manager plugin found");
    } else if (networkManagers.size() > 1) {
        helper::logError("Multiple network manager plugins found:");
        for (const auto &networkManager : networkManagers) {
            helper::logInfo("    ID: " + networkManager->getId() +
                            ", description:" + networkManager->getDescription());
        }
        helper::logError("Ensure only one network manager plugin is installed");
        throw std::runtime_error("Multiple network manager plugins found");
    }
    networkManagerWrapper = std::move(networkManagers.front());
    fs::create_directory(fs::path(getAppConfig().baseConfigPath) / networkManagerWrapper->getId());

    helper::logInfo("network manager plugin loaded with ID: " + networkManagerWrapper->getId() +
                    ", description:" + networkManagerWrapper->getDescription());
}

void RaceSdk::initNMPlugin() {
    TRACE_METHOD();
    PluginConfig pluginConfig;
    pluginConfig.etcDirectory = appConfig.etcDirectory;
    pluginConfig.loggingDirectory = appConfig.logDirectory;
    pluginConfig.auxDataDirectory = appConfig.pluginArtifactsBaseDir + "/network-manager/" +
                                    networkManagerWrapper->getId() + "/aux-data";
    pluginConfig.tmpDirectory = appConfig.tmpDirectory.empty() ?
                                    "" :
                                    appConfig.tmpDirectory + "/" + networkManagerWrapper->getId();
    pluginConfig.pluginDirectory =
        appConfig.pluginArtifactsBaseDir + "/network-manager/" + networkManagerWrapper->getId();

    if (!networkManagerWrapper->init(pluginConfig)) {
        const std::string errorMessage = "NM failed to init. Shutting down.";
        helper::logError(errorMessage);
        // The call to start here is intentional. At this point the handler is _not_ started, so
        // we must call it in order to call shutdown.
        networkManagerWrapper->startHandler();
        cleanShutdown();
        destroyPlugins();
        throw std::runtime_error(errorMessage);
    } else {
        networkManagerWrapper->startHandler();
    }
}

void RaceSdk::loadCommsPlugins() {
    TRACE_METHOD();
    helper::logDebug("Loading comms plugin candidates");
    // TODO: pass in plugin info from global config file.
    auto commsWrapperList = pluginLoader.loadCommsPlugins(*this, raceConfig.getCommsPluginDefs(),
                                                          raceConfig.compositions);
    if (commsWrapperList.size() == 0) {
        helper::logError("No valid comms plugin found");
        throw std::runtime_error("No valid comms plugin found");
    }
    std::unique_lock<std::shared_mutex> commsWrapperWriteLock(commsWrapperReadWriteLock);
    for (auto &commsWrapper : commsWrapperList) {
        commsWrappers[commsWrapper->getId()] = std::move(commsWrapper);
    }
    helper::logInfo("comms plugins loaded:");
    for (const auto &[id, commsWrapper] : commsWrappers) {
        helper::logInfo("    ID: " + commsWrapper->getId() +
                        ", description:" + commsWrapper->getDescription());
        fs::create_directory(fs::path(getAppConfig().baseConfigPath) / id);
    }

    // loop over all channel
    for (auto &props : channels->getChannels()) {
        try {
            std::string id = channels->getWrapperIdForChannel(props.channelGid);
            if (commsWrappers.count(id) == 0) {
                channels->setStatus(props.channelGid, CHANNEL_UNSUPPORTED);
            }
        } catch (std::exception &e) {
        }
    }
}

void RaceSdk::initCommsPlugins() {
    TRACE_METHOD();
    PluginConfig pluginConfig;
    pluginConfig.etcDirectory = appConfig.etcDirectory;
    pluginConfig.loggingDirectory = appConfig.logDirectory;

    std::vector<CommsWrapper *> commsWrapperList;
    std::unique_lock<std::shared_mutex> commsWrapperWriteLock(commsWrapperReadWriteLock);

    // Make a list of all the comms plugins. We want to shut down and delete the plugin if it fails.
    // We can't iterate over the wrappers because of that.
    std::transform(commsWrappers.begin(), commsWrappers.end(), std::back_inserter(commsWrapperList),
                   [](auto &namePluginPair) { return namePluginPair.second.get(); });

    for (auto commsWrapper : commsWrapperList) {
        pluginConfig.auxDataDirectory =
            appConfig.pluginArtifactsBaseDir + "/comms/" + commsWrapper->getId() + "/aux-data";
        pluginConfig.tmpDirectory = appConfig.tmpDirectory.empty() ?
                                        "" :
                                        appConfig.tmpDirectory + "/" + commsWrapper->getId();
        pluginConfig.pluginDirectory =
            appConfig.pluginArtifactsBaseDir + "/comms/" + commsWrapper->getId();
        bool success = commsWrapper->init(pluginConfig);
        if (!success) {
            // have to start handler so shutdown works
            helper::logWarning("comms plugin initialization failed for plugin with ID: \"" +
                               commsWrapper->getId());
            commsWrapper->startHandler();
            shutdownPluginInternal(*commsWrapper);
        } else {
            commsWrapper->startHandler();
        }
    }
}

SdkResponse RaceSdk::onUserInputReceived(RaceHandle handle, bool answered,
                                         const std::string &response) {
    TRACE_METHOD(handle, answered, response);
    if (isShuttingDown) {
        helper::logInfo("onUserInputReceived: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    std::string pluginId;
    {
        std::lock_guard<std::mutex> lock(userInputHandlesLock);
        auto iter = userInputHandles.find(handle);
        if (iter == userInputHandles.end()) {
            helper::logError("Error: no user input response handle mapping found");
            return SDK_PLUGIN_MISSING;
        }
        pluginId = iter->second;
        userInputHandles.erase(iter);
    }

    // Check first if this handle was generated/associated with the sdk, then check the network
    // manager plugin, and comms plugins
    if (pluginId == "sdk") {
        std::lock_guard<std::mutex> lock(sdkUserResponseLock);
        auto promiseIter = sdkUserInputRequests.find(handle);
        if (promiseIter != sdkUserInputRequests.end()) {
            if (answered) {
                promiseIter->second.set_value(response);
            } else {
                promiseIter->second.set_value({});
            }
        }
        return {SDK_OK, 0, handle};
    } else if (pluginId == getNM(handle)->getId()) {
        auto [success, utilization] =
            getNM(handle)->onUserInputReceived(handle, answered, response, 0);
        auto sdkStatus = success ? SDK_OK : SDK_QUEUE_FULL;
        return {sdkStatus, utilization, handle};
    } else {
        std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
        auto iter = commsWrappers.find(pluginId);
        if (iter == commsWrappers.end()) {
            helper::logError("Error: plugin could not be found in RaceSdk.");
            return SDK_PLUGIN_MISSING;
        }
        auto [success, utilization] =
            iter->second->onUserInputReceived(handle, answered, response, 0);
        auto sdkStatus = success ? SDK_OK : SDK_QUEUE_FULL;
        return {sdkStatus, utilization, handle};
    }
    // TODO acknowledgements and user input responses need to go to AMP as well

    return SDK_INVALID_ARGUMENT;
}

SdkResponse RaceSdk::asyncError(RaceHandle handle, PluginResponse status) {
    TRACE_METHOD(handle, status);
    return SDK_OK;
}

/**
 * @brief Create the directory of directoryPath, including any directories in the path that do
 * not yet exist
 * @param directoryPath the path of the directory to create.
 *
 * @return SdkResponse indicator of success or failure of the create
 */
SdkResponse RaceSdk::makeDir(const std::string &directoryPath) {
    TRACE_METHOD(directoryPath);
    if (!helper::makeDir(directoryPath, getAppConfig().sdkFilePath,
                         getAppConfig().baseConfigPath)) {
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
SdkResponse RaceSdk::removeDir(const std::string &directoryPath) {
    TRACE_METHOD(directoryPath);
    if (!helper::removeDir(directoryPath, getAppConfig().sdkFilePath,
                           getAppConfig().baseConfigPath)) {
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
std::vector<std::string> RaceSdk::listDir(const std::string &directoryPath) {
    TRACE_METHOD(directoryPath);
    std::vector<std::string> contents =
        helper::listDir(directoryPath, getAppConfig().sdkFilePath, getAppConfig().baseConfigPath);
    return contents;
}

std::vector<std::uint8_t> RaceSdk::readFile(const std::string &filename) {
    TRACE_METHOD(filename);
    std::vector<std::uint8_t> data =
        helper::readFile(filename, getAppConfig().sdkFilePath, getAppConfig().baseConfigPath,
                         pluginStorageEncryption);
    return data;
}

SdkResponse RaceSdk::appendFile(const std::string &filename,
                                const std::vector<std::uint8_t> &data) {
    TRACE_METHOD(filename);
    if (!helper::appendFile(filename, getAppConfig().sdkFilePath, getAppConfig().baseConfigPath,
                            data, pluginStorageEncryption)) {
        return SDK_INVALID_ARGUMENT;
    }
    return SDK_OK;
}

SdkResponse RaceSdk::writeFile(const std::string &filename, const std::vector<std::uint8_t> &data) {
    TRACE_METHOD(filename);
    if (!helper::writeFile(filename, getAppConfig().sdkFilePath, getAppConfig().baseConfigPath,
                           data, pluginStorageEncryption)) {
        return SDK_INVALID_ARGUMENT;
    }
    return SDK_OK;
}

LinkProperties RaceSdk::getLinkProperties(LinkID linkId) {
    TRACE_METHOD(linkId);

    LinkProperties result;
    try {
        result = links->getLinkProperties(linkId);
    } catch (const std::out_of_range &) {
        helper::logError("getLinkProperties: unable able to find link properties for ID: " +
                         linkId);
    }
    return result;
}

std::map<std::string, ChannelProperties> RaceSdk::getSupportedChannels() {
    TRACE_METHOD();
    auto result = channels->getSupportedChannels();
    return result;
}

ChannelProperties RaceSdk::getChannelProperties(std::string channelGid) {
    TRACE_METHOD(channelGid);

    ChannelProperties result;
    try {
        result = channels->getChannelProperties(channelGid);
    } catch (const std::out_of_range &) {
        helper::logError("getChannelProperties: unable to find channel properties for: " +
                         channelGid);
    }
    return result;
}

std::vector<ChannelProperties> RaceSdk::getAllChannelProperties() {
    TRACE_METHOD();

    const auto theChannels = channels->getChannels();
    helper::logDebug(logPrefix + "found " + std::to_string(theChannels.size()) + " channels");
    return theChannels;
}

SdkResponse RaceSdk::deactivateChannel(NMWrapper &plugin, std::string channelGid,
                                       std::int32_t timeout) {
    TRACE_METHOD(plugin.getId(), channelGid);
    if (isShuttingDown) {
        helper::logInfo("deactivateChannel: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    if (!channels->isAvailable(channelGid)) {
        helper::logError("deactivateChannel: channel " + channelGid + " is already not available ");
        return SDK_INVALID_ARGUMENT;
    }

    std::string pluginName;
    try {
        pluginName = channels->getWrapperIdForChannel(channelGid);
    } catch (const std::out_of_range &) {
        // should be impossible because it would be caught by isAvailable above
        helper::logError("deactivateChannel: Could not find plugin for channel: " + channelGid);
        return SDK_INVALID_ARGUMENT;
    }

    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError("Error: plugin for channel could not be found in RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }
    auto &commsWrapper = commsWrappers[pluginName];
    RaceHandle handle = generateHandle(plugin.isTestHarness());
    SdkResponse response = commsWrapper->deactivateChannel(handle, channelGid, timeout);

    return response;
}

SdkResponse RaceSdk::activateChannel(NMWrapper &plugin, const std::string &channelGid,
                                     const std::string &roleName, std::int32_t timeout) {
    TRACE_METHOD(plugin.getId(), channelGid, roleName);
    if (isShuttingDown) {
        helper::logInfo("activateChannel: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    if (!channels->activate(channelGid, roleName)) {
        helper::logError("Channel " + channelGid + " is in invalid state to activate");
        return SDK_INVALID_ARGUMENT;
    }

    auto insertSuccess = channelsActivateRequested.insert(channelGid);
    if (!insertSuccess.second) {
        helper::logInfo("activateChannel: channel " + channelGid +
                        " activation requested again after unsucessful attempt");
    }

    std::string pluginName;
    try {
        pluginName = channels->getWrapperIdForChannel(channelGid);
    } catch (const std::out_of_range &) {
        // should be impossible because it would be caught by isAvailable above
        helper::logError("activateChannel: Could not find plugin for channel: " + channelGid);
        channels->channelFailed(channelGid);
        return SDK_INVALID_ARGUMENT;
    }

    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError("Error: plugin for channel could not be found in RaceSdk.");
        channels->channelFailed(channelGid);
        return SDK_PLUGIN_MISSING;
    }
    auto &commsWrapper = commsWrappers[pluginName];
    RaceHandle handle = generateHandle(plugin.isTestHarness());
    SdkResponse response = commsWrapper->activateChannel(handle, channelGid, roleName, timeout);

    return response;
}

SdkResponse RaceSdk::destroyLink(NMWrapper &plugin, LinkID linkId, std::int32_t timeout) {
    TRACE_METHOD(plugin.getId(), linkId);
    if (isShuttingDown) {
        helper::logInfo("destroyLink: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    std::string pluginName = RaceLinks::getPluginFromLinkID(linkId);
    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError("Error: plugin for link could not be found in RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }
    auto &commsWrapper = commsWrappers[pluginName];

    RaceHandle handle = generateHandle(plugin.isTestHarness());
    SdkResponse response = commsWrapper->destroyLink(handle, linkId, timeout);

    return response;
}

SdkResponse RaceSdk::createLink(NMWrapper &plugin, std::string channelGid,
                                std::vector<std::string> personas, std::int32_t timeout) {
    TRACE_METHOD(plugin.getId(), channelGid);

    if (isShuttingDown) {
        helper::logInfo("createLink: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    if (!channels->isAvailable(channelGid)) {
        helper::logError("createLink: channel " + channelGid + " is not available ");
        return SDK_INVALID_ARGUMENT;
    }
    std::string pluginName;
    try {
        pluginName = channels->getWrapperIdForChannel(channelGid);
    } catch (const std::out_of_range &) {
        helper::logError("createLink: Could not find plugin for channel: " + channelGid);
        return SDK_INVALID_ARGUMENT;
    }

    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError("Error: plugin for channel could not be found in RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }
    auto &commsWrapper = commsWrappers[pluginName];

    RaceHandle handle = generateHandle(plugin.isTestHarness());
    // Store link request by handle in links to read when the onLinkStatusChanged callback comes
    links->addNewLinkRequest(handle, personas::PersonaSet(personas.begin(), personas.end()), "");
    SdkResponse response = commsWrapper->createLink(handle, channelGid, timeout);

    return response;
}

SdkResponse RaceSdk::loadLinkAddress(NMWrapper &plugin, std::string channelGid,
                                     std::string linkAddress, std::vector<std::string> personas,
                                     std::int32_t timeout) {
    TRACE_METHOD(plugin.getId(), channelGid, linkAddress);

    if (isShuttingDown) {
        helper::logInfo("loadLinkAddress: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    if (!channels->isAvailable(channelGid)) {
        helper::logError("loadLinkAddresses: channel " + channelGid + " is not available ");
        return SDK_INVALID_ARGUMENT;
    }
    std::string pluginName;
    try {
        pluginName = channels->getWrapperIdForChannel(channelGid);
    } catch (const std::out_of_range &) {
        helper::logError("createLink: Could not find plugin for channel: " + channelGid);
        return SDK_INVALID_ARGUMENT;
    }

    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError("Error: plugin for channel could not be found in RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }
    auto &commsWrapper = commsWrappers[pluginName];

    RaceHandle handle = generateHandle(plugin.isTestHarness());
    // Store link request by handle in links to read when the onLinkStatusChanged callback comes
    links->addNewLinkRequest(handle, personas::PersonaSet(personas.begin(), personas.end()),
                             linkAddress);
    SdkResponse response = commsWrapper->loadLinkAddress(handle, channelGid, linkAddress, timeout);

    return response;
}

SdkResponse RaceSdk::loadLinkAddresses(NMWrapper &plugin, std::string channelGid,
                                       std::vector<std::string> linkAddresses,
                                       std::vector<std::string> personas, std::int32_t timeout) {
    TRACE_METHOD(plugin.getId(), channelGid);

    if (isShuttingDown) {
        helper::logInfo("loadLinkAddresses: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    if (!channels->isAvailable(channelGid)) {
        helper::logError("loadLinkAddresses: channel " + channelGid + " is not available ");
        return SDK_INVALID_ARGUMENT;
    }
    std::string pluginName;
    try {
        pluginName = channels->getWrapperIdForChannel(channelGid);
    } catch (const std::out_of_range &) {
        helper::logError("createLink: Could not find plugin for channel: " + channelGid);
        return SDK_INVALID_ARGUMENT;
    }

    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError("Error: plugin for channel could not be found in RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }
    auto &commsWrapper = commsWrappers[pluginName];

    RaceHandle handle = generateHandle(plugin.isTestHarness());
    // Store link request by handle in links to read when the onLinkStatusChanged callback comes
    links->addNewLinkRequest(handle, personas::PersonaSet(personas.begin(), personas.end()),
                             "");  // TODO: handle multiple addresses
    SdkResponse response =
        commsWrapper->loadLinkAddresses(handle, channelGid, linkAddresses, timeout);

    return response;
}

SdkResponse RaceSdk::createLinkFromAddress(NMWrapper &plugin, std::string channelGid,
                                           std::string linkAddress,
                                           std::vector<std::string> personas,
                                           std::int32_t timeout) {
    TRACE_METHOD(plugin.getId(), channelGid, linkAddress);

    if (isShuttingDown) {
        helper::logInfo("createLinkFromAddress: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    if (!channels->isAvailable(channelGid)) {
        helper::logError("createLinkFromAddress: channel " + channelGid + " is not available ");
        return SDK_INVALID_ARGUMENT;
    }
    std::string pluginName;
    try {
        pluginName = channels->getWrapperIdForChannel(channelGid);
    } catch (const std::out_of_range &) {
        helper::logError("createLinkFromAddress: Could not find plugin for channel: " + channelGid);
        return SDK_INVALID_ARGUMENT;
    }

    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError("Error: plugin for channel could not be found in RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }
    auto &commsWrapper = commsWrappers[pluginName];

    RaceHandle handle = generateHandle(plugin.isTestHarness());
    // Store link request by handle in links to read when the onLinkStatusChanged callback comes
    links->addNewLinkRequest(handle, personas::PersonaSet(personas.begin(), personas.end()),
                             linkAddress);
    SdkResponse response =
        commsWrapper->createLinkFromAddress(handle, channelGid, linkAddress, timeout);

    return response;
}

SdkResponse RaceSdk::bootstrapDevice(NMWrapper &networkManagerPlugin, RaceHandle handle,
                                     std::vector<std::string> commsChannels) {
    TRACE_METHOD(networkManagerPlugin.getId(), handle);
    getBootstrapManager().bootstrapDevice(handle, commsChannels);
    return SDK_OK;
}

SdkResponse RaceSdk::bootstrapFailed(RaceHandle handle) {
    TRACE_METHOD(handle);
    getBootstrapManager().bootstrapFailed(handle);
    return SDK_OK;
}

SdkResponse RaceSdk::setPersonasForLink(NMWrapper &plugin, std::string linkId,
                                        std::vector<std::string> personas) {
    TRACE_METHOD(plugin.getId(), linkId);
    bool success =
        links->setPersonasForLink(linkId, personas::PersonaSet(personas.begin(), personas.end()));
    if (!success) {
        helper::logError("setPersonasForLink: could not find LinkID " + linkId + " in links");
        return SDK_INVALID_ARGUMENT;
    }
    return SDK_OK;
}

std::vector<std::string> RaceSdk::getPersonasForLink(std::string linkId) {
    TRACE_METHOD(linkId);
    try {
        personas::PersonaSet personas = links->getAllPersonasForLink(linkId);
        return std::vector<std::string>(personas.begin(), personas.end());
    } catch (const std::out_of_range &error) {
        helper::logError("getPersonasForLink: could not find LinkID " + linkId + " in links");
        return std::vector<std::string>();
    }
}

SdkResponse RaceSdk::shipPackage(RaceHandle handle, EncPkg ePkg, ConnectionID connectionId,
                                 int32_t timeout, bool isTestHarness, uint64_t batchId) {
    TRACE_METHOD(handle, connectionId, timeout, isTestHarness, batchId);

    if (isShuttingDown) {
        helper::logInfo("shipPackage: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    std::string pluginName = RaceLinks::getPluginFromConnectionID(connectionId);
    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError("Error: plugin for connection could not be found in RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }
    auto &commsWrapper = commsWrappers[pluginName];

    std::shared_lock<std::shared_mutex> connectionsReadLock(connectionsReadWriteLock);
    if (!links->doesConnectionExist(connectionId)) {
        helper::logError("shipPackage: connection is no longer open: " + connectionId);
        return SDK_INVALID_ARGUMENT;
    }

    // Add trace for connection use
    const std::pair<std::uint64_t, std::uint64_t> traceIds =
        links->getTraceCtxForConnection(connectionId);
    auto ctx = spanContextFromIds(traceIds);
    std::shared_ptr<opentracing::Span> span =
        tracer->StartSpan("CONNECTION_SEND", {opentracing::ChildOf(ctx.get())});
    span->SetTag("connectionId", connectionId);
    span->SetTag("size", ePkg.getSize());
    LinkID linkId = links->getLinkForConnection(connectionId);
    traceLinkStatus(span, linkId);
    span->Finish();

    ePkg.setPackageType(isTestHarness ? PKG_TYPE_TEST_HARNESS : PKG_TYPE_NM);
    links->cachePackageHandle(connectionId, handle);
    SdkResponse response = commsWrapper->sendPackage(handle, connectionId, ePkg, timeout, batchId);

    return response;
}

bool RaceSdk::addVoaRules(const nlohmann::json &payload) {
    TRACE_METHOD();
    if (!raceConfig.isVoaEnabled) {
        helper::logWarning("addVoaRules() called, but VoA is not enabled");
        return false;
    }
    helper::logDebug("RaceSdk::addVoaRules() called");
    return voaThread->addVoaRules(payload);
}

bool RaceSdk::deleteVoaRules(const nlohmann::json &payload) {
    TRACE_METHOD();
    if (!raceConfig.isVoaEnabled) {
        helper::logWarning("deleteVoaRules() called, but VoA is not enabled");
        return false;
    }
    helper::logDebug("RaceSdk::deleteVoaRules() called");
    return voaThread->deleteVoaRules(payload);
}

void RaceSdk::setVoaActiveState(bool state) {
    TRACE_METHOD(state);
    voaThread->setVoaActiveState(state);
}

SdkResponse RaceSdk::shipVoaItems(RaceHandle handle,
                                  std::list<std::pair<EncPkg, double>> voaPkgQueue,
                                  ConnectionID connectionId, int32_t timeout, bool isTestHarness,
                                  uint64_t batchId) {
    TRACE_METHOD(handle, connectionId, timeout, isTestHarness, batchId);

    if (isShuttingDown) {
        helper::logInfo("shipVoaItems: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    // Create work items from the
    // package queue
    std::list<std::shared_ptr<VoaThread::VoaWorkItem>> voaItems;
    for (std::pair<EncPkg, double> pkgItem : voaPkgQueue) {
        EncPkg ePkg = pkgItem.first;
        double holdTimestamp = pkgItem.second;

        // Special handling for dropped packages
        if (holdTimestamp == VOA_DROP_TIMESTAMP) {
            helper::logInfo("shipVoaItems: dropping package on connection ID:" + connectionId);
            // Return a null handle
            return SdkResponse(SDK_OK);
        }

        auto voa = std::make_shared<VoaThread::VoaWorkItem>(
            [=] {
                return shipPackage(handle, ePkg, connectionId, timeout, isTestHarness, batchId);
            },
            holdTimestamp);
        voaItems.push_back(voa);
    }
    voaThread->process(voaItems);

    return SdkResponse(SDK_OK, 0.0, handle);
}

SdkResponse RaceSdk::sendEncryptedPackage(NMWrapper &plugin, EncPkg ePkg, ConnectionID connectionId,
                                          uint64_t batchId, int32_t timeout) {
    TRACE_METHOD(plugin.getId(), connectionId, batchId);
    if (isShuttingDown) {
        helper::logInfo("sendEncryptedPackage: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    // check if the connection is valid
    std::shared_lock<std::shared_mutex> connectionsReadLock(connectionsReadWriteLock);
    if (!links->doesConnectionExist(connectionId)) {
        helper::logError("sendEncryptedPackage: connection is no longer open: " + connectionId);
        return SDK_INVALID_ARGUMENT;
    }

    bool isTestHarness = plugin.isTestHarness();
    RaceHandle handle = generateHandle(isTestHarness);

    if (raceConfig.isVoaEnabled && voaThread->isVoaActive()) {
        // Get VoA selectors
        LinkID linkId = links->getLinkForConnection(connectionId);
        std::string activePersona = getActivePersona();
        LinkProperties properties = getLinkProperties(linkId);
        personas::PersonaSet personas = links->getAllPersonasForLink(linkId);
        // Convert personaSet to personaList
        std::vector<std::string> personaList =
            std::vector<std::string>(personas.begin(), personas.end());
        std::list<std::pair<EncPkg, double>> voaPkgQueue = voaThread->getVoaPkgQueue(
            ePkg, activePersona, linkId, properties.channelGid, personaList);

        if (!voaPkgQueue.empty()) {
            helper::logDebug("RaceSdk::sendEncryptedPackage Number of VoA packages for linkId:" +
                             linkId + " Gid:" + properties.channelGid +
                             " personas:" + helper::personasToString(personaList) + " = " +
                             std::to_string(voaPkgQueue.size()));
            // only cache one handle for 1+ packages because they are all the same package
            links->cachePackageHandle(connectionId, handle);
            return shipVoaItems(handle, voaPkgQueue, connectionId, timeout, isTestHarness, batchId);
        }
    }

    // Make sure to unlock since shipPackage locks it again and that can cause a deadlock due to
    // undefined behavior
    connectionsReadLock.unlock();
    // If no VoA rule matched, simply ship the package
    return shipPackage(handle, ePkg, connectionId, timeout, isTestHarness, batchId);
}

SdkResponse RaceSdk::presentCleartextMessage(NMWrapper &plugin, ClrMsg msg) {
    TRACE_METHOD(plugin.getId());

    if (isShuttingDown) {
        helper::logInfo("presentCleartextMessage: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    if (msg.getAmpIndex() != NON_AMP_MESSAGE) {
        helper::logInfo(logPrefix + "Received amp message");
        try {
            artifactManager->receiveAmpMessage(msg);
            return SDK_OK;
        } catch (std::exception &e) {
            helper::logError(logPrefix + "Amp message has invalid amp index");
            return SDK_OK;  // No need to inform networkManager of the error
        }
    }

    // if (persona.type == PT_CLIENT) {
    if (appWrapper == nullptr) {
        helper::logError("Error: client has not been set for Race SDK.");
        return SDK_PLUGIN_MISSING;
    }

    appWrapper->handleReceivedMessage(msg);

    return SDK_OK;
}

SdkResponse RaceSdk::onPluginStatusChanged(NMWrapper &plugin, PluginStatus status) {
    TRACE_METHOD(plugin.getId(), status);

    if (isShuttingDown) {
        helper::logInfo("onPluginStatusChanged: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }
    isReady = status == PLUGIN_READY;

    if (appWrapper == nullptr) {
        helper::logInfo("client has not yet been set for Race SDK.");
        return SDK_PLUGIN_MISSING;
    }

    statusJson["network-manager-status"] = pluginStatusToString(status);
    try {
        appWrapper->onSdkStatusChanged(statusJson);
    } catch (nlohmann::json::exception &error) {
        helper::logError("RaceSdk::onPluginStatusChanged Failed to parse string : " +
                         std::string(error.what()));
    }

    return SDK_OK;
}

static bool isValidLinkType(LinkType linkType) {
    return linkType == LT_SEND || linkType == LT_RECV || linkType == LT_BIDI;
}

std::vector<LinkID> RaceSdk::getLinksForPersonas(std::vector<std::string> recipientPersonas,
                                                 LinkType linkType) {
    if (!isValidLinkType(linkType)) {
        const std::string errorMessage =
            "getLinksForPersonas: invalid link type" + std::to_string(static_cast<int>(linkType));
        helper::logError(errorMessage);
        return {};
    }

    helper::logDebug("getLinksForPersonas: getLinks for " +
                     helper::personasToString(recipientPersonas) +
                     " link type = " + linkTypeToString(linkType));

    const auto linkIds = links->getAllLinksForPersonas(
        personas::PersonaSet(recipientPersonas.begin(), recipientPersonas.end()), linkType);

    if (linkIds.size() == 0) {
        helper::logDebug("getLinksForPersonas: no links found in getLinks");
    }

    return linkIds;
}

std::vector<LinkID> RaceSdk::getLinksForChannel(std::string channelGid) {
    TRACE_METHOD(channelGid);
    auto result = channels->getLinksForChannel(channelGid);
    return result;
}

SdkResponse RaceSdk::openConnectionInternal(RaceHandle handle, LinkType linkType, LinkID linkId,
                                            std::string linkHints, int32_t priority,
                                            int32_t sendTimeout, int32_t timeout) {
    TRACE_METHOD(handle, linkType, linkId, linkHints, priority, sendTimeout, timeout);
    if (isShuttingDown) {
        helper::logInfo("openConnection: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    std::string pluginName = RaceLinks::getPluginFromLinkID(linkId);
    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError("Error: plugin for link could not be found in RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }

    try {
        links->addConnectionRequest(handle, linkId);
        SdkResponse response = commsWrappers[pluginName]->openConnection(
            handle, linkType, linkId, linkHints, priority, sendTimeout, timeout);

        return response;
    } catch (const std::out_of_range &error) {
        helper::logError("openConnection: invalid link ID \"" + linkId +
                         "\" for call to openConnection: " + error.what());
        return SDK_INVALID_ARGUMENT;
    } catch (const std::invalid_argument &error) {
        helper::logError("openConnection: invalid link ID \"" + linkId +
                         "\" for call to openConnection: " + error.what());
        return SDK_INVALID_ARGUMENT;
    }
}

SdkResponse RaceSdk::openConnection(NMWrapper &plugin, LinkType linkType, LinkID linkId,
                                    std::string linkHints, int32_t priority, int32_t sendTimeout,
                                    int32_t timeout) {
    TRACE_METHOD(plugin.getId(), linkType, linkId, linkHints, priority, sendTimeout, timeout);
    RaceHandle handle = generateHandle(plugin.isTestHarness());
    return openConnectionInternal(handle, linkType, linkId, linkHints, priority, sendTimeout,
                                  timeout);
}

SdkResponse RaceSdk::closeConnection(NMWrapper &plugin, ConnectionID connectionId,
                                     int32_t timeout) {
    TRACE_METHOD(plugin.getId(), connectionId, timeout);

    if (isShuttingDown) {
        helper::logWarning("closeConnection can't be called right now. sdk is shutting down.");
        return SDK_SHUTTING_DOWN;
    }

    std::string pluginName = RaceLinks::getPluginFromConnectionID(connectionId);
    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError("Error: plugin for connection could not be found in RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }
    auto &commsWrapper = commsWrappers[pluginName];

    RaceHandle handle = generateHandle(plugin.isTestHarness());
    SdkResponse response = commsWrapper->closeConnection(handle, connectionId, timeout);

    return response;
}

SdkResponse RaceSdk::onMessageStatusChanged(RaceHandle handle, MessageStatus status) {
    TRACE_METHOD(handle, status);
    appWrapper->onMessageStatusChanged(handle, status);
    return SDK_OK;
}

EncPkg RaceSdk::createBootstrapPkg(const std::string &persona, const RawData &key) {
    nlohmann::json contentsJson;
    contentsJson["persona"] = persona;
    contentsJson["key"] = base64::encode(key);

    // TODO: opentracing
    std::string contentsString = contentsJson.dump();
    EncPkg pkg(0, 0, {contentsString.begin(), contentsString.end()});
    pkg.setPackageType(PKG_TYPE_SDK);
    return pkg;
}

SdkResponse RaceSdk::sendBootstrapPkg(NMWrapper &plugin, ConnectionID connectionId,
                                      const std::string &persona, const RawData &key,
                                      int32_t timeout) {
    TRACE_METHOD(plugin.getId(), connectionId, persona, timeout);

    if (isShuttingDown) {
        helper::logInfo("sendBootstrapPkg: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    std::string pluginName = RaceLinks::getPluginFromConnectionID(connectionId);
    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError("Error: plugin for connection could not be found in RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }
    auto &commsWrapper = commsWrappers[pluginName];

    std::shared_lock<std::shared_mutex> connectionsReadLock(connectionsReadWriteLock);

    if (!links->doesConnectionExist(connectionId)) {
        helper::logError("sendBootstrapPkg: connection is no longer open: " + connectionId);
        return SDK_INVALID_ARGUMENT;
    }

    EncPkg pkg = createBootstrapPkg(persona, key);

    RaceHandle handle = generateHandle(plugin.isTestHarness());
    links->cachePackageHandle(connectionId, handle);
    SdkResponse response =
        commsWrapper->sendPackage(handle, connectionId, pkg, timeout, RACE_BATCH_ID_NULL);

    return response;
}

SdkResponse RaceSdk::flushChannel(NMWrapper &plugin, std::string channelGid, uint64_t batchId,
                                  int32_t timeout) {
    TRACE_METHOD(plugin.getId(), channelGid, batchId, timeout);

    if (isShuttingDown) {
        helper::logInfo("flushChannel: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    if (batchId == RACE_BATCH_ID_NULL) {
        helper::logError("flushChannel: null/invalid batch ID.");
        return SDK_INVALID_ARGUMENT;
    }

    if (!channels->isAvailable(channelGid)) {
        helper::logError("flushChannel: channel not available: " + channelGid);
        return SDK_INVALID_ARGUMENT;
    }

    std::string pluginName = channels->getWrapperIdForChannel(channelGid);
    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError(
            "flushChannel: Error: plugin for channel could not be found in RaceSdk with channel "
            "GID "
            "ID: " +
            channelGid + " and plugin name: " + pluginName);
        return SDK_PLUGIN_MISSING;
    }
    auto &commsWrapper = commsWrappers[pluginName];

    RaceHandle handle = generateHandle(plugin.isTestHarness());
    SdkResponse response = commsWrapper->flushChannel(handle, channelGid, batchId, timeout);

    return response;
}

NMWrapper *RaceSdk::getNM(RaceHandle handle) const {
    if ((handle & START_TEST_HARNESS_HANDLE) != 0u) {
        return networkManagerTestHarness.get();
    }
    return networkManagerWrapper.get();
}

void RaceSdk::traceLinkStatus(std::shared_ptr<opentracing::Span> span, LinkID linkId) {
    personas::PersonaSet personas = links->getAllPersonasForLink(linkId);
    std::vector<std::string> personaList =
        std::vector<std::string>(personas.begin(), personas.end());
    LinkProperties properties = getLinkProperties(linkId);
    ChannelProperties chProperties = getChannelProperties(properties.channelGid);

    span->SetTag("source", "racesdk");
    span->SetTag("linkId", linkId);
    span->SetTag("channelGid", properties.channelGid);
    span->SetTag("linkAddress", properties.linkAddress);
    span->SetTag("personas", helper::personasToString(personaList));
    span->SetTag("linkType", linkTypeToString(properties.linkType));
    span->SetTag("transmissionType", transmissionTypeToString(properties.transmissionType));
    span->SetTag("connectionType", connectionTypeToString(properties.connectionType));
    span->SetTag("sendType", sendTypeToString(properties.sendType));
    span->SetTag("reliable", properties.reliable);
    span->SetTag("linkDirection", linkDirectionToString(chProperties.linkDirection));
}

SdkResponse RaceSdk::onPackageStatusChanged(CommsWrapper &plugin, RaceHandle handle,
                                            PackageStatus status, int32_t timeout) {
    TRACE_METHOD(plugin.getId(), handle, status, timeout);
    links->removeCachedPackageHandle(handle);

    auto [success, utilization] = getNM(handle)->onPackageStatusChanged(handle, status, timeout);
    auto sdkStatus = success ? SDK_OK : SDK_QUEUE_FULL;
    return {sdkStatus, utilization, 0};
}

SdkResponse RaceSdk::onConnectionStatusChanged(CommsWrapper &plugin, RaceHandle handle,
                                               ConnectionID connId, ConnectionStatus status,
                                               LinkProperties properties, int32_t timeout) {
    TRACE_METHOD(plugin.getId(), handle, connId, status, timeout);
    if (isShuttingDown) {
        helper::logInfo("onConnectionStatusChanged: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    if (networkManagerWrapper == nullptr) {
        helper::logError(
            "onConnectionStatusChanged: plugin for network manager has not been set for RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }

    if (connId == "") {
        helper::logError("onConnectionStatusChanged: invalid connId: \"" + connId + "\"");
        return SDK_INVALID_ARGUMENT;
    }

    LinkID linkId;
    try {
        linkId = links->getLinkIDFromConnectionID(connId);
    } catch (const std::invalid_argument &error) {
        helper::logError("tried to get LinkID from invalid ConnectionID: " + connId);
        return SDK_INVALID_ARGUMENT;
    }

    // Check if the link properties provided by comms plugin are valid, unless the connection is
    // being marked as closed or unavailable, in which case we don't care about the link properties
    // at all.
    if (status != CONNECTION_CLOSED && status != CONNECTION_UNAVAILABLE) {
        if (doesLinkPropertiesContainUndef(properties, "onConnectionStatusChanged: ")) {
            helper::logError("onConnectionStatusChanged: invalid link properties");
            return SDK_INVALID_ARGUMENT;
        }
    }

    if (status == CONNECTION_OPEN) {
        helper::logDebug("onConnectionStatusChanged: received CONNECTION_OPEN for connection " +
                         connId);
        std::unique_lock<std::shared_mutex> connectionsWriteLock(connectionsReadWriteLock);
        try {
            links->addConnection(handle, connId);
        } catch (const std::invalid_argument &error) {
            helper::logError("onConnectionStatusChanged: connection ID \"" + connId +
                             "\" invalid argument: " + error.what());
            return SDK_INVALID_ARGUMENT;
        }

        // Add trace for connection creation
        std::string spanName = connectionStatusToString(status);
        const std::pair<std::uint64_t, std::uint64_t> traceIds = links->getTraceCtxForLink(linkId);
        auto ctx = spanContextFromIds(traceIds);
        std::shared_ptr<opentracing::Span> span =
            tracer->StartSpan(spanName, {opentracing::ChildOf(ctx.get())});
        span->SetTag("connectionId", connId);
        traceLinkStatus(span, linkId);

        // Save a reference to the opentracing context IDs
        links->addTraceCtxForConnection(connId, traceIdFromContext(span->context()),
                                        spanIdFromContext(span->context()));

        helper::logInfo("onConnectionStatusChanged: added connection with ID " + connId);
    } else if (status == CONNECTION_CLOSED) {
        helper::logDebug("onConnectionStatusChanged: received CONNECTION_CLOSED for connection " +
                         connId);

        std::unique_lock<std::shared_mutex> connectionsWriteLock(connectionsReadWriteLock);

        // handle pending messages before closing connection
        auto pkgHandles = links->getCachedPackageHandles(connId);
        for (RaceHandle pkgHandle : pkgHandles) {
            onPackageStatusChanged(plugin, pkgHandle, PACKAGE_FAILED_GENERIC, 0);
        }

        // Add trace for connection destruction
        std::string spanName = connectionStatusToString(status);
        const std::pair<std::uint64_t, std::uint64_t> traceIds =
            links->getTraceCtxForConnection(connId);
        auto ctx = spanContextFromIds(traceIds);
        std::shared_ptr<opentracing::Span> span =
            tracer->StartSpan(spanName, {opentracing::ChildOf(ctx.get())});
        span->SetTag("connectionId", connId);
        traceLinkStatus(span, linkId);
        span->Finish();

        links->removeConnectionRequest(handle);
        links->removeConnection(connId);
    } else if (status == CONNECTION_AVAILABLE) {
        helper::logDebug(
            "onConnectionStatusChanged: received CONNECTION_AVAILABLE for connection " + connId);
    } else if (status == CONNECTION_UNAVAILABLE) {
        helper::logDebug(
            "onConnectionStatusChanged: received CONNECTION_UNAVAILABLE for connection " + connId);
    } else {
        helper::logError(
            "RaceSdk::onConnectionStatusChanged: received invalid connection status: " +
            std::to_string(static_cast<int>(status)));
        return SDK_INVALID_ARGUMENT;
    }

    if (getBootstrapManager().onConnectionStatusChanged(handle, connId, status, properties)) {
        // This was a bootstrap connection managed by the sdk. Prevent propagation to network
        // manager.
        return SDK_OK;
    }

    auto [success, utilization] = getNM(handle)->onConnectionStatusChanged(
        handle, connId, status, linkId, properties, timeout);
    auto sdkStatus = success ? SDK_OK : SDK_QUEUE_FULL;
    return {sdkStatus, utilization, handle};
}

SdkResponse RaceSdk::onLinkStatusChanged(CommsWrapper &plugin, RaceHandle handle, LinkID linkId,
                                         LinkStatus status, LinkProperties properties,
                                         int32_t timeout) {
    TRACE_METHOD(plugin.getId(), handle, linkId, status, timeout);
    if (isShuttingDown) {
        helper::logInfo("onLinkStatusChanged: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    if (networkManagerWrapper == nullptr) {
        helper::logError(
            "onLinkStatusChanged: plugin for network manager has not been set for RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }

    if (linkId == "") {
        helper::logError("onLinkStatusChanged: invalid linkId: \"" + linkId + "\"");
        return SDK_INVALID_ARGUMENT;
    }

    // Check if the link properties provided by comms plugin are valid, unless the link is being
    // marked as destroyed, in which case we don't care about the link properties at all.
    if (status != LINK_DESTROYED) {
        if (doesLinkPropertiesContainUndef(properties, "onLinkStatusChanged: ")) {
            helper::logError("onLinkStatusChanged: invalid link properties");
            return SDK_INVALID_ARGUMENT;
        }
    }

    if (status == LINK_CREATED or status == LINK_LOADED) {
        try {
            auto address = links->completeNewLinkRequest(handle, linkId);
            helper::logLinkChange(linkId, status, links->getAllPersonasForLink(linkId));
            // If comms plugin did not set the address and it was a load, then fill the loaded
            // address in
            if (address != "" and properties.linkAddress == "") {
                properties.linkAddress = address;
            }
            links->updateLinkProperties(linkId, properties);
            if (!properties.channelGid.empty()) {
                channels->setLinkId(properties.channelGid, linkId);
            } else {
                helper::logError("Could not associate linkId " + linkId +
                                 " with empty channelGid.");
            }

            // Add a trace for link creation
            std::string spanName = linkStatusToString(status);
            std::shared_ptr<opentracing::Span> span = tracer->StartSpan(spanName);
            traceLinkStatus(span, linkId);
            // Save a reference to the opentracing context IDs
            links->addTraceCtxForLink(linkId, traceIdFromContext(span->context()),
                                      spanIdFromContext(span->context()));
        } catch (const std::invalid_argument &error) {
            helper::logError(
                "Handle " + std::to_string(handle) +
                " was not associated with previous createLink or loadLinkAddress call.");
            return SDK_INVALID_ARGUMENT;
        }
    } else if (status == LINK_DESTROYED) {
        helper::logLinkChange(linkId, status, links->getAllPersonasForLink(linkId));

        // close connections before removing and destroying link
        std::unordered_set<ConnectionID> connIds = links->getLinkConnections(linkId);
        for (ConnectionID connId : connIds) {
            onConnectionStatusChanged(plugin, handle, connId, CONNECTION_CLOSED, properties, 0);
        }

        // Add a trace for link destruction
        const std::pair<std::uint64_t, std::uint64_t> traceIds = links->getTraceCtxForLink(linkId);
        auto ctx = spanContextFromIds(traceIds);
        std::string spanName = linkStatusToString(status);
        std::shared_ptr<opentracing::Span> span =
            tracer->StartSpan(spanName, {opentracing::ChildOf(ctx.get())});
        traceLinkStatus(span, linkId);
        span->Finish();

        links->removeNewLinkRequest(handle, linkId);  // in case it was a pending request
        links->removeLink(linkId);
        if (!properties.channelGid.empty()) {
            channels->removeLinkId(properties.channelGid, linkId);
        } else {
            helper::logError("Could not remove associated linkId " + linkId +
                             " from empty channelGid.");
        }
    } else {
        helper::logError("RaceSdk::onLinkStatusChanged: received invalid link status: " +
                         std::to_string(static_cast<int>(status)));
        return SDK_INVALID_ARGUMENT;
    }

    // Check if this was called in response to an sdk request to open a bootstrap link
    // If so, it shouldn't propagate to the network manager
    if (getBootstrapManager().onLinkStatusChanged(handle, linkId, status, properties)) {
        return SDK_OK;
    }

    auto [success, utilization] =
        getNM(handle)->onLinkStatusChanged(handle, linkId, status, properties, timeout);
    auto sdkStatus = success ? SDK_OK : SDK_QUEUE_FULL;
    return {sdkStatus, utilization, handle};
}

SdkResponse RaceSdk::onChannelStatusChanged(CommsWrapper &plugin, RaceHandle handle,
                                            const std::string &channelGid, ChannelStatus status,
                                            const ChannelProperties &properties, int32_t timeout) {
    TRACE_METHOD(plugin.getId(), handle, channelGid, status, timeout);
    ChannelProperties chanProps = getChannelProperties(properties.channelGid);
    if (!channelStaticPropertiesEqual(chanProps, properties)) {
        helper::logError(
            "onChannelStatusChanged: static ChannelProperties passed in from comms plugin do not "
            "match "
            "RaceSdk ChannelProperties");
        helper::logError("Passed in ChannelProperties from comms plugin: ");
        helper::logError(channelPropertiesToString(properties));
        helper::logError("ChannelProperties with matching channelGid in RaceSdk: ");
        helper::logError(channelPropertiesToString(chanProps));
        return SDK_INVALID_ARGUMENT;
    }

    if (isShuttingDown) {
        helper::logInfo("onChannelStatusChanged: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    if (networkManagerWrapper == nullptr) {
        helper::logError(
            "onChannelStatusChanged: plugin for network manager has not been set for RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }

    std::string channelGidWithoutWhiteSpace = channelGid;
    channelGidWithoutWhiteSpace.erase(
        remove_if(channelGidWithoutWhiteSpace.begin(), channelGidWithoutWhiteSpace.end(), isspace),
        channelGidWithoutWhiteSpace.end());
    if (channelGidWithoutWhiteSpace.empty()) {
        helper::logError("onChannelStatusChanged: empty string provided for channel GID.");
        return SDK_INVALID_ARGUMENT;
    }

    if (status == CHANNEL_AVAILABLE) {
        int itemsErased = channelsActivateRequested.erase(channelGid);
        if (itemsErased == 0) {
            helper::logError(
                "onChannelStatusChanged: (handle: " + std::to_string(handle) +
                ") failed to update channel with GID: " + channelGid +
                " , channel set AVAILABLE without activateChannel call from network manager");
            return SDK_INVALID_ARGUMENT;
        }
    }

    if (channelsDisableRequested.erase(channelGid) != 0) {
        helper::logInfo("onChannelStatusChanged: channel " + channelGid + " was disabled");
        if (status != CHANNEL_UNAVAILABLE) {
            helper::logError("onChannelStatusChanged: disabled channel " + channelGid +
                             " did not properly deactivate, status was " +
                             channelStatusToString(status) + " instead of CHANNEL_UNAVAILABLE");
        }
        // Force status to disabled
        status = CHANNEL_DISABLED;
    }

    bool updated = channels->update(channelGid, status, properties);
    // updated fails if the channel was not registered before this
    if (!updated) {
        helper::logError("onChannelStatusChanged: (handle: " + std::to_string(handle) +
                         ") failed to update channel with GID: " + channelGid);
        return SDK_INVALID_ARGUMENT;
    }
    auto [success, utilization] =
        getNM(handle)->onChannelStatusChanged(handle, channelGid, status, properties, timeout);
    auto sdkStatus = success ? SDK_OK : SDK_QUEUE_FULL;

    if (status == CHANNEL_FAILED || status == CHANNEL_DISABLED) {
        // destroy links if channel failed
        std::vector<LinkID> linkIds = channels->getLinksForChannel(channelGid);
        LinkProperties defaultLinkProps;
        defaultLinkProps.channelGid = channelGid;
        for (LinkID &linkId : linkIds) {
            onLinkStatusChanged(plugin, handle, linkId, LINK_DESTROYED, defaultLinkProps, 0);
        }
    }

    return {sdkStatus, utilization, handle};
}

SdkResponse RaceSdk::updateLinkProperties(CommsWrapper &plugin, const std::string &linkId,
                                          const LinkProperties &properties, int32_t timeout) {
    TRACE_METHOD(plugin.getId(), linkId, properties.linkType, timeout);

    if (doesLinkPropertiesContainUndef(properties, "updateLinkProperties: ")) {
        helper::logError("updateLinkProperties: invalid link properties");
        return SDK_INVALID_ARGUMENT;
    }

    try {
        links->updateLinkProperties(linkId, properties);
        networkManagerWrapper->onLinkPropertiesChanged(linkId, properties, timeout);
        networkManagerTestHarness->onLinkPropertiesChanged(linkId, properties, timeout);
    } catch (const std::out_of_range &) {
        const std::string errorMessage =
            "RaceSdk::updateLinkProperties: Error: invalid link ID for call to "
            "updateLinkProperties: link ID = " +
            linkId;
        helper::logError(errorMessage);
        return SDK_INVALID_ARGUMENT;
    } catch (const std::invalid_argument &error) {
        helper::logError("RaceSdk::updateLinkProperties: failed to update link properties: " +
                         std::string(error.what()));
        return SDK_INVALID_ARGUMENT;
    }

    return SDK_OK;
}

LinkID RaceSdk::getLinkForConnection(ConnectionID connectionId) {
    TRACE_METHOD(connectionId);
    try {
        return links->getLinkForConnection(connectionId);
    } catch (std::invalid_argument) {
        helper::logError("Tried to get LinkID for ConnectionID: " + connectionId +
                         " but none existed");
        return "";
    }
}

ConnectionID RaceSdk::generateConnectionId(CommsWrapper &plugin, LinkID linkId) {
    TRACE_METHOD(plugin.getId(), linkId);
    if (linkId == "") {
        helper::logError("generateConnectionId: invalid linkId: \"" + linkId + "\"");
        return "";
    }

    static std::atomic<int> count = 0;
    return linkId + "/Connection_" + std::to_string(count++);
}

LinkID RaceSdk::generateLinkId(CommsWrapper &plugin, const std::string &channelGid) {
    TRACE_METHOD(plugin.getId(), channelGid);
    if (!channels->isAvailable(channelGid)) {
        helper::logError(
            "generateLinkId: request for LinkID on a channel that does not exist or is not set "
            "as "
            "available: " +
            channelGid);
        return "";
    }
    static std::atomic<int> count = 0;
    return plugin.getId() + "/" + channelGid + "/LinkID_" + std::to_string(count++);
}

SdkResponse RaceSdk::serveFiles(LinkID linkId, const std::string &path, int32_t timeout) {
    TRACE_METHOD(linkId, path, timeout);

    if (isShuttingDown) {
        helper::logInfo("openConnection: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    std::string pluginName = RaceLinks::getPluginFromLinkID(linkId);
    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    if (!commsWrappers.count(pluginName)) {
        helper::logError("Error: plugin for link could not be found in RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }

    try {
        SdkResponse response = commsWrappers[pluginName]->serveFiles(linkId, path, timeout);

        return response;
    } catch (const std::out_of_range &error) {
        helper::logError("openConnection: invalid link ID \"" + linkId +
                         "\" for call to openConnection: " + error.what());
        return SDK_INVALID_ARGUMENT;
    } catch (const std::invalid_argument &error) {
        helper::logError("openConnection: invalid link ID \"" + linkId +
                         "\" for call to openConnection: " + error.what());
        return SDK_INVALID_ARGUMENT;
    }

    return SDK_OK;
}

static std::string setOfStringsToString(const std::unordered_set<std::string> &setOfStrings) {
    if (setOfStrings.size() == 0) {
        return "{empty set}";
    }

    auto iter = setOfStrings.begin();
    std::string result = *(iter++);
    for (; iter != setOfStrings.end(); ++iter) {
        result += ", " + *iter;
    }
    return result;
}

SdkResponse RaceSdk::receiveEncPkg(CommsWrapper &plugin, const EncPkg &pkg,
                                   const std::vector<ConnectionID> &connIDs, int32_t timeout) {
    TRACE_METHOD(plugin.getId(), timeout);
    if (std::any_of(connIDs.begin(), connIDs.end(),
                    [](ConnectionID connId) { return connId == ""; })) {
        helper::logError("receiveEncPkg: invalid connId: \"\"");
        return SDK_INVALID_ARGUMENT;
    }

    const std::string logPrefx =
        "receiveEncPkg (connection IDs: " + vectorToString(connIDs) + "): ";
    helper::logDebug("Package size = " + std::to_string(pkg.getCipherText().size()));
    helper::logDebug("Package type = " + packageTypeToString(pkg.getPackageType()));

    if (isShuttingDown) {
        helper::logInfo("receiveEncPkg: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    if (networkManagerWrapper == nullptr) {
        helper::logError("receiveEncPkg: plugin for network manager has not been set for RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }

    if (connIDs.size() == 0) {
        helper::logError("receiveEncPkg: didn't get any connection IDs. What do I do now?");
    }

    if (pkg.getPackageType() == PKG_TYPE_UNDEF) {
        helper::logError("receiveEncPkg: received encrypted package with unset package type");
    }

    std::shared_lock<std::shared_mutex> connectionsReadLock(connectionsReadWriteLock);

    const auto closedConnections =
        links->doConnectionsExist(std::unordered_set<ConnectionID>{connIDs.begin(), connIDs.end()});
    if (closedConnections.size() == connIDs.size()) {
        helper::logError("receiveEncPkg: none of the provided connections are still open: " +
                         setOfStringsToString(closedConnections));
        return SDK_INVALID_ARGUMENT;
    } else {
        std::vector<ConnectionID> filteredConnectionIds = connIDs;
        if (closedConnections.size() > 0) {
            helper::logWarning("receiveEncPkg: some of the provided connections are closed: " +
                               setOfStringsToString(closedConnections));
            for (const auto &connection : closedConnections) {
                filteredConnectionIds.erase(std::find(filteredConnectionIds.begin(),
                                                      filteredConnectionIds.end(), connection));
            }
        }

        // Add trace for connection use for each connection Id
        for (auto connId : filteredConnectionIds) {
            const std::pair<std::uint64_t, std::uint64_t> traceIds =
                links->getTraceCtxForConnection(connId);
            auto ctx = spanContextFromIds(traceIds);
            std::shared_ptr<opentracing::Span> span =
                tracer->StartSpan("CONNECTION_RECV", {opentracing::ChildOf(ctx.get())});
            span->SetTag("connectionId", connId);
            span->SetTag("size", pkg.getSize());
            LinkID linkId = links->getLinkForConnection(connId);
            traceLinkStatus(span, linkId);
            span->Finish();
        }

        if (pkg.getPackageType() == PKG_TYPE_SDK) {
            LinkID linkId = getLinkForConnection(connIDs.front());
            getBootstrapManager().onReceiveEncPkg(pkg, linkId, timeout);
            return {SDK_OK, 0, NULL_RACE_HANDLE};
        } else {
            RaceHandle handle = generateHandle(pkg.getPackageType() == PKG_TYPE_TEST_HARNESS);
            auto [success, utilization] =
                getNM(handle)->processEncPkg(handle, pkg, filteredConnectionIds, timeout);
            auto sdkStatus = success ? SDK_OK : SDK_QUEUE_FULL;
            return {sdkStatus, utilization, handle};
        }
    }
}

RaceHandle RaceSdk::sendClientMessage(ClrMsg msg) {
    TRACE_METHOD();

    if (isShuttingDown) {
        helper::logInfo("sendClientMessage: sdk is shutting down");
        return NULL_RACE_HANDLE;
    }

    if (networkManagerWrapper == nullptr) {
        helper::logError("plugin for network manager has not been set for RaceSdk.");
        return NULL_RACE_HANDLE;
    }

    RaceHandle handle = generateHandle(false);
    auto [success, utilization] = networkManagerWrapper->processClrMsg(handle, msg, RACE_BLOCKING);
    if (!success) {
        helper::logError("sendClientMessage: networkManager processClrMsg failed. Utilization: " +
                         std::to_string(utilization));
    }

    return handle;
}

void RaceSdk::sendNMBypassMessage(ClrMsg msg, const std::string &route) {
    TRACE_METHOD(route);

    if (isShuttingDown) {
        helper::logInfo("sendNMBypassMessage: sdk is shutting down");
        return;
    }

    RaceHandle handle = generateHandle(true);
    auto [success, utilization] =
        networkManagerTestHarness->processNMBypassMsg(handle, msg, route, RACE_BLOCKING);
    if (!success) {
        helper::logError(
            "sendClientMessage: networkManager bypass processClrMsg failed. Utilization: " +
            std::to_string(utilization));
    }
}

void RaceSdk::openNMBypassReceiveConnection(const std::string &persona, const std::string &route) {
    TRACE_METHOD(persona, route);

    if (isShuttingDown) {
        helper::logInfo("openNMBypassReceiveConnection: sdk is shutting down");
        return;
    }

    RaceHandle handle = generateHandle(true);
    auto [success, utilization] =
        networkManagerTestHarness->openRecvConnection(handle, persona, route, RACE_BLOCKING);
    if (!success) {
        helper::logError(
            "openNMBypassReceiveConnection: networkManager bypass openRecvConnection failed.");
    }
}

void RaceSdk::rpcDeactivateChannel(const std::string &channelGid) {
    TRACE_METHOD(channelGid);

    if (isShuttingDown) {
        helper::logInfo(logPrefix + "sdk is shutting down");
        return;
    }

    auto [success, utilization] =
        networkManagerTestHarness->rpcDeactivateChannel(channelGid, RACE_BLOCKING);
    if (!success) {
        helper::logError(logPrefix + "failed");
    }
}

void RaceSdk::rpcDestroyLink(const std::string &linkId) {
    TRACE_METHOD(linkId);

    if (isShuttingDown) {
        helper::logInfo(logPrefix + "sdk is shutting down");
        return;
    }

    auto [success, utilization] = networkManagerTestHarness->rpcDestroyLink(linkId, RACE_BLOCKING);
    if (!success) {
        helper::logError(logPrefix + "failed");
    }
}

void RaceSdk::rpcCloseConnection(const std::string &connectionId) {
    TRACE_METHOD(connectionId);

    if (isShuttingDown) {
        helper::logInfo(logPrefix + "sdk is shutting down");
        return;
    }

    auto [success, utilization] =
        networkManagerTestHarness->rpcCloseConnection(connectionId, RACE_BLOCKING);
    if (!success) {
        helper::logError(logPrefix + "failed");
    }
}

void RaceSdk::rpcNotifyEpoch(const std::string &data) {
    TRACE_METHOD(data);

    if (isShuttingDown) {
        helper::logInfo(logPrefix + "sdk is shutting down");
        return;
    }

    auto [success, utilization] = networkManagerWrapper->notifyEpoch(data, RACE_BLOCKING);
    if (!success) {
        helper::logError(logPrefix + "failed");
    }
}

bool RaceSdk::validateDeviceInfo(const DeviceInfo &deviceInfo) {
    if (!(deviceInfo.platform == "linux" && deviceInfo.architecture == "x86_64") &&
        !(deviceInfo.platform == "android" && deviceInfo.architecture == "x86_64") &&
        !(deviceInfo.platform == "android" && deviceInfo.architecture == "arm64-v8a")) {
        helper::logError("validateDeviceInfo: Invalid platform/arch: " + deviceInfo.platform + "/" +
                         deviceInfo.architecture);
        return false;
    }
    if (!(deviceInfo.nodeType == "client" && deviceInfo.platform == "android") &&
        !(deviceInfo.nodeType == "client" && deviceInfo.platform == "linux") &&
        !(deviceInfo.nodeType == "server" && deviceInfo.platform == "linux")) {
        helper::logError("validateDeviceInfo: Invalid nodeType/platform: " + deviceInfo.nodeType +
                         "/" + deviceInfo.platform);
        return false;
    }

    return true;
}

RaceHandle RaceSdk::prepareToBootstrap(DeviceInfo deviceInfo, std::string passphrase,
                                       std::string bootstrapChannelId) {
    TRACE_METHOD(passphrase);
    return getBootstrapManager().prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelId);
}

bool RaceSdk::cancelBootstrap(RaceHandle handle) {
    TRACE_METHOD();
    return getBootstrapManager().cancelBootstrap(handle);
}

bool RaceSdk::onBootstrapFinished(RaceHandle bootstrapHandle, BootstrapState state) {
    TRACE_METHOD(bootstrapHandle, state);

    if (isShuttingDown) {
        helper::logInfo(logPrefix + " sdk is shutting down");
        return false;
    }

    if (networkManagerWrapper == nullptr) {
        helper::logError(logPrefix + " plugin for network manager has not been set for RaceSdk.");
        return false;
    }
    return networkManagerWrapper->onBootstrapFinished(bootstrapHandle, state);
}

bool RaceSdk::createBootstrapLink(RaceHandle handle, const std::string &passphrase,
                                  const std::string &bootstrapChannelId) {
    TRACE_METHOD(handle, passphrase, bootstrapChannelId);
    auto channelProps = channels->getSupportedChannels();
    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    CommsWrapper *channel = nullptr;
    std::string channelId;
    for (auto &channelProp : channelProps) {
        helper::logInfo(logPrefix + "Checking if channel is bootstrap/local: " + channelProp.first);
        if (channelProp.second.bootstrap == true && channelProp.second.connectionType == CT_LOCAL) {
            try {
                helper::logInfo(logPrefix + "bootstrap channel: " + channelProp.first);
                std::string pluginName = channels->getWrapperIdForChannel(channelProp.first);
                channelId = channelProp.first;
                if ((bootstrapChannelId.empty()) || (bootstrapChannelId == channelId)) {
                    channel = commsWrappers.at(pluginName).get();
                    helper::logInfo(logPrefix + "Using bootstrap channel: " + channelId);
                    break;
                }
            } catch (std::exception &e) {
                helper::logError(logPrefix + "Failed to get plugin for channel: " +
                                 channelProp.first + ". what: " + e.what());
                return false;
            }
        } else {
            helper::logInfo(logPrefix + "non-bootstrap, non-local channel: " + channelProp.first);
        }
    }

    if (channel == nullptr) {
        helper::logError(logPrefix + "Failed to find bootstrap plugin");
        return false;
    }

    helper::logInfo(logPrefix + "Got bootstrap channel: " + channelId);

    links->addNewLinkRequest(handle, {}, "");
    SdkResponse response =
        channel->createBootstrapLink(handle, channelId, passphrase, RACE_BLOCKING);
    if (response.status != SDK_OK) {
        helper::logError(logPrefix + "createBootstrapLink failed: channel: " + channelId +
                         " status: " + sdkStatusToString(response.status));

        return false;
    }
    return true;
}

std::vector<std::string> RaceSdk::getContacts() {
    TRACE_METHOD();
    personas::PersonaSet personas = links->getAllPersonaSet();
    return std::vector<std::string>(personas.begin(), personas.end());
}

bool RaceSdk::isConnected() {
    return isReady;
}

RaceHandle RaceSdk::generateHandle(bool testHarness) {
    if (testHarness) {
        // If we overflow back to 0, go back to the starting test harness handle
        RaceHandle overflow = 0u;
        testHarnessHandleCount.compare_exchange_strong(overflow, START_TEST_HARNESS_HANDLE);
        return testHarnessHandleCount++;
    } else {
        // If we get up to the starting test harness handle, reset back to 1
        RaceHandle overflow = START_TEST_HARNESS_HANDLE;
        networkManagerPluginHandleCount.compare_exchange_strong(overflow, 1u);
        return networkManagerPluginHandleCount++;
    }
}

void RaceSdk::cleanShutdown() {
    TRACE_METHOD();
    isShuttingDown = true;
    // TODO: finish implementation.
    shutdownPlugins();

    if (appWrapper != nullptr) {
        appWrapper->stopHandler();
    }

    if (raceConfig.isVoaEnabled) {
        if (voaThread != nullptr) {
            voaThread->stopThread();
        }
    }
}

void RaceSdk::notifyShutdown(int numSeconds) {
    TRACE_METHOD(numSeconds);
    isShuttingDown = true;
    // TODO: finish implementation.
    // TODO: same as cleanShutdown? Just have one function for shutting down?
}

void RaceSdk::shutdownPlugins() {
    TRACE_METHOD();

    // TODO: errors
    if (networkManagerWrapper != nullptr) {
        helper::logDebug("shutdownPlugins: network manager plugin calling shutdown...");
        networkManagerWrapper->shutdown();
        helper::logDebug("shutdownPlugins: network manager plugin shutdown returned");
    }

    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    runEachComms(commsWrappers, [](auto &commsWrapper) {
        helper::logDebug("shutdownPlugins: comms plugin " + commsWrapper->getId() +
                         " calling shutdown...");
        commsWrapper->shutdown();
        helper::logDebug("shutdownPlugins: comms plugin " + commsWrapper->getId() +
                         " shutdown returned");
    });
}

void RaceSdk::destroyPlugins() {
    TRACE_METHOD();

    if (networkManagerWrapper != nullptr) {
        helper::logDebug("destroyPlugins: destroying network manager plugin...");
        networkManagerWrapper->stopHandler();
        networkManagerWrapper.reset();
        helper::logDebug("destroyPlugins: network manager plugin destroyed");
    }

    helper::logDebug("destroyPlugins: destroying comms plugins...");
    {
        std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
        runEachComms(commsWrappers, [](auto &commsWrapper) { commsWrapper->stopHandler(); });
    }

    {
        std::unique_lock<std::shared_mutex> commsWrapperWriteLock(commsWrapperReadWriteLock);
        commsWrappers.clear();
    }
    helper::logDebug("destroyPlugins: comms plugins destroyed");
}

void RaceSdk::cleanupChannels(CommsWrapper &plugin) {
    TRACE_METHOD(plugin.getId());

    std::vector<std::string> channelIds = channels->getPluginChannelIds(plugin.getId());

    for (auto channelId : channelIds) {
        helper::logDebug(logPrefix + "channel: " + channelId + " failed");
        ChannelProperties channelProps = getChannelProperties(channelId);

        // race conditions between package failure (network manager trying to send package), and
        // connection close addressed by locking the connectionsReadWriteLock in
        // onChannelStatusChanged

        // this will indicate pending packet failure, then close connections and destroy links
        onChannelStatusChanged(plugin, NULL_RACE_HANDLE, channelId, CHANNEL_FAILED, channelProps,
                               0);
    }
}

void RaceSdk::shutdownPluginAsync(CommsWrapper &plugin) {
    TRACE_METHOD(plugin.getId());
    // We need to make sure the new thread grabs the read lock before this thread releases it.
    // Otherwise it is possible the plugin could get deleted by a different thread and the
    // reference would become invalid.
    std::promise<void> promise;

    std::thread([this, &plugin, &promise, logPrefix] {
        std::string id;
        {
            std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
            promise.set_value();
            id = plugin.getId();

            helper::logDebug(logPrefix + "comms plugin " + id + ": calling shutdown...");
            plugin.shutdown(CommsWrapper::WAIT_FOREVER);
            helper::logDebug(logPrefix + "comms plugin " + id + ": shutdown returned");

            helper::logDebug(logPrefix + " comms plugin " + id + " cleaning up channels");
            cleanupChannels(plugin);
            helper::logDebug(logPrefix + "comms plugin " + id + " clean up returned");

            helper::logDebug(logPrefix + "comms plugin " + id + ": stopping plugin...");
            plugin.stopHandler();
            helper::logDebug(logPrefix + "comms plugin " + id + ": plugin stopped");
        }

        // it's okay to release the lock and grab the write lock because this block doesn't use
        // the plugin. If the plugin was deleted in the mean time, trying to erase it does nothing.
        {
            std::unique_lock<std::shared_mutex> commsWrapperWriteLock(commsWrapperReadWriteLock);
            helper::logDebug(logPrefix + "comms plugin " + id + ": destroying plugin...");
            commsWrappers.erase(id);
            helper::logDebug(logPrefix + "comms plugin " + id + ": plugin destroyed");
        }
    }).detach();

    // wait until the other thread has grabbed the read lock
    promise.get_future().wait();
}

void RaceSdk::shutdownPluginInternal(CommsWrapper &plugin) {
    TRACE_METHOD(plugin.getId());
    std::string id = plugin.getId();

    helper::logDebug(logPrefix + "comms plugin " + id + " calling shutdown...");
    plugin.shutdown(CommsWrapper::WAIT_FOREVER);
    helper::logDebug(logPrefix + "comms plugin " + id + " shutdown returned");

    helper::logDebug(logPrefix + "comms plugin " + id + " cleaning up channels");
    cleanupChannels(plugin);
    helper::logDebug(logPrefix + "comms plugin " + id + " clean up returned");

    helper::logDebug(logPrefix + "comms plugin " + id + " stopping plugin...");
    plugin.stopHandler();
    helper::logDebug(logPrefix + "comms plugin " + id + " plugin stopped");

    helper::logDebug(logPrefix + "comms plugin " + id + " destroying plugin...");
    commsWrappers.erase(id);
    helper::logDebug(logPrefix + "comms plugin " + id + " plugin destroying");
}

bool RaceSdk::doesLinkPropertiesContainUndef(const LinkProperties &props,
                                             const std::string &logPrefix) {
    bool result = false;
    if (props.linkType == LT_UNDEF) {
        helper::logWarning(logPrefix + "link properties linkType = LT_UNDEF");
        result = true;
    }
    if (props.transmissionType == TT_UNDEF) {
        helper::logWarning(logPrefix + "link properties transmissionType = TT_UNDEF");
        result = true;
    }
    if (props.connectionType == CT_UNDEF) {
        helper::logWarning(logPrefix + "link properties connectionType = CT_UNDEF");
        result = true;
    }
    if (props.sendType == ST_UNDEF) {
        helper::logWarning(logPrefix + "link properties sendType = ST_UNDEF");
        result = true;
    }
    return result;
}

void RaceSdk::shutdownCommsAndCrash() {
    TRACE_METHOD();
    isShuttingDown = true;

    std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
    runEachComms(commsWrappers, [](auto &commsWrapper) {
        helper::logDebug("shutdownCommsAndCrash: comms plugin " + commsWrapper->getId() +
                         " calling shutdown...");
        commsWrapper->shutdown();
        helper::logDebug("shutdownCommsAndCrash: comms plugin " + commsWrapper->getId() +
                         " shutdown returned");
    });

    helper::logDebug("shutdownCommsAndCrash: crashing");
    abort();
}

SdkResponse RaceSdk::requestPluginUserInput(const std::string &pluginId, bool isTestHarness,
                                            const std::string &key, const std::string &prompt,
                                            bool cache) {
    TRACE_METHOD(pluginId, isTestHarness, key, prompt);

    RaceHandle handle = generateHandle(isTestHarness);
    {
        std::lock_guard<std::mutex> handlesWriteLock(userInputHandlesLock);
        userInputHandles[handle] = pluginId;
    }
    SdkResponse response = appWrapper->requestUserInput(handle, pluginId, key, prompt, cache);
    // If not able to post to the user input queue, clean up the handle mapping
    if (response.status != SDK_OK) {
        std::lock_guard<std::mutex> handlesWriteLock(userInputHandlesLock);
        userInputHandles.erase(handle);
    }

    return response;
}

SdkResponse RaceSdk::requestCommonUserInput(const std::string &pluginId, bool isTestHarness,
                                            const std::string &key) {
    TRACE_METHOD(pluginId, isTestHarness, key);

    if (not appWrapper->isValidCommonKey(key)) {
        helper::logWarning("RaceSdk::requestCommonUserInput: invalid key: " + key);
        return SDK_INVALID_ARGUMENT;
    }

    RaceHandle handle = generateHandle(isTestHarness);
    {
        std::lock_guard<std::mutex> handlesWriteLock(userInputHandlesLock);
        userInputHandles[handle] = pluginId;
    }
    // TODO: We're currently reusing the key as the prompt because we don't actually display
    // this in the UI. When the update to display this is made, this should be updated
    SdkResponse response = appWrapper->requestUserInput(handle, "Common", key, key, true);
    // If not able to post to the user input queue, clean up the handle mapping
    if (response.status != SDK_OK) {
        std::lock_guard<std::mutex> handlesWriteLock(userInputHandlesLock);
        userInputHandles.erase(handle);
    }

    return response;
}

SdkResponse RaceSdk::onUserAcknowledgementReceived(RaceHandle handle) {
    TRACE_METHOD(handle);

    if (isShuttingDown) {
        helper::logInfo("onUserAcknowledgementReceived: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    std::string pluginId;
    {
        std::lock_guard<std::mutex> lock(userInputHandlesLock);
        auto iter = userInputHandles.find(handle);
        if (iter == userInputHandles.end()) {
            helper::logError("Error: no user display acknowledgement handle mapping found");
            return SDK_PLUGIN_MISSING;
        }
        pluginId = iter->second;
        userInputHandles.erase(iter);
    }

    // Check first if this handle was generated/associated with the sdk, then check the network
    // manager plugin, and comms plugins
    if (pluginId == "sdk") {
        helper::logDebug("onUserAcknowledgementReceived: sdk received acknowledgment");
        return SDK_OK;
    } else if (pluginId == getNM(handle)->getId()) {
        auto [success, utilization] = getNM(handle)->onUserAcknowledgementReceived(handle, 0);
        auto sdkStatus = success ? SDK_OK : SDK_QUEUE_FULL;
        return {sdkStatus, utilization, handle};
    } else {
        std::shared_lock<std::shared_mutex> commsWrapperReadLock(commsWrapperReadWriteLock);
        auto iter = commsWrappers.find(pluginId);
        if (iter == commsWrappers.end()) {
            helper::logError("Error: Comms plugin could not be found in RaceSdk.");
            return SDK_PLUGIN_MISSING;
        }
        auto [success, utilization] = iter->second->onUserAcknowledgementReceived(handle, 0);
        auto sdkStatus = success ? SDK_OK : SDK_QUEUE_FULL;
        return {sdkStatus, utilization, handle};
    }
    // TODO acknowledgements and user input responses need to go to AMP as well

    return SDK_INVALID_ARGUMENT;
}

SdkResponse RaceSdk::displayInfoToUser(const std::string &pluginId, const std::string &data,
                                       RaceEnums::UserDisplayType displayType) {
    TRACE_METHOD(pluginId, data, displayType);

    RaceHandle handle = generateHandle(false);
    {
        std::lock_guard<std::mutex> handlesWriteLock(userInputHandlesLock);
        userInputHandles[handle] = pluginId;
    }
    SdkResponse response = appWrapper->displayInfoToUser(handle, data, displayType);
    // If not able to post to the user input queue, clean up the handle mapping
    if (response.status != SDK_OK) {
        std::lock_guard<std::mutex> handlesWriteLock(userInputHandlesLock);
        userInputHandles.erase(handle);
    }

    return response;
}
SdkResponse RaceSdk::displayBootstrapInfoToUser(const std::string &pluginId,
                                                const std::string &data,
                                                RaceEnums::UserDisplayType displayType,
                                                RaceEnums::BootstrapActionType actionType) {
    TRACE_METHOD(pluginId, data, displayType, actionType);

    RaceHandle handle = generateHandle(false);
    {
        std::lock_guard<std::mutex> handlesWriteLock(userInputHandlesLock);
        userInputHandles[handle] = pluginId;
    }
    SdkResponse response =
        appWrapper->displayBootstrapInfoToUser(handle, data, displayType, actionType);
    // If not able to post to the user input queue, clean up the handle mapping
    if (response.status != SDK_OK) {
        std::lock_guard<std::mutex> handlesWriteLock(userInputHandlesLock);
        userInputHandles.erase(handle);
    }

    return response;
}

std::string RaceSdk::getAppPath(const std::string & /* pluginId */) {
    return getAppConfig().appPath;
}

SdkResponse RaceSdk::sendAmpMessage(const std::string &pluginId, const std::string &destination,
                                    const std::string &message) {
    TRACE_METHOD(pluginId, destination, message);

    if (isShuttingDown) {
        helper::logInfo("onUserAcknowledgementReceived: sdk is shutting down");
        return SDK_SHUTTING_DOWN;
    }

    if (networkManagerWrapper == nullptr) {
        helper::logError("plugin for network manager has not been set for RaceSdk.");
        return SDK_PLUGIN_MISSING;
    }

    int8_t ampIndex = 0;
    auto ids = artifactManager->getIds();
    for (auto &ampId : ids) {
        helper::logError(ampId);
        if (ampId == pluginId) {
            break;
        }
        ampIndex++;
    }

    if (static_cast<size_t>(ampIndex) >= ids.size()) {
        helper::logError("Invalid plugin id");
        return SDK_INVALID_ARGUMENT;
    }

    // The index we pass is the actual index + 1. 0 is reserved for non-amp messages.
    ampIndex = ampIndex + 1;

    json wrappedAmpMessage = {{"ampIndex", ampIndex}, {"body", message}};

    ClrMsg msg(wrappedAmpMessage.dump(), getActivePersona(), destination, 0, 0, 0);

    RaceHandle handle = generateHandle(false);
    auto [success, utilization] = networkManagerWrapper->processClrMsg(handle, msg, RACE_BLOCKING);
    if (!success) {
        helper::logError("sendClientMessage: networkManager processClrMsg failed. Utilization: " +
                         std::to_string(utilization));
    }

    return SDK_OK;
}

void RaceSdk::initializeConfigsFromTarGz(const std::string &configTarGz,
                                         const std::string &destDir) {
    TRACE_METHOD(configTarGz, destDir);

    // Check if the SDK data dir exists and is not empty. If so, this means configs have already
    // been extracted and decrypted on a previous start and nothing needs to be done.
    const std::string sdkDataDir = destDir + "/sdk/";
    helper::logDebug(logPrefix + "checking directory exists and is not empty: " + sdkDataDir);
    if (fs::exists(sdkDataDir) && !fs::is_empty(sdkDataDir)) {
        helper::logDebug(
            logPrefix + "SDK data dir at: " + sdkDataDir +
            " exists and is not empty. Will use existing configs contained in this directory.");
        return;
    }
    helper::logDebug(logPrefix + "no configs found at: " + sdkDataDir);

    helper::logDebug(logPrefix + "extracting... " + configTarGz);

    // Extract the configs tar. Destination dir is the full data path. Because the tar contains a
    // dir called data/configs we need to go up one level.
    helper::extractConfigTarGz(configTarGz, destDir + "/../../");

    helper::logDebug(logPrefix + "extracted  " + configTarGz);
    fs::remove(configTarGz);
    helper::logDebug(logPrefix + "removed  " + configTarGz);

    // Encrypt each of the files extracted from the tar.
    // Iterate over the /data/ directory and encrypt EVERYTHING!
    try {
        fs::path dataDir(destDir);

        for (auto &iter : fs::recursive_directory_iterator(dataDir)) {
            auto currentPath = iter.path().string();

            if (!StorageEncryption::isFileEncryptable(iter.path().filename().string())) {
                continue;
            }

            if (!fs::is_directory(iter.path())) {
                // Read the file to a byte vector.
                std::ifstream file(currentPath.c_str(), std::ifstream::in);
                std::stringstream fileBuffer;
                fileBuffer << file.rdbuf();
                std::string fileString = fileBuffer.str();
                std::vector<std::uint8_t> fileData(fileString.begin(), fileString.end());

                helper::logDebug(logPrefix + "encrypting file: " + currentPath + " ...");

                // Encrypt and overwrite the file.
                pluginStorageEncryption.write(currentPath, fileData);

                helper::logDebug(logPrefix + "encrypted file: " + currentPath);
            }
        }

    } catch (const fs::filesystem_error &error) {
        helper::logError(logPrefix +
                         "failed to encrypt configs files: " + std::string(error.what()));
        throw error;
    }
}
