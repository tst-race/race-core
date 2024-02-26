
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

#ifndef __MOCK_BOOTSTRAP_MANAGER_H__
#define __MOCK_BOOTSTRAP_MANAGER_H__

#include "../../include/BootstrapManager.h"
#include "LogExpect.h"
#include "gmock/gmock.h"
#include "race_printers.h"

// TODO move this to it's own file
class MockFileSystemHelper : public FileSystemHelper {
public:
    bool copyAndDecryptDir(const std::string &, const std::string &, StorageEncryption &) override {
        return true;
    }
};

class MockBootstrapInstanceManager : public BootstrapInstanceManager {
public:
    MockBootstrapInstanceManager(LogExpect &logger, BootstrapManager &manager,
                                 std::shared_ptr<FileSystemHelper> fileSystemHelper) :
        BootstrapInstanceManager(manager, fileSystemHelper), logger(logger) {
        using ::testing::_;
        ON_CALL(*this, handleBootstrapStart(_)).WillByDefault([this](BootstrapInfo &bootstrap) {
            LOG_EXPECT(this->logger, "handleBootstrapStart", bootstrap);
            return false;
        });
        ON_CALL(*this, handleLinkCreated(_, _))
            .WillByDefault([this](BootstrapInfo &bootstrap, const LinkID &linkId) {
                LOG_EXPECT(this->logger, "handleLinkCreated", bootstrap, linkId);
            });
        ON_CALL(*this, handleConnectionOpened(_, _))
            .WillByDefault([this](BootstrapInfo &bootstrap, const ConnectionID &connId) {
                LOG_EXPECT(this->logger, "handleConnectionOpened", bootstrap, connId);
            });
        ON_CALL(*this, handleConnectionClosed(_)).WillByDefault([this](BootstrapInfo &bootstrap) {
            LOG_EXPECT(this->logger, "handleConnectionClosed", bootstrap);
        });
        ON_CALL(*this, handleBootstrapPkgReceived(_, _, _))
            .WillByDefault([this](BootstrapInfo &bootstrap, const EncPkg &pkg, int32_t timeout) {
                LOG_EXPECT(this->logger, "handleBootstrapPkgReceived", bootstrap, pkg, timeout);
                return true;
            });
        ON_CALL(*this, handleNMReady(_, _))
            .WillByDefault([this](const std::shared_ptr<BootstrapInfo> &bootstrap,
                                  std::vector<std::string> commsChannels) {
                json channels = commsChannels;
                LOG_EXPECT(this->logger, "handleNMReady", *bootstrap, channels);
            });
        ON_CALL(*this, handleLinkFailed(_, _))
            .WillByDefault([this](BootstrapInfo &bootstrap, const LinkID &linkId) {
                LOG_EXPECT(this->logger, "handleLinkFailed", bootstrap, linkId);
            });
        ON_CALL(*this, handleNMFailed(_)).WillByDefault([this](BootstrapInfo &bootstrap) {
            LOG_EXPECT(this->logger, "handleNMFailed", bootstrap);
        });
        ON_CALL(*this, handleServeFilesFailed(_)).WillByDefault([this](BootstrapInfo &bootstrap) {
            LOG_EXPECT(this->logger, "handleServeFilesFailed", bootstrap);
        });
    }

    MOCK_METHOD(RaceHandle, handleBootstrapStart, (BootstrapInfo & bootstrap), (override));
    MOCK_METHOD(void, handleLinkCreated, (BootstrapInfo & bootstrap, const LinkID &linkId),
                (override));
    MOCK_METHOD(void, handleConnectionOpened,
                (BootstrapInfo & bootstrap, const ConnectionID &connId), (override));
    MOCK_METHOD(void, handleConnectionClosed, (BootstrapInfo & bootstrap), (override));
    MOCK_METHOD(bool, handleBootstrapPkgReceived,
                (BootstrapInfo & bootstrap, const EncPkg &pkg, int32_t timeout), (override));
    MOCK_METHOD(void, handleNMReady,
                (const std::shared_ptr<BootstrapInfo> &bootstrapInfo,
                 std::vector<std::string> commsChannels),
                (override));
    MOCK_METHOD(void, handleLinkFailed, (BootstrapInfo & bootstrap, const LinkID &linkId),
                (override));
    MOCK_METHOD(void, handleNMFailed, (BootstrapInfo & bootstrap), (override));
    MOCK_METHOD(void, handleCancelled, (BootstrapInfo & bootstrap), (override));
    MOCK_METHOD(void, handleServeFilesFailed, (BootstrapInfo & bootstrap), (override));

    LogExpect &logger;

    using BootstrapInstanceManager::bootstrapThread;
};

class MockBootstrapManager : public BootstrapManager {
public:
    MockBootstrapManager(LogExpect &logger, RaceSdk &sdk,
                         std::shared_ptr<FileSystemHelper> fileSystemHelper) :
        BootstrapManager(sdk, fileSystemHelper), logger(logger) {
        using ::testing::_;
        ON_CALL(*this, prepareToBootstrap(_, _, _))
            .WillByDefault([this](DeviceInfo deviceInfo, std::string passphrase,
                                  std::string bootstrapChannelId) {
                LOG_EXPECT(this->logger, "prepareToBootstrap", deviceInfo, passphrase,
                           bootstrapChannelId);
                return 12345;
            });
        ON_CALL(*this, onLinkStatusChanged(_, _, _, _))
            .WillByDefault([this](RaceHandle handle, LinkID linkId, LinkStatus status,
                                  LinkProperties properties) {
                LOG_EXPECT(this->logger, "onLinkStatusChanged", handle, linkId, status, properties);
                return false;
            });
        ON_CALL(*this, onConnectionStatusChanged(_, _, _, _))
            .WillByDefault([this](RaceHandle handle, ConnectionID connId, ConnectionStatus status,
                                  LinkProperties properties) {
                LOG_EXPECT(this->logger, "onConnectionStatusChanged", handle, connId, status,
                           properties);
                return false;
            });
        ON_CALL(*this, onReceiveEncPkg(_, _, _))
            .WillByDefault([this](const EncPkg &pkg, const LinkID &linkId, int32_t timeout) {
                LOG_EXPECT(this->logger, "onReceiveEncPkg", pkg, linkId, timeout);
                return false;
            });
        ON_CALL(*this, bootstrapDevice(_, _))
            .WillByDefault([this](RaceHandle handle, std::vector<std::string> commsChannels) {
                json channels = commsChannels;
                LOG_EXPECT(this->logger, "bootstrapDevice", handle, channels);
                return false;
            });
        ON_CALL(*this, bootstrapFailed(_)).WillByDefault([this](RaceHandle handle) {
            LOG_EXPECT(this->logger, "bootstrapFailed", handle);
            return false;
        });
        ON_CALL(*this, onServeFilesFailed(_))
            .WillByDefault([this](const BootstrapInfo &failedBootstrap) {
                LOG_EXPECT(this->logger, "onServeFilesFailed", failedBootstrap);
                return false;
            });
        ON_CALL(*this, removePendingBootstrap(_))
            .WillByDefault([this](const BootstrapInfo &failedBootstrap) {
                LOG_EXPECT(this->logger, "removePendingBootstrap", failedBootstrap);
            });
    }
    MOCK_METHOD(RaceHandle, prepareToBootstrap,
                (DeviceInfo deviceInfo, std::string passphrase, std::string bootstrapChannelId),
                (override));
    MOCK_METHOD(bool, onLinkStatusChanged,
                (RaceHandle handle, LinkID linkId, LinkStatus status, LinkProperties properties),
                (override));
    MOCK_METHOD(bool, onConnectionStatusChanged,
                (RaceHandle handle, ConnectionID connId, ConnectionStatus status,
                 LinkProperties properties),
                (override));
    MOCK_METHOD(bool, onReceiveEncPkg, (const EncPkg &pkg, const LinkID &linkId, int32_t timeout),
                (override));
    MOCK_METHOD(bool, bootstrapDevice, (RaceHandle handle, std::vector<std::string> commsChannels),
                (override));
    MOCK_METHOD(bool, bootstrapFailed, (RaceHandle handle), (override));
    MOCK_METHOD(bool, cancelBootstrap, (RaceHandle handle), (override));
    MOCK_METHOD(bool, onServeFilesFailed, (const BootstrapInfo &failedBootstrap), (override));

    MOCK_METHOD(void, removePendingBootstrap, (const BootstrapInfo &failedBootstrap), (override));

    LogExpect &logger;

    using BootstrapManager::bsInstanceManager;
};

class MockBootstrapThread : public BootstrapThread {
public:
    MockBootstrapThread(LogExpect &logger, BootstrapManager &manager,
                        std::shared_ptr<FileSystemHelper> fileSystemHelper) :
        BootstrapThread(manager, fileSystemHelper), logger(logger) {
        using ::testing::_;
        ON_CALL(*this, fetchArtifacts(_, _))
            .WillByDefault([this](std::vector<std::string> artifacts,
                                  const std::shared_ptr<BootstrapInfo> &bootstrapInfo) {
                json artifactsList = artifacts;
                const std::string &platform = bootstrapInfo->deviceInfo.platform;
                const std::string &architecture = bootstrapInfo->deviceInfo.architecture;
                const std::string &nodeType = bootstrapInfo->deviceInfo.nodeType;
                LOG_EXPECT(this->logger, "fetchArtifacts", artifactsList, platform, architecture,
                           nodeType);
                return true;
            });
        ON_CALL(*this, serveFiles(_, _))
            .WillByDefault(
                [this](const LinkID &linkId, const std::shared_ptr<BootstrapInfo> &bootstrapInfo) {
                    LOG_EXPECT(this->logger, "serveFiles", linkId, *bootstrapInfo);
                    return true;
                });
        ON_CALL(*this, waitForCallbacks()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "waitForCallbacks");
            return true;
        });
    }
    MOCK_METHOD(bool, fetchArtifacts,
                (std::vector<std::string> artifacts,
                 const std::shared_ptr<BootstrapInfo> &bootstrapInfo),
                (override));
    MOCK_METHOD(bool, serveFiles,
                (const LinkID &linkId, const std::shared_ptr<BootstrapInfo> &bootstrapInfo),
                (override));
    MOCK_METHOD(void, waitForCallbacks, (), (override));
    LogExpect &logger;
};

#endif
