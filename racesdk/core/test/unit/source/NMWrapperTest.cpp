
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

#include "../../../include/RaceSdk.h"
#include "../../../source/CommsWrapper.h"
#include "../../../source/NMWrapper.h"
#include "../../common/MockRacePluginNM.h"
#include "../../common/MockRaceSdk.h"
#include "../../common/helpers.h"
#include "../../common/race_printers.h"
#include "gtest/gtest.h"

using ::testing::Return;

TEST(NMWrapperTest, test_constructor) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);
}

TEST(NMWrapperTest, startHandler) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);
    wrapper.startHandler();

    // destructor should stop handler thread
}

TEST(NMWrapperTest, start_stop_handler) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);
    wrapper.startHandler();
    wrapper.stopHandler();
}

TEST(NMWrapperTest, init) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    PluginConfig pluginConfig;
    pluginConfig.etcDirectory = "bloop";
    pluginConfig.loggingDirectory = "foo";
    pluginConfig.auxDataDirectory = "bar";

    EXPECT_CALL(*mockNM, init(pluginConfig)).Times(1).WillOnce(Return(PLUGIN_OK));
    EXPECT_EQ(wrapper.init(pluginConfig), true);
}

TEST(NMWrapperTest, init_error) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    PluginConfig pluginConfig;
    pluginConfig.etcDirectory = "bloop";
    pluginConfig.loggingDirectory = "foo";
    pluginConfig.auxDataDirectory = "bar";

    EXPECT_CALL(*mockNM, init(pluginConfig)).Times(1).WillOnce(Return(PLUGIN_ERROR));
    EXPECT_EQ(wrapper.init(pluginConfig), false);
}

TEST(NMWrapperTest, init_fatal) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    PluginConfig pluginConfig;
    pluginConfig.etcDirectory = "bloop";
    pluginConfig.loggingDirectory = "foo";
    pluginConfig.auxDataDirectory = "bar";

    EXPECT_CALL(*mockNM, init(pluginConfig)).Times(1).WillOnce(Return(PLUGIN_FATAL));
    EXPECT_EQ(wrapper.init(pluginConfig), false);
}

TEST(NMWrapperTest, shutdown) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);
    EXPECT_CALL(*mockNM, shutdown()).Times(1);

    wrapper.startHandler();
    wrapper.shutdown();
    wrapper.stopHandler();
}

TEST(NMWrapperTest, processClrMsg) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);
    std::string messageText = "my message";
    const ClrMsg sentMessage(messageText, "from sender", "to recipient", 1, 0);
    RaceHandle handle = 42;

    EXPECT_CALL(*mockNM, processClrMsg(handle, sentMessage)).Times(1);

    wrapper.startHandler();
    wrapper.processClrMsg(handle, sentMessage, 0);
    wrapper.stopHandler();
}

TEST(NMWrapperTest, processEncPkg) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    const std::string cipherText = "my cipher text";
    EncPkg encPkg(0, 0, {cipherText.begin(), cipherText.end()});
    const std::vector<ConnectionID> connIds = {"connectionId"};

    RaceHandle handle = 42;

    EXPECT_CALL(*mockNM, processEncPkg(handle, encPkg, connIds)).Times(1);

    wrapper.startHandler();
    wrapper.processEncPkg(handle, encPkg, connIds, 0);
    wrapper.stopHandler();
}

TEST(NMWrapperTest, processEncPkg_queue_full) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    const std::string cipherText(sdk.getRaceConfig().wrapperQueueMaxSize,
                                 'a');  // shouldn't fit queue
    EncPkg encPkg(0, 0, {cipherText.begin(), cipherText.end()});
    const std::vector<ConnectionID> connIds = {"connectionId"};

    RaceHandle handle = 42;

    EXPECT_CALL(*mockNM, processEncPkg(handle, encPkg, connIds)).Times(0);

    wrapper.startHandler();
    auto [success, utilization] = wrapper.processEncPkg(handle, encPkg, connIds, 0);
    wrapper.stopHandler();

    (void)utilization;
    EXPECT_EQ(success, false);
}

/*
 * Make sure timeout will cause posting to block until space is available
 */
TEST(NMWrapperTest, processEncPkg_queue_full_timeout) {
    using namespace std::chrono_literals;
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    const std::string cipherText(sdk.getRaceConfig().wrapperQueueMaxSize / 2 + 1,
                                 'a');  // so two packages won't fit
    EncPkg encPkg(0, 0, {cipherText.begin(), cipherText.end()});
    const std::vector<ConnectionID> connIds = {"connectionId"};

    RaceHandle handle = 42;
    RaceHandle handle2 = 1337;

    EXPECT_CALL(*mockNM, processEncPkg(handle, encPkg, connIds))
        .Times(1)
        .WillOnce([&](RaceHandle, const EncPkg &, const std::vector<std::string> &) {
            std::this_thread::sleep_for(10ms);
            return PLUGIN_OK;
        });
    EXPECT_CALL(*mockNM, processEncPkg(handle2, encPkg, connIds)).Times(1);

    wrapper.startHandler();
    auto [success1, utilization1] = wrapper.processEncPkg(handle, encPkg, connIds, 0);
    auto [success2, utilization2] = wrapper.processEncPkg(handle2, encPkg, connIds, 10000);

    wrapper.stopHandler();

    (void)utilization1;
    (void)utilization2;
    EXPECT_EQ(success1, true);
    EXPECT_EQ(success2, true);
}

TEST(NMWrapperTest, processEncPkg_queue_utilization) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    const std::string cipherText(sdk.getRaceConfig().wrapperQueueMaxSize / 100,
                                 'a');  // should result in 0.01 utilization
    EncPkg encPkg(0, 0, {cipherText.begin(), cipherText.end()});
    const std::vector<ConnectionID> connIds = {"connectionId"};

    RaceHandle handle = 42;

    EXPECT_CALL(*mockNM, processEncPkg(handle, encPkg, connIds)).Times(1);

    wrapper.startHandler();
    auto [success, utilization] = wrapper.processEncPkg(handle, encPkg, connIds, 0);
    wrapper.stopHandler();

    (void)success;
    EXPECT_NEAR(utilization, 0.01, 0.0001);
}

TEST(NMWrapperTest, onPackageStatusChanged) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    RaceHandle handle = 42;
    EXPECT_CALL(*mockNM, onPackageStatusChanged(handle, PACKAGE_SENT)).Times(1);

    wrapper.startHandler();
    wrapper.onPackageStatusChanged(handle, PACKAGE_SENT, 0);
    wrapper.stopHandler();
}

TEST(NMWrapperTest, onConnectionStatusChanged) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    RaceHandle handle = 42;
    const LinkID linkId = "LinkID";
    const ConnectionID connId = "my connection";
    LinkProperties linkProperties = getDefaultLinkProperties();
    EXPECT_CALL(*mockNM,
                onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, linkId, linkProperties))
        .Times(1);

    wrapper.startHandler();
    wrapper.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, linkId, linkProperties, 0);
    wrapper.stopHandler();
}

TEST(NMWrapperTest, onLinkStatusChanged) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    RaceHandle handle = 42;
    const LinkID linkId = "LinkID";
    LinkProperties linkProperties = getDefaultLinkProperties();
    EXPECT_CALL(*mockNM, onLinkStatusChanged(handle, linkId, LINK_CREATED, linkProperties))
        .Times(1);

    wrapper.startHandler();
    wrapper.onLinkStatusChanged(handle, linkId, LINK_CREATED, getDefaultLinkProperties(), 0);
    wrapper.stopHandler();
}

TEST(NMWrapperTest, onChannelStatusChanged) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    RaceHandle handle = 42;
    const std::string channelGid = "channel1";
    ChannelProperties channelProperties = {};
    EXPECT_CALL(*mockNM,
                onChannelStatusChanged(handle, channelGid, CHANNEL_AVAILABLE, channelProperties))
        .Times(1);

    wrapper.startHandler();
    wrapper.onChannelStatusChanged(handle, channelGid, CHANNEL_AVAILABLE, {}, 0);
    wrapper.stopHandler();
}

TEST(NMWrapperTest, onLinkPropertiesChanged) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    const LinkID linkId = "my link";
    LinkProperties linkProperties = getDefaultLinkProperties();
    EXPECT_CALL(*mockNM, onLinkPropertiesChanged(linkId, linkProperties)).Times(1);

    wrapper.startHandler();
    wrapper.onLinkPropertiesChanged(linkId, getDefaultLinkProperties(), 0);
    wrapper.stopHandler();
}

TEST(NMWrapperTest, onPersonaLinksChanged) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    std::string persona = "my persona";
    const std::vector<LinkID> linkIds = {"My Link 1", "My Link 2"};
    EXPECT_CALL(*mockNM, onPersonaLinksChanged(persona, LT_SEND, linkIds)).Times(1);

    wrapper.startHandler();
    wrapper.onPersonaLinksChanged(persona, LT_SEND, linkIds, 0);
    wrapper.stopHandler();
}

TEST(NMWrapper, onUserInputReceived) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    EXPECT_CALL(*mockNM, onUserInputReceived(0x11223344l, true, "expected-response")).Times(1);

    wrapper.startHandler();
    wrapper.onUserInputReceived(0x11223344l, true, "expected-response", 0);
    wrapper.stopHandler();
}

/**
 * @brief The constructor has an optional parameter for the configuration path. If an argument is
 * NOT provided then it should default to use the provided plugin ID.
 *
 */
TEST(NMWrapperTest, config_path_should_default_to_id) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk);

    EXPECT_EQ(wrapper.getConfigPath(), "MockNM");
}

/**
 * @brief The constructor has an optional parameter for the configuration path. If an argument is
 * provided then it should set the config path for the object.
 *
 */
TEST(NMWrapperTest, constructor_should_set_the_config_path) {
    auto mockNM = std::make_shared<MockRacePluginNM>();
    MockRaceSdk sdk;
    NMWrapper wrapper(mockNM, "MockNM", "Mock Network Manager Testing", sdk, "my/config/path/");

    EXPECT_EQ(wrapper.getConfigPath(), "my/config/path/");
}
