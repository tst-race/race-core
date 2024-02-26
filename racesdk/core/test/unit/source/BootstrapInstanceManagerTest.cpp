
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

#include "../../../include/BootstrapManager.h"
#include "../../../source/BootstrapThread.h"
#include "../../common/LogExpect.h"
#include "../../common/MockBootstrapManager.h"
#include "../../common/MockNMWrapper.h"
#include "../../common/MockRaceChannels.h"
#include "../../common/MockRaceLinks.h"
#include "../../common/MockRaceSdk.h"
#include "../../common/helpers.h"
#include "../../common/race_printers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;
using ::testing::Return;

class TestBootstrapInstanceManager : public BootstrapInstanceManager {
public:
    explicit TestBootstrapInstanceManager(BootstrapManager &_manager,
                                          std::shared_ptr<FileSystemHelper> _fileSystemHelper) :
        BootstrapInstanceManager(_manager, _fileSystemHelper) {}

    using BootstrapInstanceManager::bootstrapThread;
};

class BootstrapInstanceManagerTestFixture : public ::testing::Test {
public:
    BootstrapInstanceManagerTestFixture() :
        test_info(testing::UnitTest::GetInstance()->current_test_info()),
        logger(test_info->test_suite_name(), test_info->name()),
        appConfig(createDefaultAppConfig()),
        raceConfig(createDefaultRaceConfig()),
        sdk(appConfig, raceConfig),
        networkManager(logger, sdk),
        fileSystemHelper(std::make_shared<MockFileSystemHelper>()),
        manager(logger, sdk, fileSystemHelper),
        // bsInstanceManager(manager.bsInstanceManager.get()) {
        bsInstanceManager(
            std::make_shared<TestBootstrapInstanceManager>(manager, fileSystemHelper)) {
        manager.bsInstanceManager = bsInstanceManager;
        bsInstanceManager->bootstrapThread =
            std::make_unique<MockBootstrapThread>(logger, manager, fileSystemHelper);

        ON_CALL(sdk, createBootstrapLink(_, _, _))
            .WillByDefault([this](RaceHandle createdLinkHandle, std::string passphrase,
                                  std::string bootstrapChannelId) {
                LOG_EXPECT(this->logger, "createBootstrapLink", createdLinkHandle, passphrase,
                           bootstrapChannelId);
                return true;
            });
        ON_CALL(sdk, getNM()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getNM");
            return &networkManager;
        });

        auto channels = std::make_unique<MockRaceChannels>(logger);
        ON_CALL(*channels, getPluginsForChannel("MockComms-0/channel1"))
            .WillByDefault([this](const std::string &channelGid) {
                LOG_EXPECT(this->logger, "getPluginsForChannel", channelGid);
                return std::vector<std::string>{"MockComms-0"};
            });
        ON_CALL(*channels, getPluginsForChannel("MockComms-1/channel2"))
            .WillByDefault([this](const std::string &channelGid) {
                LOG_EXPECT(this->logger, "getPluginsForChannel", channelGid);
                return std::vector<std::string>{"MockComms-1"};
            });
        sdk.channels = std::move(channels);

        auto links = std::make_unique<MockRaceLinks>(logger);
        sdk.links = std::move(links);

        ::testing::DefaultValue<SdkResponse>::Set(SDK_OK);
        createAppDirectories(appConfig);
    }
    virtual ~BootstrapInstanceManagerTestFixture() {}
    virtual void SetUp() override {}
    virtual void TearDown() override {
        logger.check();
    }

    void waitForCallbacks() {}

public:
    const testing::TestInfo *test_info;
    LogExpect logger;
    AppConfig appConfig;
    RaceConfig raceConfig;
    MockRaceSdk sdk;
    MockNMWrapper networkManager;
    std::shared_ptr<MockFileSystemHelper> fileSystemHelper;
    MockBootstrapManager manager;
    std::shared_ptr<TestBootstrapInstanceManager> bsInstanceManager;
};

TEST_F(BootstrapInstanceManagerTestFixture, handleBootstrapStart) {
    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    ASSERT_EQ(bsInstanceManager->handleBootstrapStart(bootstrap), 2);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleBootstrapStart_invalid_state) {
    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    bootstrap.state = BootstrapInfo::FAILED;
    ASSERT_EQ(bsInstanceManager->handleBootstrapStart(bootstrap), NULL_RACE_HANDLE);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleBootstrapStart_failed) {
    EXPECT_CALL(sdk, createBootstrapLink(_, _, _))
        .WillOnce([this](RaceHandle createdLinkHandle, std::string passphrase,
                         std::string bootstrapChannelId) {
            LOG_EXPECT(this->logger, "createBootstrapLink", createdLinkHandle, passphrase,
                       bootstrapChannelId);
            return false;
        });
    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    ASSERT_EQ(bsInstanceManager->handleBootstrapStart(bootstrap), NULL_RACE_HANDLE);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleLinkCreated) {
    LinkID linkId = "link-id";
    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    bootstrap.state = BootstrapInfo::WAITING_FOR_LINK;
    bsInstanceManager->handleLinkCreated(bootstrap, linkId);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleLinkCreated_invalid_state) {
    LinkID linkId = "link-id";
    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    bootstrap.state = BootstrapInfo::FAILED;
    bsInstanceManager->handleLinkCreated(bootstrap, linkId);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleConnectionOpened) {
    ConnectionID connId = "conn-id";
    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    bootstrap.state = BootstrapInfo::WAITING_FOR_BOOTSTRAP_PKG;
    bsInstanceManager->handleConnectionOpened(bootstrap, connId);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleConnectionOpened_invalid_state) {
    ConnectionID connId = "conn-id";
    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    bootstrap.state = BootstrapInfo::FAILED;
    bsInstanceManager->handleConnectionOpened(bootstrap, connId);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleConnectionClosed) {
    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    bootstrap.state = BootstrapInfo::WAITING_FOR_CONNECTION_CLOSED;
    bsInstanceManager->handleConnectionClosed(bootstrap);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleConnectionClosed_invalid_state) {
    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    bootstrap.state = BootstrapInfo::FAILED;
    bsInstanceManager->handleConnectionClosed(bootstrap);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleBootstrapPkgReceived) {
    std::string contentsString = R"({
        "persona": "personaName",
        "key": "YWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWE="
    })";
    EncPkg pkg(0, 0, {contentsString.begin(), contentsString.end()});
    pkg.setPackageType(PKG_TYPE_SDK);

    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    bootstrap.state = BootstrapInfo::WAITING_FOR_BOOTSTRAP_PKG;
    bsInstanceManager->handleBootstrapPkgReceived(bootstrap, pkg, 0);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleBootstrapPkgReceived_invalid_state) {
    EncPkg pkg(0, 0, {});

    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    bootstrap.state = BootstrapInfo::FAILED;
    bsInstanceManager->handleBootstrapPkgReceived(bootstrap, pkg, 0);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleNMReady) {
    std::vector<std::string> commsChannels = {"MockComms-0/channel1", "MockComms-1/channel2"};
    DeviceInfo devInfo = {"linux", "x86_64", "client"};
    auto bootstrap = std::make_shared<BootstrapInfo>(devInfo, "passphrase", "bootstrapChannel");
    bootstrap->state = BootstrapInfo::WAITING_FOR_NM;
    bsInstanceManager->handleNMReady(bootstrap, commsChannels);
    LOG_EXPECT(logger, __func__, *bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleNMReady_invalid_state) {
    std::vector<std::string> commsChannels = {"MockComms-0/channel1", "MockComms-1/channel2"};
    DeviceInfo devInfo = {"linux", "x86_64", "client"};
    auto bootstrap = std::make_shared<BootstrapInfo>(devInfo, "passphrase", "bootstrapChannel");
    bootstrap->state = BootstrapInfo::FAILED;
    bsInstanceManager->handleNMReady(bootstrap, commsChannels);
    LOG_EXPECT(logger, __func__, *bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleLinkFailed) {
    LinkID linkId = "link-id";
    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    bootstrap.state = BootstrapInfo::WAITING_FOR_LINK;
    bsInstanceManager->handleLinkFailed(bootstrap, linkId);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleNMFailed) {
    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    bootstrap.state = BootstrapInfo::WAITING_FOR_NM;
    bsInstanceManager->handleNMFailed(bootstrap);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleServeFilesFailed) {
    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    bootstrap.state = BootstrapInfo::WAITING_FOR_BOOTSTRAP_PKG;
    bsInstanceManager->handleNMFailed(bootstrap);
    LOG_EXPECT(logger, __func__, bootstrap);
}

TEST_F(BootstrapInstanceManagerTestFixture, handleCancelled) {
    auto bootstrap = BootstrapInfo{{"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"};
    for (int state = BootstrapInfo::INITIALIZED; state <= BootstrapInfo::CANCELLED; ++state) {
        bootstrap.state = bootstrap.state = static_cast<BootstrapInfo::State>(state);

        EXPECT_CALL(manager, removePendingBootstrap(_)).Times(1);
        bsInstanceManager->handleCancelled(bootstrap);
        EXPECT_EQ(bootstrap.state, BootstrapInfo::CANCELLED);
    }
}
