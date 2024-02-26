
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

#include <memory.h>

#include "../../../include/BootstrapManager.h"
#include "../../../source/BootstrapThread.h"
#include "../../common/LogExpect.h"
#include "../../common/MockBootstrapManager.h"
#include "../../common/MockRaceSdk.h"
#include "../../common/helpers.h"
#include "../../common/race_printers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;
using ::testing::Return;

class TestBootstrapManager : public BootstrapManager {
public:
    explicit TestBootstrapManager(RaceSdk &sdk,
                                  std::shared_ptr<FileSystemHelper> _fileSystemHelper) :
        BootstrapManager(sdk, _fileSystemHelper) {}

    using BootstrapManager::bootstraps;
    using BootstrapManager::bsInstanceManager;
};

class BootstrapManagerTestFixture : public ::testing::Test {
public:
    BootstrapManagerTestFixture() :
        test_info(testing::UnitTest::GetInstance()->current_test_info()),
        logger(test_info->test_suite_name(), test_info->name()),
        appConfig(createDefaultAppConfig()),
        raceConfig(createDefaultRaceConfig()),
        fileSystemHelper(std::make_shared<MockFileSystemHelper>()),
        manager(sdk, fileSystemHelper) {
        manager.bsInstanceManager =
            std::make_shared<BootstrapInstanceManager>(manager, fileSystemHelper);

        auto bsInstanceManager =
            std::make_unique<MockBootstrapInstanceManager>(logger, manager, fileSystemHelper);
        mockBsInstanceManager = bsInstanceManager.get();
        manager.bsInstanceManager = std::move(bsInstanceManager);
        mockBsInstanceManager->bootstrapThread =
            std::make_unique<MockBootstrapThread>(logger, manager, fileSystemHelper);
    }
    virtual ~BootstrapManagerTestFixture() {}
    virtual void SetUp() override {}
    virtual void TearDown() override {
        logger.check();
    }

    void waitForCallbacks() {}

public:
    const std::string bootstrapChannelId = "bsChannelId";
    const testing::TestInfo *test_info;
    LogExpect logger;
    AppConfig appConfig;
    RaceConfig raceConfig;
    MockRaceSdk sdk;
    std::shared_ptr<MockFileSystemHelper> fileSystemHelper;
    TestBootstrapManager manager;
    MockBootstrapInstanceManager *mockBsInstanceManager;
};

TEST_F(BootstrapManagerTestFixture, test_constructor) {
    ASSERT_EQ(manager.bootstraps.size(), 0);
}

TEST_F(BootstrapManagerTestFixture, prepareToBootstrap_invalid_device) {
    ASSERT_EQ(manager.prepareToBootstrap({"invalid", "invalid", "invalid"}, "passphrase",
                                         "bootstrapChannel"),
              false);
}

TEST_F(BootstrapManagerTestFixture, prepareToBootstrap_createBootstrapLink_failed) {
    ASSERT_EQ(manager.bootstraps.size(), 0);
    EXPECT_CALL(*mockBsInstanceManager, handleBootstrapStart(_)).WillOnce(Return(false));
    ASSERT_EQ(
        manager.prepareToBootstrap({"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"),
        false);
    ASSERT_EQ(manager.bootstraps.size(), 1);
}

TEST_F(BootstrapManagerTestFixture, prepareToBootstrap_createBootstrapLink_cancelled) {
    RaceHandle handle = 12345;
    EXPECT_CALL(*mockBsInstanceManager, handleBootstrapStart(_))
        .WillOnce(testing::Invoke([&handle](BootstrapInfo &bootstrap) {
            bootstrap.prepareBootstrapHandle = handle;
            return bootstrap.prepareBootstrapHandle;
        }));

    manager.prepareToBootstrap({"linux", "x86_64", "client"}, "passphrase", bootstrapChannelId);
    ASSERT_EQ(manager.bootstraps.size(), 1);
    EXPECT_EQ(handle, manager.bootstraps[0]->prepareBootstrapHandle);
    EXPECT_CALL(*mockBsInstanceManager, handleCancelled(_)).Times(1);
    manager.cancelBootstrap(handle);
}

TEST_F(BootstrapManagerTestFixture, prepareToBootstrap) {
    ASSERT_EQ(manager.bootstraps.size(), 0);
    EXPECT_CALL(*mockBsInstanceManager, handleBootstrapStart(_)).WillOnce(Return(true));
    ASSERT_EQ(
        manager.prepareToBootstrap({"linux", "x86_64", "client"}, "passphrase", "bootstrapChannel"),
        true);
    ASSERT_EQ(manager.bootstraps.size(), 1);
}

TEST_F(BootstrapManagerTestFixture, onLinkStatusChanged) {
    RaceHandle handle = 1;
    LinkID linkId = "link id";
    // auto bootstrap = std::make_shared<MockBootstrap>(manager);
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->createdLinkHandle = handle;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.onLinkStatusChanged(handle, linkId, LINK_CREATED, {}), true);
}

TEST_F(BootstrapManagerTestFixture, onLinkStatusChanged_cancelled) {
    RaceHandle handle = 1;
    LinkID linkId = "link id";
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->createdLinkHandle = handle;
    bootstrap->bootstrapChannelId = bootstrapChannelId;
    manager.bootstraps.push_back(bootstrap);
    manager.onLinkStatusChanged(handle, linkId, LINK_CREATED, {});

    EXPECT_CALL(*mockBsInstanceManager, handleCancelled(_)).Times(1);
    manager.cancelBootstrap(bootstrap->prepareBootstrapHandle);
}

TEST_F(BootstrapManagerTestFixture, onLinkStatusChanged_destroyed) {
    RaceHandle handle = 1;
    LinkID linkId = "link id";
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->createdLinkHandle = handle;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.onLinkStatusChanged(handle, linkId, LINK_DESTROYED, {}), true);
}

TEST_F(BootstrapManagerTestFixture, onLinkStatusChanged_invalid) {
    RaceHandle handle = 1;
    LinkID linkId = "link id";
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->createdLinkHandle = handle;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.onLinkStatusChanged(handle, linkId, LINK_LOADED, {}), true);
}

TEST_F(BootstrapManagerTestFixture, onLinkStatusChanged_no_matching_bootstrap) {
    RaceHandle handle = 1;
    RaceHandle handle2 = 2;
    LinkID linkId = "link id";
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->createdLinkHandle = handle;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.onLinkStatusChanged(handle2, linkId, LINK_CREATED, {}), false);
}

TEST_F(BootstrapManagerTestFixture, onConnectionStatusChanged) {
    RaceHandle handle = 1;
    ConnectionID connId = "conn id";
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->connectionHandle = handle;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, {}), true);
}

TEST_F(BootstrapManagerTestFixture, onConnectionStatusChanged_cancelled) {
    RaceHandle handle = 1;
    ConnectionID connId = "conn id";
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->connectionHandle = handle;
    bootstrap->bootstrapChannelId = bootstrapChannelId;
    manager.bootstraps.push_back(bootstrap);
    manager.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, {});

    EXPECT_CALL(*mockBsInstanceManager, handleCancelled(_)).Times(1);
    manager.cancelBootstrap(bootstrap->prepareBootstrapHandle);
}

TEST_F(BootstrapManagerTestFixture, onConnectionStatusChanged_closed) {
    RaceHandle handle = 1;
    ConnectionID connId = "conn id";
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->connectionHandle = handle;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.onConnectionStatusChanged(handle, connId, CONNECTION_CLOSED, {}), true);
}

TEST_F(BootstrapManagerTestFixture, onConnectionStatusChanged_closed_cancelled) {
    RaceHandle handle = 1;
    ConnectionID connId = "conn id";
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->connectionHandle = handle;
    bootstrap->bootstrapChannelId = bootstrapChannelId;
    manager.bootstraps.push_back(bootstrap);
    manager.onConnectionStatusChanged(handle, connId, CONNECTION_CLOSED, {});

    EXPECT_CALL(*mockBsInstanceManager, handleCancelled(_)).Times(1);
    manager.cancelBootstrap(bootstrap->prepareBootstrapHandle);
}

TEST_F(BootstrapManagerTestFixture, onConnectionStatusChanged_no_matching_bootstrap) {
    RaceHandle handle = 1;
    RaceHandle handle2 = 2;
    ConnectionID connId = "conn id";
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->connectionHandle = handle;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.onConnectionStatusChanged(handle2, connId, CONNECTION_OPEN, {}), false);
}

TEST_F(BootstrapManagerTestFixture, onReceiveEncPkg) {
    LinkID linkId = "link id";
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->bootstrapLink = linkId;
    bootstrap->bootstrapConnection = "conn id";
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.onReceiveEncPkg(EncPkg{{}}, linkId, 0), true);
}

TEST_F(BootstrapManagerTestFixture, onReceiveEncPkg_cancelled) {
    LinkID linkId = "link id";
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->bootstrapLink = linkId;
    bootstrap->bootstrapChannelId = bootstrapChannelId;
    bootstrap->bootstrapConnection = "conn id";
    manager.bootstraps.push_back(bootstrap);
    manager.onReceiveEncPkg(EncPkg{{}}, linkId, 0);

    EXPECT_CALL(*mockBsInstanceManager, handleCancelled(_)).Times(1);
    manager.cancelBootstrap(bootstrap->prepareBootstrapHandle);
}

TEST_F(BootstrapManagerTestFixture, onReceiveEncPkg_no_matching_bootstrap) {
    LinkID linkId = "link id";
    LinkID linkId2 = "link id2";
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->bootstrapLink = linkId;
    bootstrap->bootstrapConnection = "conn id";
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.onReceiveEncPkg(EncPkg{{}}, linkId2, 0), false);
}

TEST_F(BootstrapManagerTestFixture, bootstrapDevice) {
    RaceHandle handle = 1;
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->prepareBootstrapHandle = handle;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.bootstrapDevice(handle, {"channel1", "channel3"}), true);
}

TEST_F(BootstrapManagerTestFixture, bootstrapDevice_no_matching_bootstrap) {
    RaceHandle handle = 1;
    RaceHandle handle2 = 2;
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->prepareBootstrapHandle = handle;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.bootstrapDevice(handle2, {"channel1", "channel3"}), false);
}

TEST_F(BootstrapManagerTestFixture, bootstrapFailed) {
    RaceHandle handle = 1;
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->prepareBootstrapHandle = handle;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.bootstrapFailed(handle), true);
}

TEST_F(BootstrapManagerTestFixture, bootstrapFailed_no_matching_bootstrap) {
    RaceHandle handle = 1;
    RaceHandle handle2 = 2;
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->prepareBootstrapHandle = handle;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.bootstrapFailed(handle2), false);
}

TEST_F(BootstrapManagerTestFixture, onServeFilesFailed) {
    RaceHandle handle = 1;
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->prepareBootstrapHandle = handle;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_EQ(manager.onServeFilesFailed(*bootstrap), true);
}

TEST_F(BootstrapManagerTestFixture, onServeFilesFailed_no_matching_bootstrap) {
    RaceHandle handle = 1;
    RaceHandle handle2 = 2;
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->prepareBootstrapHandle = handle;
    manager.bootstraps.push_back(bootstrap);
    auto bootstrap2 = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap2->prepareBootstrapHandle = handle2;
    EXPECT_EQ(manager.onServeFilesFailed(*bootstrap2), false);
}

TEST_F(BootstrapManagerTestFixture, cancelBootstrap) {
    RaceHandle handle = 42;
    EXPECT_CALL(*mockBsInstanceManager, handleBootstrapStart(_))
        .WillOnce(testing::Invoke([&handle](BootstrapInfo &bootstrap) {
            bootstrap.prepareBootstrapHandle = handle;
            return bootstrap.prepareBootstrapHandle;
        }));

    manager.prepareToBootstrap({"linux", "x86_64", "client"}, "passphrase", bootstrapChannelId);
    ASSERT_EQ(manager.bootstraps.size(), 1);
    EXPECT_EQ(manager.bootstraps[0]->prepareBootstrapHandle, handle);
    EXPECT_CALL(*mockBsInstanceManager, handleCancelled(_)).Times(1);
    manager.cancelBootstrap(manager.bootstraps[0]->prepareBootstrapHandle);
}

TEST_F(BootstrapManagerTestFixture, onBootstrapFinished) {
    RaceHandle handle = 1;
    auto bootstrap = std::make_shared<BootstrapInfo>(DeviceInfo{}, std::string{}, std::string{});
    bootstrap->prepareBootstrapHandle = handle;
    bootstrap->bootstrapChannelId = bootstrapChannelId;
    bootstrap->state = BootstrapInfo::SUCCESS;
    manager.bootstraps.push_back(bootstrap);

    EXPECT_CALL(sdk, onBootstrapFinished(_, BOOTSTRAP_SUCCESS))
        .Times(1)
        .WillOnce(::testing::Return(true));
    manager.removePendingBootstrap(*bootstrap);

    bootstrap->state = BootstrapInfo::FAILED;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_CALL(sdk, onBootstrapFinished(_, BOOTSTRAP_FAILED))
        .Times(1)
        .WillOnce(::testing::Return(true));
    manager.removePendingBootstrap(*bootstrap);

    bootstrap->state = BootstrapInfo::CANCELLED;
    manager.bootstraps.push_back(bootstrap);
    EXPECT_CALL(sdk, onBootstrapFinished(_, BOOTSTRAP_CANCELLED))
        .Times(1)
        .WillOnce(::testing::Return(true));
    manager.removePendingBootstrap(*bootstrap);
}
