
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

#include "BootstrapManager.h"

#include <RaceLog.h>
#include <base64.h>

#include <exception>
#include <fstream>
#include <string>
#include <unordered_set>

#include "ArtifactManager.h"
#include "ArtifactManagerWrapper.h"
#include "BootstrapState.h"
#include "BootstrapThread.h"
#include "NMWrapper.h"
#include "RaceSdk.h"
#include "helper.h"

#define EXPECT_STATE(bootstrap, expected_state, ...)                               \
    if ((bootstrap).state != BootstrapInfo::expected_state) {                      \
        helper::logError(logPrefix + " Bootstrap failed due to unexpected state"); \
        (bootstrap).state = BootstrapInfo::FAILED;                                 \
        manager.removePendingBootstrap(bootstrap);                                 \
        return __VA_ARGS__;                                                        \
    }

BootstrapInfo::BootstrapInfo(const DeviceInfo &deviceInfo, const std::string &passphrase,
                             const std::string &bootstrapChannelId) :
    deviceInfo(deviceInfo),
    state{State::INITIALIZED},
    passphrase(passphrase),
    bootstrapChannelId(bootstrapChannelId) {}

BootstrapInstanceManager::BootstrapInstanceManager(
    BootstrapManager &manager, std::shared_ptr<FileSystemHelper> fileSystemHelper) :
    manager(manager),
    fileSystemHelper(fileSystemHelper),
    bootstrapThread(std::make_unique<BootstrapThread>(manager, fileSystemHelper)) {}

RaceHandle BootstrapInstanceManager::handleBootstrapStart(BootstrapInfo &bootstrap) {
    TRACE_METHOD();
    EXPECT_STATE(bootstrap, INITIALIZED, false);

    RaceHandle handle = prepareToBootstrap(bootstrap);
    if (handle == NULL_RACE_HANDLE) {
        helper::logError(logPrefix + "Failed. Removing bootstrap.");
        bootstrap.state = BootstrapInfo::FAILED;
        manager.removePendingBootstrap(bootstrap);
    }
    return handle;
}

void BootstrapInstanceManager::handleLinkCreated(BootstrapInfo &bootstrap, const LinkID &linkId) {
    TRACE_METHOD(linkId);
    EXPECT_STATE(bootstrap, WAITING_FOR_LINK);

    manager.sdk.displayBootstrapInfoToUser("sdk", "Preparing bootstrap configs...",
                                           RaceEnums::UD_NOTIFICATION,
                                           RaceEnums::BS_PREPARING_CONFIGS);

    bootstrap.bootstrapLink = linkId;
    auto [success, utilization] = manager.sdk.getNM()->prepareToBootstrap(
        bootstrap.prepareBootstrapHandle, linkId, "bootstrap-files/" + bootstrap.timeSinceEpoch,
        bootstrap.deviceInfo, RACE_BLOCKING);

    if (!success) {
        helper::logError(logPrefix +
                         "networkManagerWrapper::prepareToBootstrap failed. Utilization: " +
                         std::to_string(utilization));
        bootstrap.state = BootstrapInfo::FAILED;
        manager.removePendingBootstrap(bootstrap);
        return;
    }

    bootstrap.state = BootstrapInfo::WAITING_FOR_NM;
}

void BootstrapInstanceManager::handleNMReady(const std::shared_ptr<BootstrapInfo> &bootstrap,
                                             std::vector<std::string> commsChannels) {
    TRACE_METHOD();
    EXPECT_STATE(*bootstrap, WAITING_FOR_NM);

    // Copy Network Manager Configs
    const auto &appConfig = manager.sdk.getAppConfig();
    auto &pluginStorage = manager.sdk.getPluginStorage();
    if (!fileSystemHelper->copyAndDecryptDir(
            appConfig.baseConfigPath + "/" + manager.sdk.getNM()->getId() + "/bootstrap-files/" +
                bootstrap->timeSinceEpoch,
            bootstrap->bootstrapPath + "/data/configs/" + manager.sdk.getNM()->getId(),
            pluginStorage)) {
        helper::logError("Failed to copy network manager bootstrap configs");
        return;
    }

    // Convert list of channels to plugins (and deduplicate them)
    std::set<std::string> pluginSet;
    try {
        for (auto &channel : commsChannels) {
            for (auto &plugin : manager.sdk.channels->getPluginsForChannel(channel)) {
                pluginSet.insert(plugin);
            }
        }
    } catch (std::exception &e) {
        helper::logError(logPrefix +
                         "Could not convert channel to plugin: " + std::string(e.what()));
        bootstrap->state = BootstrapInfo::FAILED;
        manager.removePendingBootstrap(*bootstrap);
        return;
    }

    bootstrap->commsPlugins = {pluginSet.begin(), pluginSet.end()};

    // Fetch network manager plugin, comms plugins, RACE app, and ArtifactManager plugins
    std::vector<std::string> artifacts(pluginSet.begin(), pluginSet.end());
    artifacts.push_back(manager.sdk.getNM()->getId());
    artifacts.push_back("race");

    if (auto artifactManager = manager.sdk.getArtifactManager(); artifactManager != nullptr) {
        auto artifactManagerPluginIds = artifactManager->getIds();
        artifacts.insert(artifacts.end(), artifactManagerPluginIds.begin(),
                         artifactManagerPluginIds.end());
    }

    for (auto &artifact : artifacts) {
        helper::logInfo(logPrefix + "Bootstrapping node with artifact: " + artifact);
    }

    bool success = bootstrapThread->fetchArtifacts(artifacts, bootstrap);

    if (!success) {
        helper::logWarning(logPrefix + "fetchArtifacts failed");
        if (bootstrap->state != BootstrapInfo::CANCELLED) {
            bootstrap->state = BootstrapInfo::FAILED;
        }
        manager.removePendingBootstrap(*bootstrap);
        return;
    }

    bootstrap->connectionHandle = manager.sdk.generateHandle(false);
    success = bootstrapThread->serveFiles(bootstrap->bootstrapLink, bootstrap);

    if (!success) {
        helper::logWarning(logPrefix + "serveFiles failed");
        if (bootstrap->state != BootstrapInfo::CANCELLED) {
            bootstrap->state = BootstrapInfo::FAILED;
        }
        manager.removePendingBootstrap(*bootstrap);
        return;
    }

    bootstrap->state = BootstrapInfo::WAITING_FOR_BOOTSTRAP_PKG;
}

void BootstrapInstanceManager::handleConnectionOpened(BootstrapInfo &bootstrap,
                                                      const ConnectionID &connId) {
    TRACE_METHOD(connId);
    EXPECT_STATE(bootstrap, WAITING_FOR_BOOTSTRAP_PKG);
    // There's not an explicit waiting for open connection state. Should there be?
    bootstrap.bootstrapConnection = connId;
}

bool BootstrapInstanceManager::handleBootstrapPkgReceived(BootstrapInfo &bootstrap,
                                                          const EncPkg &pkg, int32_t timeout) {
    TRACE_METHOD();
    EXPECT_STATE(bootstrap, WAITING_FOR_BOOTSTRAP_PKG, false);

    auto [success, persona, key] = parseBootstrapPkg(pkg);
    if (!success) {
        // Not a bootstrap package
        return false;
    }

    manager.sdk.links->setPersonasForLink(bootstrap.bootstrapLink, {persona});
    manager.sdk.getNM()->onBootstrapPkgReceived(persona, key, timeout);
    if (not closeBootstrapConnection(bootstrap)) {
        bootstrap.state = BootstrapInfo::FAILED;
        manager.removePendingBootstrap(bootstrap);
        // This was a valid bootstrap package so return true even though it failed
    }
    return true;
}

bool BootstrapInstanceManager::closeBootstrapConnection(BootstrapInfo &bootstrap) {
    TRACE_METHOD();
    helper::logInfo(logPrefix + "closing bootstrap connection: " + bootstrap.bootstrapConnection);
    SdkResponse resp = manager.sdk.closeConnection(*manager.sdk.getNM(),
                                                   bootstrap.bootstrapConnection, RACE_UNLIMITED);
    if (resp.status == SDK_OK) {
        bootstrap.connectionHandle = resp.handle;
    } else {
        helper::logError("Failed to close bootstrap connection");
        return false;
    }

    bootstrap.state = BootstrapInfo::WAITING_FOR_CONNECTION_CLOSED;
    return true;
}

void BootstrapInstanceManager::handleConnectionClosed(BootstrapInfo &bootstrap) {
    TRACE_METHOD();
    EXPECT_STATE(bootstrap, WAITING_FOR_CONNECTION_CLOSED);

    bootstrap.bootstrapConnection = "";
    bootstrap.state = BootstrapInfo::SUCCESS;
    manager.removePendingBootstrap(bootstrap);
}

void BootstrapInstanceManager::handleLinkFailed(BootstrapInfo &bootstrap, const LinkID &linkId) {
    TRACE_METHOD(linkId);
    helper::logError(logPrefix + "Bootstrap failed due to link closed");
    bootstrap.state = BootstrapInfo::FAILED;
    manager.removePendingBootstrap(bootstrap);
}

void BootstrapInstanceManager::handleNMFailed(BootstrapInfo &bootstrap) {
    TRACE_METHOD();
    helper::logError(logPrefix +
                     "Bootstrap failed due to network manager calling bootstrap failed");
    bootstrap.state = BootstrapInfo::FAILED;
    manager.removePendingBootstrap(bootstrap);
}

void BootstrapInstanceManager::handleCancelled(BootstrapInfo &bootstrap) {
    TRACE_METHOD();
    bootstrap.state = BootstrapInfo::CANCELLED;
    manager.removePendingBootstrap(bootstrap);
}

void BootstrapInstanceManager::handleServeFilesFailed(BootstrapInfo &bootstrap) {
    TRACE_METHOD();
    helper::logError(logPrefix + "Bootstrap failed due to serve files failing");
    bootstrap.state = BootstrapInfo::FAILED;
    manager.removePendingBootstrap(bootstrap);
}

void BootstrapInstanceManager::cleanupBootstrap(const std::shared_ptr<BootstrapInfo> &bootstrap) {
    TRACE_METHOD();
    bootstrapThread->onBootstrapFinished(bootstrap);
    if (not bootstrap->bootstrapConnection.empty()) {
        closeBootstrapConnection(*bootstrap);
    }
}

RaceHandle BootstrapInstanceManager::prepareToBootstrap(BootstrapInfo &bootstrap) {
    manager.sdk.displayBootstrapInfoToUser("sdk", "Preparing bootstrap bundle...",
                                           RaceEnums::UD_NOTIFICATION,
                                           RaceEnums::BS_PREPARING_BOOTSTRAP);

    bootstrap.prepareBootstrapHandle = NULL_RACE_HANDLE;
    if (!createBootstrapDirectories(bootstrap)) {
        return NULL_RACE_HANDLE;
    }
    if (!populateGlobalDirectories(bootstrap)) {
        return NULL_RACE_HANDLE;
    }
    if (!createBootstrapLink(bootstrap)) {
        return NULL_RACE_HANDLE;
    }
    bootstrap.prepareBootstrapHandle = manager.sdk.generateHandle(false);
    bootstrap.state = BootstrapInfo::WAITING_FOR_LINK;
    return bootstrap.prepareBootstrapHandle;
}

bool BootstrapInstanceManager::createBootstrapDirectories(BootstrapInfo &bootstrap) {
    TRACE_METHOD();

    // NOTE: this manager creates the bootstrap dirs but relinquishes ownership to the thread
    // Consider putting directory creation in the bootstrap thread
    const auto &appConfig = manager.sdk.getAppConfig();
    std::chrono::duration<double> sinceEpoch = std::chrono::system_clock::now().time_since_epoch();
    bootstrap.timeSinceEpoch = std::to_string(sinceEpoch.count());
    bootstrap.bootstrapPath = appConfig.bootstrapFilesDirectory + "/" + bootstrap.timeSinceEpoch;
    try {
        helper::logDebug(logPrefix + " creating bootstrap dir " + bootstrap.bootstrapPath);
        fs::create_directories(bootstrap.bootstrapPath + "/data/configs");
        fs::create_directories(bootstrap.bootstrapPath + "/data/configs/sdk");
        fs::create_directories(bootstrap.bootstrapPath + "/data/configs/sdk/global");
        fs::create_directories(bootstrap.bootstrapPath + "/data/configs/" +
                               manager.sdk.getNM()->getId());
        fs::create_directories(bootstrap.bootstrapPath + "/data/configs/" +
                               manager.sdk.getNM()->getId() + "/global");
        fs::create_directories(bootstrap.bootstrapPath + "/artifacts");
        fs::create_directories(bootstrap.bootstrapPath + "/artifacts/network-manager");
        fs::create_directories(bootstrap.bootstrapPath + "/artifacts/comms");
        fs::create_directories(bootstrap.bootstrapPath + "/artifacts/artifact-manager");
        return true;
    } catch (std::exception &e) {
        helper::logError("Failed to create subdirectories within " + bootstrap.bootstrapPath +
                         ". what: " + e.what());
        return false;
    }
}

bool BootstrapInstanceManager::populateGlobalDirectories(const BootstrapInfo &bootstrap) {
    TRACE_METHOD();
    const auto &appConfig = manager.sdk.getAppConfig();
    auto &pluginStorage = manager.sdk.getPluginStorage();
    if (!fileSystemHelper->copyAndDecryptDir(appConfig.baseConfigPath + "/sdk/",
                                             bootstrap.bootstrapPath + "/data/configs/sdk",
                                             pluginStorage)) {
        helper::logError("Failed to copy configs for SDK");
        return false;
    }

    return true;
}

bool BootstrapInstanceManager::createBootstrapLink(BootstrapInfo &bootstrap) {
    TRACE_METHOD();
    bootstrap.createdLinkHandle = manager.sdk.generateHandle(false);
    // TODO: We still have the bootstrap lock when we call this. Is there potential for a deadlock?
    if (!manager.sdk.createBootstrapLink(bootstrap.createdLinkHandle, bootstrap.passphrase,
                                         bootstrap.bootstrapChannelId)) {
        helper::logError(logPrefix + "Failed to create bootstrap link");
        return false;
    }
    return true;
}

std::tuple<bool, std::string, RawData> BootstrapInstanceManager::parseBootstrapPkg(
    const EncPkg &bootstrapPkg) {
    TRACE_METHOD();
    try {
        auto ciphertext = bootstrapPkg.getCipherText();
        std::string jsonString = {ciphertext.begin(), ciphertext.end()};
        nlohmann::json info = nlohmann::json::parse(jsonString);
        std::string persona = info.at("persona").get<std::string>();
        RawData key = base64::decode(info.at("key").get<std::string>());

        return {true, persona, key};
    } catch (nlohmann::json::exception &error) {
        helper::logError("parseBootstrapPkg: failed to parse bootstrap package: " +
                         std::string(error.what()));
    } catch (std::exception &error) {
        helper::logError("parseBootstrapPkg: failed to parse bootstrap package: " +
                         std::string(error.what()));
    }

    return {false, {}, {}};
}

BootstrapManager::BootstrapManager(RaceSdk &_sdk) :
    fileSystemHelper(std::make_shared<FileSystemHelper>()), sdk(_sdk) {
    bsInstanceManager = std::make_unique<BootstrapInstanceManager>(*this, fileSystemHelper);
}

BootstrapManager::BootstrapManager(RaceSdk &_sdk,
                                   std::shared_ptr<FileSystemHelper> _fileSystemHelper) :
    fileSystemHelper(_fileSystemHelper), sdk(_sdk) {
    bsInstanceManager = std::make_unique<BootstrapInstanceManager>(*this, fileSystemHelper);
}

RaceHandle BootstrapManager::prepareToBootstrap(DeviceInfo deviceInfo, std::string passphrase,
                                                std::string bootstrapChannelId) {
    TRACE_METHOD(passphrase, bootstrapChannelId);
    std::lock_guard lock(bootstrapLock);

    if (!validateDeviceInfo(deviceInfo)) {
        helper::logError(logPrefix + "Invalid device info passed to prepareToBootstrap");
        return NULL_RACE_HANDLE;
    }

    auto bootstrap = std::make_shared<BootstrapInfo>(deviceInfo, passphrase, bootstrapChannelId);
    bootstraps.push_back(bootstrap);
    return bsInstanceManager->handleBootstrapStart(*bootstrap);
}

bool BootstrapManager::onLinkStatusChanged(RaceHandle handle, LinkID linkId, LinkStatus status,
                                           LinkProperties /*properties*/) {
    TRACE_METHOD(handle, linkId, status);
    std::lock_guard lock(bootstrapLock);

    // Check if this was called in response to an sdk request to open a bootstrap link
    auto it = std::find_if(bootstraps.begin(), bootstraps.end(), [handle](const auto &info) {
        return handle == info->createdLinkHandle;
    });
    if (it != bootstraps.end()) {
        auto bootstrap = *it;
        helper::logInfo(logPrefix + "received update for bootstrap link");
        // this call is in response to a bootstrapDevice call
        if (status == LINK_CREATED) {
            helper::logInfo(logPrefix + "received LINK_CREATED for bootstrap");
            bsInstanceManager->handleLinkCreated(*bootstrap, linkId);
        } else if (status == LINK_DESTROYED) {
            helper::logError(logPrefix + "received unexpected LINK_DESTROYED for bootstrap link");
            bsInstanceManager->handleLinkFailed(*bootstrap, linkId);
        } else {
            helper::logError(logPrefix +
                             "received invalid link status response to bootstrap link request: " +
                             std::to_string(static_cast<int>(status)));
            bsInstanceManager->handleLinkFailed(*bootstrap, linkId);
        }

        // This was a bootstrap link
        return true;
    }
    return false;
}

bool BootstrapManager::onConnectionStatusChanged(RaceHandle handle, ConnectionID connId,
                                                 ConnectionStatus status,
                                                 LinkProperties /*properties*/) {
    TRACE_METHOD(handle, connId, status);
    std::lock_guard lock(bootstrapLock);

    // TODO: This only handles expected changes due to calls to the comms. Does it also need to
    // handle unexpected changes, e.g. a connection closing on its own?
    auto it = std::find_if(bootstraps.begin(), bootstraps.end(),
                           [handle](const auto &info) { return handle == info->connectionHandle; });

    if (it != bootstraps.end()) {
        auto bootstrap = *it;
        helper::logInfo(logPrefix + "received update for bootstrap link");
        if (status == CONNECTION_OPEN) {
            helper::logInfo(logPrefix + "bootstrap connection opened = " + connId);
            bsInstanceManager->handleConnectionOpened(*bootstrap, connId);
        } else if (status == CONNECTION_CLOSED) {
            helper::logInfo(logPrefix +
                            "bootstrap link connection is closed, cleaning up bootstrap info");
            bsInstanceManager->handleConnectionClosed(*bootstrap);
        }

        // This was a bootstrap connection
        return true;
    }
    return false;
}

bool BootstrapManager::onReceiveEncPkg(const EncPkg &pkg, const LinkID &linkId, int32_t timeout) {
    TRACE_METHOD();
    std::lock_guard lock(bootstrapLock);
    auto it = std::find_if(bootstraps.begin(), bootstraps.end(),
                           [linkId](const auto &info) { return linkId == info->bootstrapLink; });
    if ((it != bootstraps.end()) && !(*it)->bootstrapConnection.empty()) {
        auto bootstrap = *it;
        helper::logInfo(logPrefix + "received package on bootstrap connection " +
                        bootstrap->bootstrapConnection);
        return bsInstanceManager->handleBootstrapPkgReceived(*bootstrap, pkg, timeout);
    }
    return false;
}

bool BootstrapManager::bootstrapDevice(RaceHandle handle, std::vector<std::string> commsChannels) {
    TRACE_METHOD(handle);
    std::lock_guard lock(bootstrapLock);
    auto it = std::find_if(bootstraps.begin(), bootstraps.end(), [handle](const auto &info) {
        return handle == info->prepareBootstrapHandle;
    });
    if (it != bootstraps.end()) {
        auto bootstrap = *it;
        bsInstanceManager->handleNMReady(bootstrap, commsChannels);
        return true;
    } else {
        helper::logError(logPrefix + "could not find handle " + std::to_string(handle) +
                         " in pending bootstraps");
        return false;
    }
}

bool BootstrapManager::bootstrapFailed(RaceHandle handle) {
    TRACE_METHOD(handle);
    std::lock_guard lock(bootstrapLock);
    auto it = std::find_if(bootstraps.begin(), bootstraps.end(), [handle](const auto &info) {
        return handle == info->prepareBootstrapHandle;
    });
    if (it != bootstraps.end()) {
        auto bootstrap = *it;
        bsInstanceManager->handleNMFailed(*bootstrap);
        return true;
    } else {
        helper::logError(logPrefix + "could not find handle " + std::to_string(handle) +
                         " in pending bootstraps");
        return false;
    }
}

bool BootstrapManager::cancelBootstrap(RaceHandle handle) {
    TRACE_METHOD();
    std::lock_guard lock(bootstrapLock);

    auto bootstrap = std::find_if(
        bootstraps.begin(), bootstraps.end(),
        [&handle](const auto &info) { return handle == info->prepareBootstrapHandle; });

    if (bootstrap == bootstraps.end()) {
        helper::logError(logPrefix + " bootstrap handle \'" + std::to_string(handle) +
                         "\' not found");
        return false;
    } else {
        bsInstanceManager->handleCancelled(*bootstrap->get());
    }
    return true;
}

bool BootstrapManager::onServeFilesFailed(const BootstrapInfo &failedBootstrap) {
    TRACE_METHOD(failedBootstrap.prepareBootstrapHandle);
    std::lock_guard lock(bootstrapLock);
    auto it =
        std::find_if(bootstraps.begin(), bootstraps.end(), [&failedBootstrap](const auto &info) {
            return failedBootstrap.prepareBootstrapHandle == info->prepareBootstrapHandle;
        });
    if ((it != bootstraps.end())) {
        auto bootstrap = *it;
        bsInstanceManager->handleServeFilesFailed(*bootstrap);
        return true;
    }
    return false;
}

bool BootstrapManager::validateDeviceInfo(const DeviceInfo &deviceInfo) {
    TRACE_METHOD(deviceInfo.platform, deviceInfo.architecture, deviceInfo.nodeType);
    if (!(deviceInfo.platform == "linux" && deviceInfo.architecture == "x86_64") &&
        !(deviceInfo.platform == "linux" && deviceInfo.architecture == "arm64-v8a") &&
        !(deviceInfo.platform == "android" && deviceInfo.architecture == "x86_64") &&
        !(deviceInfo.platform == "android" && deviceInfo.architecture == "arm64-v8a")) {
        helper::logError(logPrefix + "Invalid platform/arch: " + deviceInfo.platform + "/" +
                         deviceInfo.architecture);
        return false;
    }
    if (!(deviceInfo.nodeType == "client" && deviceInfo.platform == "android") &&
        !(deviceInfo.nodeType == "client" && deviceInfo.platform == "linux") &&
        !(deviceInfo.nodeType == "server" && deviceInfo.platform == "linux")) {
        helper::logError(logPrefix + "Invalid nodeType/platform: " + deviceInfo.nodeType + "/" +
                         deviceInfo.platform);
        return false;
    }

    return true;
}

void BootstrapManager::removePendingBootstrap(const BootstrapInfo &bootstrap) {
    TRACE_METHOD(bootstrap.prepareBootstrapHandle);

    auto it = std::find_if(bootstraps.begin(), bootstraps.end(), [&bootstrap](const auto &info) {
        return bootstrap.prepareBootstrapHandle == info->prepareBootstrapHandle;
    });

    if (it != bootstraps.end()) {
        BootstrapState finalState;
        switch (bootstrap.state) {
            case BootstrapInfo::CANCELLED:
                finalState = BootstrapState::BOOTSTRAP_CANCELLED;
                break;
            case BootstrapInfo::SUCCESS:
                finalState = BootstrapState::BOOTSTRAP_SUCCESS;
                break;
            case BootstrapInfo::FAILED:
                finalState = BootstrapState::BOOTSTRAP_FAILED;
                break;
            default:
                helper::logInfo(logPrefix + " unexpected internal bootstrap state " +
                                std::to_string(bootstrap.state));
                finalState = BootstrapState::BOOTSTRAP_INVALID;
                break;
        }
        sdk.onBootstrapFinished(it->get()->prepareBootstrapHandle, finalState);
        bsInstanceManager->cleanupBootstrap(*it);
        bootstraps.erase(it);
    } else {
        helper::logInfo(logPrefix + " no record of bootstrap to cleanup");
    }

    if (bootstrap.state == BootstrapInfo::SUCCESS) {
        sdk.displayBootstrapInfoToUser("sdk", "Bootstrap completed", RaceEnums::UD_NOTIFICATION,
                                       RaceEnums::BS_COMPLETE);
    } else if (bootstrap.state == BootstrapInfo::CANCELLED) {
        sdk.displayBootstrapInfoToUser("sdk", "Bootstrap cancelled", RaceEnums::UD_NOTIFICATION,
                                       RaceEnums::BS_FAILED);
    } else {
        sdk.displayBootstrapInfoToUser("sdk", "Bootstrap failed", RaceEnums::UD_NOTIFICATION,
                                       RaceEnums::BS_FAILED);
    }
}

BootstrapThread *BootstrapManager::getBootstrapThread() {
    if (bsInstanceManager.get() != nullptr) {
        return bsInstanceManager->getBootstrapThread();
    }
    return nullptr;
}
