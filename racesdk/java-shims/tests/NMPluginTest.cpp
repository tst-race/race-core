
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

// System
#include <gmock/gmock.h>
#include <jni.h>

// SDK Core
#include <IRaceSdkNM.h>

// Java Shims
#include <JavaShimUtils.h>
#include <PluginNMJavaWrapper.h>

#include "../source/JavaIds.h"

// Test helpers
#include "race/mocks/MockRaceSdkNM.h"

class NMPluginTest : public ::testing::Test {
public:
    MockRaceSdkNM mockSdk;
    IRacePluginNM *plugin;
    JNIEnv *env;

    virtual void SetUp() override {
        JavaVM *jvm = JavaShimUtils::getJvm();
        JavaShimUtils::getEnv(&env, jvm);
        ASSERT_NE(nullptr, env);
        JavaIds::load(env);
        plugin = new PluginNMJavaWrapper(&mockSdk, "", "com/twosix/race/StubNMPlugin");
    }

    virtual void TearDown() override {
        delete plugin;
        JavaIds::unload(env);
        // JavaShimUtils::destroyJvm();
    }
};

TEST_F(NMPluginTest, sdkFunctions) {
    PluginConfig pluginConfig;
    pluginConfig.etcDirectory = "/expected/global/path";
    pluginConfig.loggingDirectory = "/expected/logging/path";
    pluginConfig.auxDataDirectory = "/expected/aux-data/path";

    RawData entropy = {0x01, 0x02};
    EXPECT_CALL(mockSdk, getEntropy(2)).WillOnce(::testing::Return(entropy));
    EXPECT_CALL(mockSdk, getActivePersona()).WillOnce(::testing::Return("expected-persona"));

    SdkResponse response;
    response.status = SDK_OK;
    response.handle = 0x1122334455667788;
    response.queueUtilization = 0.15;

    EXPECT_CALL(mockSdk, requestPluginUserInput("expected-user-input-key",
                                                "expected-user-input-prompt", true))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, requestCommonUserInput("expected-user-input-key"))
        .WillOnce(::testing::Return(response));

    EncPkg pkg(0x1122113311441155, 0x43214321, {0x42});
    const uint64_t batchId = 0;
    EXPECT_CALL(mockSdk, sendEncryptedPackage(pkg, "expected-conn-id", batchId, 1))
        .WillOnce(::testing::Return(response));

    ClrMsg msg("expected-plaintext", "expected-from-persona", "expected-to-persona", 0, 0, 0, 0);
    EXPECT_CALL(mockSdk, presentCleartextMessage(msg)).WillOnce(::testing::Return(response));

    EXPECT_CALL(mockSdk,
                openConnection(LT_SEND, "expected-link-id", "expected-link-hints", 7, 2, 3))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, closeConnection("expected-conn-id", 3))
        .WillOnce(::testing::Return(response));

    std::vector<std::string> personas = {"expected-persona-1", "expected-persona-2"};
    std::vector<std::string> links = {"expected-link-id-1", "expected-link-id-2"};
    EXPECT_CALL(mockSdk, getLinksForPersonas(personas, LT_RECV)).WillOnce(::testing::Return(links));

    std::vector<LinkID> chanLinks;
    EXPECT_CALL(mockSdk, getLinksForChannel("expected-channel-gid"))
        .WillOnce(::testing::Return(chanLinks));

    EXPECT_CALL(mockSdk, getPersonasForLink("expected-link-id"))
        .WillOnce(::testing::Return(personas));
    EXPECT_CALL(mockSdk, setPersonasForLink("expected-link-id", personas))
        .WillOnce(::testing::Return(response));

    LinkProperties props;
    props.linkType = LT_SEND;
    props.transmissionType = TT_UNICAST;
    EXPECT_CALL(mockSdk, getLinkProperties("expected-link-id")).WillOnce(::testing::Return(props));

    ChannelProperties channelProps;
    channelProps.linkDirection = LD_CREATOR_TO_LOADER;
    EXPECT_CALL(mockSdk, getChannelProperties("expected-channel-gid"))
        .WillOnce(::testing::Return(channelProps));

    std::map<std::string, ChannelProperties> supportedChannels;
    supportedChannels.insert({"expected-channel-gid", channelProps});
    EXPECT_CALL(mockSdk, getSupportedChannels()).WillOnce(::testing::Return(supportedChannels));

    EXPECT_CALL(mockSdk, deactivateChannel("expected-channel-gid", 3))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, destroyLink("expected-link-id", 3)).WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, createLink("expected-channel-gid",
                                    std::vector<std::string>({"expected-persona"}), 3))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, loadLinkAddress("expected-channel-gid", "expected-link-address",
                                         std::vector<std::string>({"expected-persona"}), 3))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, loadLinkAddresses("expected-channel-gid",
                                           std::vector<std::string>({"expected-link-address"}),
                                           std::vector<std::string>({"expected-persona"}), 3))
        .WillOnce(::testing::Return(response));

    ASSERT_EQ(PLUGIN_OK, plugin->init(pluginConfig));
}

TEST_F(NMPluginTest, shutdown) {
    ASSERT_EQ(PLUGIN_OK, plugin->shutdown());
}

TEST_F(NMPluginTest, processClrMsg) {
    ClrMsg msg("expected-message", "expected-from", "expected-to", 0, 0, 0, 0);
    ASSERT_EQ(PLUGIN_OK, plugin->processClrMsg(0x8877665544332211, msg));
}

TEST_F(NMPluginTest, processEncPkg) {
    EncPkg pkg(0x1122113311441155, 0x12344321, {0x08, 0x67, 0x53, 0x09});
    ASSERT_EQ(PLUGIN_OK,
              plugin->processEncPkg(0x12345678, pkg, {"expected-conn-id-1", "expected-conn-id-2"}));
}

TEST_F(NMPluginTest, onPackageStatusChanged) {
    ASSERT_EQ(PLUGIN_OK, plugin->onPackageStatusChanged(0x11223344, PACKAGE_RECEIVED));
}

TEST_F(NMPluginTest, onConnectionStatusChanged) {
    LinkProperties props;
    props.linkType = LT_SEND;
    props.transmissionType = TT_UNICAST;
    ASSERT_EQ(PLUGIN_OK,
              plugin->onConnectionStatusChanged(0x7777, "expected-conn-id", CONNECTION_OPEN,
                                                "expected-link-id", props));
}

TEST_F(NMPluginTest, onChannelStatusChanged) {
    ChannelProperties props;
    props.linkDirection = LD_CREATOR_TO_LOADER;
    ASSERT_EQ(PLUGIN_OK, plugin->onChannelStatusChanged(0x7777, "expected-channel-gid",
                                                        CHANNEL_AVAILABLE, props));
}

TEST_F(NMPluginTest, onLinkStatusChanged) {
    LinkProperties props;
    props.linkType = LT_SEND;
    props.transmissionType = TT_MULTICAST;
    ASSERT_EQ(PLUGIN_OK,
              plugin->onLinkStatusChanged(0x7777, "expected-link-id", LINK_CREATED, props));
}

TEST_F(NMPluginTest, onLinkPropertiesChanged) {
    LinkProperties props;
    props.linkType = LT_RECV;
    props.transmissionType = TT_MULTICAST;
    ASSERT_EQ(PLUGIN_OK, plugin->onLinkPropertiesChanged("expected-link-id", props));
}

TEST_F(NMPluginTest, onPersonaLinksChanged) {
    ASSERT_EQ(PLUGIN_OK,
              plugin->onPersonaLinksChanged(
                  "expected-recipient", LT_BIDI,
                  std::vector<std::string>({"expected-link-id-1", "expected-link-id-2"})));
}

TEST_F(NMPluginTest, prepareToBootstrap) {
    ASSERT_EQ(PLUGIN_OK, plugin->prepareToBootstrap(0x1234, "link id", "config path",
                                                    {"platform", "architecture", "node type"}));
}

TEST_F(NMPluginTest, onBootstrapKeyReceived) {
    ASSERT_EQ(PLUGIN_OK, plugin->onBootstrapPkgReceived("persona", {8, 7, 6, 5, 4, 3, 2, 1}));
}

TEST_F(NMPluginTest, onUserInputReceived) {
    ASSERT_EQ(PLUGIN_OK, plugin->onUserInputReceived(0x11223344, true, "expected-user-input"));
}

TEST_F(NMPluginTest, notifyEpoch) {
    ASSERT_EQ(PLUGIN_OK, plugin->notifyEpoch("expected-epoch-data"));
}
