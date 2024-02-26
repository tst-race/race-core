
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
#include <IRaceSdkComms.h>

// Java Shims
#include <JavaShimUtils.h>
#include <PluginCommsJavaWrapper.h>

#include "../source/JavaIds.h"

// Test helpers
#include "race/mocks/MockRaceSdkComms.h"

class CommsPluginTest : public ::testing::Test {
public:
    MockRaceSdkComms mockSdk;
    IRacePluginComms *plugin;
    JNIEnv *env;

    virtual void SetUp() override {
        JavaVM *jvm = JavaShimUtils::getJvm();
        JavaShimUtils::getEnv(&env, jvm);
        ASSERT_NE(nullptr, env);
        JavaIds::load(env);
        plugin = new PluginCommsJavaWrapper(&mockSdk, "", "com/twosix/race/StubCommsPlugin");
    }

    virtual void TearDown() override {
        delete plugin;
        JavaIds::unload(env);
        // JavaShimUtils::destroyJvm();
    }
};

TEST_F(CommsPluginTest, sdkFunctions) {
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

    EXPECT_CALL(mockSdk, onPackageStatusChanged(0x8877665544332211, PACKAGE_SENT, 1))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, onConnectionStatusChanged(0x12345678, "expected-conn-id",
                                                   CONNECTION_CLOSED, ::testing::_, 2))
        .WillOnce(::testing::Return(response));

    EXPECT_CALL(mockSdk, onLinkStatusChanged(0x12345678, "expected-link-id", LINK_DESTROYED,
                                             ::testing::_, 2))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, onChannelStatusChanged(0x12345678, "expected-channel-gid",
                                                CHANNEL_AVAILABLE, ::testing::_, 2))
        .WillOnce(::testing::Return(response));

    EXPECT_CALL(mockSdk, updateLinkProperties("expected-link-id", ::testing::_, 3))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, generateConnectionId("expected-link-id"))
        .WillOnce(::testing::Return("expected-conn-id"));
    EXPECT_CALL(mockSdk, generateLinkId("expected-channel-gid"))
        .WillOnce(::testing::Return("expected-channel-gid/expected-link-id"));

    EncPkg pkg(0x0011223344556677, 0x2211331144115511, {0x08, 0x67, 0x53, 0x09});
    std::vector<std::string> connIds = {"expected-conn-id-1", "expected-conn-id-2"};
    EXPECT_CALL(mockSdk, receiveEncPkg(pkg, connIds, 4)).WillOnce(::testing::Return(response));

    EXPECT_CALL(mockSdk, unblockQueue("expected-conn-id")).WillOnce(::testing::Return(response));

    PluginConfig pluginConfig;
    pluginConfig.etcDirectory = "/expected/global/path";
    pluginConfig.loggingDirectory = "/expected/logging/path";
    pluginConfig.auxDataDirectory = "/expected/aux-data/path";
    ASSERT_EQ(PLUGIN_OK, plugin->init(pluginConfig));
}

TEST_F(CommsPluginTest, twoJavaCommsPlugins) {
    RawData entropy = {0x01, 0x02};
    MockRaceSdkComms mockSdk2;
    std::string plugin2Name = "plugin2";
    std::string plugin2ClassName = "com/twosix/race/StubCommsPlugin";
    PluginCommsJavaWrapper plugin2(&mockSdk2, plugin2Name, plugin2ClassName);

    // ensure the methods are invoked on mock sdk 1 and not on mock sdk 2
    PluginConfig pluginConfig;
    pluginConfig.etcDirectory = "/expected/global/path";
    pluginConfig.loggingDirectory = "/expected/logging/path";
    pluginConfig.auxDataDirectory = "/expected/aux-data/path";
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

    EXPECT_CALL(mockSdk, onPackageStatusChanged(0x8877665544332211, PACKAGE_SENT, 1))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, onConnectionStatusChanged(0x12345678, "expected-conn-id",
                                                   CONNECTION_CLOSED, ::testing::_, 2))
        .WillOnce(::testing::Return(response));

    EXPECT_CALL(mockSdk, onLinkStatusChanged(0x12345678, "expected-link-id", LINK_DESTROYED,
                                             ::testing::_, 2))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, onChannelStatusChanged(0x12345678, "expected-channel-gid",
                                                CHANNEL_AVAILABLE, ::testing::_, 2))
        .WillOnce(::testing::Return(response));

    EXPECT_CALL(mockSdk, updateLinkProperties("expected-link-id", ::testing::_, 3))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, generateConnectionId("expected-link-id"))
        .WillOnce(::testing::Return("expected-conn-id"));
    EXPECT_CALL(mockSdk, generateLinkId("expected-channel-gid"))
        .WillOnce(::testing::Return("expected-channel-gid/expected-link-id"));

    EncPkg pkg(0x0011223344556677, 0x2211331144115511, {0x08, 0x67, 0x53, 0x09});
    std::vector<std::string> connIds = {"expected-conn-id-1", "expected-conn-id-2"};
    EXPECT_CALL(mockSdk, receiveEncPkg(pkg, connIds, 4)).WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, unblockQueue("expected-conn-id")).WillOnce(::testing::Return(response));

    ASSERT_EQ(PLUGIN_OK, plugin->init(pluginConfig));

    // ensure the methods are invoked on mock sdk 2 and not on mock sdk 1
    EXPECT_CALL(mockSdk2, getEntropy(2)).WillOnce(::testing::Return(entropy));
    EXPECT_CALL(mockSdk2, getActivePersona()).WillOnce(::testing::Return("expected-persona"));

    EXPECT_CALL(mockSdk2, requestPluginUserInput("expected-user-input-key",
                                                 "expected-user-input-prompt", true))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk2, requestCommonUserInput("expected-user-input-key"))
        .WillOnce(::testing::Return(response));

    EXPECT_CALL(mockSdk2, onPackageStatusChanged(0x8877665544332211, PACKAGE_SENT, 1))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk2, onConnectionStatusChanged(0x12345678, "expected-conn-id",
                                                    CONNECTION_CLOSED, ::testing::_, 2))
        .WillOnce(::testing::Return(response));

    EXPECT_CALL(mockSdk2, onLinkStatusChanged(0x12345678, "expected-link-id", LINK_DESTROYED,
                                              ::testing::_, 2))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk2, onChannelStatusChanged(0x12345678, "expected-channel-gid",
                                                 CHANNEL_AVAILABLE, ::testing::_, 2))
        .WillOnce(::testing::Return(response));

    EXPECT_CALL(mockSdk2, updateLinkProperties("expected-link-id", ::testing::_, 3))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk2, generateConnectionId("expected-link-id"))
        .WillOnce(::testing::Return("expected-conn-id"));
    EXPECT_CALL(mockSdk2, generateLinkId("expected-channel-gid"))
        .WillOnce(::testing::Return("expected-channel-gid/expected-link-id"));

    EXPECT_CALL(mockSdk2, receiveEncPkg(pkg, connIds, 4)).WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk2, unblockQueue("expected-conn-id")).WillOnce(::testing::Return(response));
    ASSERT_EQ(PLUGIN_OK, plugin2.init(pluginConfig));
}

TEST_F(CommsPluginTest, shutdown) {
    ASSERT_EQ(PLUGIN_OK, plugin->shutdown());
}

TEST_F(CommsPluginTest, sendPackage) {
    EncPkg pkg(0x0011223344556677, 0x2211331144115511, {0x08, 0x67, 0x53, 0x09});
    ASSERT_EQ(PLUGIN_OK, plugin->sendPackage(0x8877665544332211, "expected-conn-id", pkg,
                                             std::numeric_limits<double>::infinity(), 6789));
}

TEST_F(CommsPluginTest, openConnection) {
    ASSERT_EQ(PLUGIN_OK, plugin->openConnection(0x03, LT_RECV, "expected-link-id",
                                                "expected-link-hints", 100));
}

TEST_F(CommsPluginTest, closeConnection) {
    ASSERT_EQ(PLUGIN_OK, plugin->closeConnection(0x12345678, "expected-conn-id"));
}

TEST_F(CommsPluginTest, destroyLink) {
    ASSERT_EQ(PLUGIN_OK, plugin->destroyLink(0x12345678, "expected-link-id"));
}

TEST_F(CommsPluginTest, deactivateChannel) {
    ASSERT_EQ(PLUGIN_OK, plugin->deactivateChannel(0x12345678, "expected-channel-gid"));
}

TEST_F(CommsPluginTest, activateChannel) {
    ASSERT_EQ(PLUGIN_OK,
              plugin->activateChannel(0x42, "expected-channel-gid", "expected-role-name"));
}

TEST_F(CommsPluginTest, createLink) {
    ASSERT_EQ(PLUGIN_OK, plugin->createLink(0x3, "expected-channel-gid"));
}

TEST_F(CommsPluginTest, loadLinkAddress) {
    ASSERT_EQ(PLUGIN_OK,
              plugin->loadLinkAddress(0x3, "expected-channel-gid", "expected-link-address"));
}

TEST_F(CommsPluginTest, loadLinkAddresses) {
    std::vector<std::string> addresses = {"expected-link-address1", "expected-link-address2"};
    ASSERT_EQ(PLUGIN_OK, plugin->loadLinkAddresses(0x3, "expected-channel-gid", addresses));
}

TEST_F(CommsPluginTest, onUserInputReceived) {
    ASSERT_EQ(PLUGIN_OK, plugin->onUserInputReceived(0x11223344, true, "expected-user-input"));
}

TEST_F(CommsPluginTest, flushChannel) {
    ASSERT_EQ(PLUGIN_OK, plugin->flushChannel(0x4321, "connection-id-for-flush", 27));
}

TEST_F(CommsPluginTest, serveFiles) {
    ASSERT_EQ(PLUGIN_OK,
              plugin->serveFiles("link-id-for-serveFiles", "/some/path/of/files/to/serve"));
}

TEST_F(CommsPluginTest, createBootstrapLink) {
    ASSERT_EQ(PLUGIN_OK,
              plugin->createBootstrapLink(0x654321, "channel-gid-for-createBootstrapLink",
                                          "passphrase-for-createBootstrapLink"));
}
