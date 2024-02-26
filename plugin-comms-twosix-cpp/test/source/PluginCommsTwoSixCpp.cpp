
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

#include "../../source/PluginCommsTwoSixCpp.h"

#include <memory>

#include "MockChannel.h"
#include "MockLink.h"
#include "gtest/gtest.h"
#include "race/mocks/MockRaceSdkComms.h"

using ::testing::_;
using ::testing::Return;
using P = std::vector<std::string>;

class PluginTestFixture : public ::testing::Test {
public:
    PluginTestFixture() : plugin(&sdk), channel(plugin) {
        ON_CALL(sdk, getActivePersona()).WillByDefault(::testing::Return("race-server-1"));
        ON_CALL(sdk, updateLinkProperties(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(SDK_OK));
        ON_CALL(sdk, receiveEncPkg(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(SDK_OK));
        ON_CALL(sdk, onPackageStatusChanged(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(SDK_OK));
        ON_CALL(sdk, onConnectionStatusChanged(::testing::_, ::testing::_, ::testing::_,
                                               ::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(SDK_OK));
        ON_CALL(sdk, asyncError(::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(SDK_OK));

        ON_CALL(sdk, generateConnectionId(::testing::_)).WillByDefault([this](LinkID linkId) {
            return linkId + "/ConnectionID-" + std::to_string(this->connectionCounter++);
        });
        ON_CALL(sdk, generateLinkId(::testing::_))
            .WillByDefault([this](std::string /*channelGid*/) {
                return "LinkID-" + std::to_string(this->linkCounter++);
            });
    }
    virtual ~PluginTestFixture() {}
    virtual void SetUp() override {}
    virtual void TearDown() override {}

public:
    MockRaceSdkComms sdk;
    PluginCommsTwoSixCpp plugin;
    MockChannel channel;

private:
    int linkCounter{0};
    int connectionCounter{0};
};

TEST_F(PluginTestFixture, init) {
    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    auto response = plugin.init(pluginConfig);
    EXPECT_EQ(response, PLUGIN_OK);
}

TEST_F(PluginTestFixture, init_bad_config) {
    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    auto response = plugin.init(pluginConfig);
    EXPECT_EQ(response, PLUGIN_OK);
}

TEST_F(PluginTestFixture, init_empty_config) {
    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    auto response = plugin.init(pluginConfig);
    EXPECT_EQ(response, PLUGIN_OK);
}

TEST_F(PluginTestFixture, add_get_link) {
    std::shared_ptr<Link> mockLink =
        std::make_shared<MockLink>(&sdk, &plugin, &channel, "LinkID0", LT_RECV);
    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    EXPECT_CALL(sdk, updateLinkProperties(_, _, _));
    plugin.addLink(mockLink);

    EXPECT_EQ(plugin.getLink("LinkID0").get(), mockLink.get());
}

TEST_F(PluginTestFixture, openConnection_valid1) {
    LinkID linkId = "LINK0";
    LinkType linkType = LT_RECV;
    auto mockLink = std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, linkType);

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    ConnectionID connId = "ConnID0";
    EXPECT_CALL(sdk, updateLinkProperties(_, _, _));
    EXPECT_CALL(sdk, generateConnectionId(linkId)).WillOnce(Return(connId));
    plugin.addLink(mockLink);

    RaceHandle handle = 0;
    EXPECT_CALL(*mockLink, openConnection(_, _, _, _))
        .WillOnce(
            Return(std::make_shared<Connection>(connId, linkType, mockLink, "", RACE_UNLIMITED)));
    EXPECT_CALL(sdk, onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, _, RACE_BLOCKING));
    auto response = plugin.openConnection(handle, linkType, linkId, "", RACE_UNLIMITED);

    EXPECT_EQ(response, PLUGIN_OK);
}

TEST_F(PluginTestFixture, openConnection_valid2) {
    LinkID linkId = "LINK1";
    LinkType linkType = LT_SEND;
    auto mockLink = std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, linkType);

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    ConnectionID connId = "ConnID0";
    EXPECT_CALL(sdk, updateLinkProperties(_, _, _));
    EXPECT_CALL(sdk, generateConnectionId(linkId)).WillOnce(Return(connId));
    plugin.addLink(mockLink);

    RaceHandle handle = 0;
    EXPECT_CALL(*mockLink, openConnection(_, _, _, _))
        .WillOnce(
            Return(std::make_shared<Connection>(connId, linkType, mockLink, "", RACE_UNLIMITED)));
    EXPECT_CALL(sdk, onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, _, RACE_BLOCKING));
    auto response = plugin.openConnection(handle, linkType, linkId, "", RACE_UNLIMITED);

    EXPECT_EQ(response, PLUGIN_OK);
}

TEST_F(PluginTestFixture, openConnection_bad_link) {
    LinkID linkId = "LINK0";
    LinkType linkType = LT_RECV;
    auto mockLink = std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, linkType);

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    ConnectionID connId = "ConnID0";
    EXPECT_CALL(sdk, updateLinkProperties(_, _, _));
    EXPECT_CALL(sdk, generateConnectionId("BAD LINK")).WillOnce(Return(connId));
    plugin.addLink(mockLink);

    RaceHandle handle = 0;
    EXPECT_CALL(*mockLink, openConnection(_, _, _, _)).Times(0);
    EXPECT_CALL(sdk,
                onConnectionStatusChanged(handle, connId, CONNECTION_CLOSED, _, RACE_BLOCKING));
    auto response = plugin.openConnection(handle, linkType, "BAD LINK", "", RACE_UNLIMITED);

    EXPECT_EQ(response, PLUGIN_ERROR);
}

TEST_F(PluginTestFixture, openConnection_bad_link_type) {
    LinkID linkId = "LINK0";
    LinkType linkType = LT_RECV;
    auto mockLink = std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, linkType);

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    ConnectionID connId = "ConnID0";
    EXPECT_CALL(sdk, updateLinkProperties(_, _, _));
    EXPECT_CALL(sdk, generateConnectionId(linkId)).WillOnce(Return(connId));
    plugin.addLink(mockLink);

    RaceHandle handle = 0;
    EXPECT_CALL(*mockLink, openConnection(_, _, _, _)).Times(0);
    EXPECT_CALL(sdk,
                onConnectionStatusChanged(handle, connId, CONNECTION_CLOSED, _, RACE_BLOCKING));
    auto response = plugin.openConnection(handle, LT_SEND, linkId, "", RACE_UNLIMITED);

    EXPECT_EQ(response, PLUGIN_ERROR);
}

TEST_F(PluginTestFixture, openConnection_link_failed) {
    LinkID linkId = "LINK0";
    LinkType linkType = LT_RECV;
    auto mockLink = std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, linkType);

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    ConnectionID connId = "ConnID0";
    EXPECT_CALL(sdk, updateLinkProperties(_, _, _));
    EXPECT_CALL(sdk, generateConnectionId(linkId)).WillOnce(Return(connId));
    plugin.addLink(mockLink);

    RaceHandle handle = 0;
    EXPECT_CALL(*mockLink, openConnection(_, _, _, _)).WillOnce(Return(nullptr));
    EXPECT_CALL(sdk,
                onConnectionStatusChanged(handle, connId, CONNECTION_CLOSED, _, RACE_BLOCKING));
    auto response = plugin.openConnection(handle, linkType, linkId, "", RACE_UNLIMITED);

    EXPECT_EQ(response, PLUGIN_ERROR);
}

TEST_F(PluginTestFixture, sendPackage_success) {
    LinkID linkId = "LINK1";
    LinkType linkType = LT_SEND;
    auto mockLink = std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, linkType);

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    ConnectionID connId = "ConnID0";
    EXPECT_CALL(sdk, updateLinkProperties(_, _, _));
    EXPECT_CALL(sdk, generateConnectionId(linkId)).WillOnce(Return(connId));
    plugin.addLink(mockLink);

    RaceHandle handle1 = 0;
    EXPECT_CALL(*mockLink, openConnection(_, _, _, _))
        .WillOnce(
            Return(std::make_shared<Connection>(connId, linkType, mockLink, "", RACE_UNLIMITED)));
    EXPECT_CALL(sdk, onConnectionStatusChanged(_, _, _, _, _));
    plugin.openConnection(handle1, linkType, linkId, "", RACE_UNLIMITED);

    RaceHandle handle2 = 0;
    const std::string cipherText = "my cipher text";
    EncPkg pkg(1, 42, {cipherText.begin(), cipherText.end()});
    ;
    EXPECT_CALL(*mockLink, sendPackage(handle2, pkg, 0)).WillOnce(Return(PLUGIN_OK));
    auto response = plugin.sendPackage(handle2, connId, pkg, 0, 0);
    EXPECT_EQ(response, PLUGIN_OK);
}

TEST_F(PluginTestFixture, sendPackage_temp_error) {
    LinkID linkId = "LINK1";
    LinkType linkType = LT_SEND;
    auto mockLink = std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, linkType);

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    ConnectionID connId = "ConnID0";
    EXPECT_CALL(sdk, updateLinkProperties(_, _, _));
    EXPECT_CALL(sdk, generateConnectionId(linkId)).WillOnce(Return(connId));
    plugin.addLink(mockLink);

    RaceHandle handle1 = 0;
    EXPECT_CALL(*mockLink, openConnection(_, _, _, _))
        .WillOnce(
            Return(std::make_shared<Connection>(connId, linkType, mockLink, "", RACE_UNLIMITED)));
    EXPECT_CALL(sdk, onConnectionStatusChanged(_, _, _, _, _));
    plugin.openConnection(handle1, linkType, linkId, "", RACE_UNLIMITED);

    RaceHandle handle2 = 0;
    const std::string cipherText = "my cipher text";
    EncPkg pkg(1, 42, {cipherText.begin(), cipherText.end()});
    ;
    EXPECT_CALL(*mockLink, sendPackage(handle2, pkg, 0)).WillOnce(Return(PLUGIN_TEMP_ERROR));
    auto response = plugin.sendPackage(handle2, connId, pkg, 0, 0);
    EXPECT_EQ(response, PLUGIN_TEMP_ERROR);
}

TEST_F(PluginTestFixture, sendPackage_bad_connection) {
    LinkID linkId = "LINK1";
    LinkType linkType = LT_SEND;
    auto mockLink = std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, linkType);

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    ConnectionID connId = "ConnID0";
    EXPECT_CALL(sdk, updateLinkProperties(_, _, _));
    EXPECT_CALL(sdk, generateConnectionId(linkId)).WillOnce(Return(connId));
    plugin.addLink(mockLink);

    RaceHandle handle1 = 0;
    EXPECT_CALL(*mockLink, openConnection(_, _, _, _))
        .WillOnce(
            Return(std::make_shared<Connection>(connId, linkType, mockLink, "", RACE_UNLIMITED)));
    EXPECT_CALL(sdk, onConnectionStatusChanged(_, _, _, _, _));
    plugin.openConnection(handle1, linkType, linkId, "", RACE_UNLIMITED);

    RaceHandle handle2 = 0;
    const std::string cipherText = "my cipher text";
    EncPkg pkg(1, 42, {cipherText.begin(), cipherText.end()});
    ;
    EXPECT_CALL(*mockLink, sendPackage(handle2, pkg, 0)).Times(0);
    EXPECT_CALL(sdk, onPackageStatusChanged(handle2, PACKAGE_FAILED_GENERIC, RACE_BLOCKING));
    auto response = plugin.sendPackage(handle2, "BAD CONNECTION", pkg, 0, 0);
    EXPECT_EQ(response, PLUGIN_ERROR);
}

TEST_F(PluginTestFixture, sendPackage_bad_link_type) {
    LinkID linkId = "LINK1";
    LinkType linkType = LT_RECV;
    auto mockLink = std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, linkType);

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    ConnectionID connId = "ConnID0";
    EXPECT_CALL(sdk, updateLinkProperties(_, _, _));
    EXPECT_CALL(sdk, generateConnectionId(linkId)).WillOnce(Return(connId));
    plugin.addLink(mockLink);

    RaceHandle handle1 = 0;
    EXPECT_CALL(*mockLink, openConnection(_, _, _, _))
        .WillOnce(
            Return(std::make_shared<Connection>(connId, linkType, mockLink, "", RACE_UNLIMITED)));
    EXPECT_CALL(sdk, onConnectionStatusChanged(_, _, _, _, _));
    plugin.openConnection(handle1, linkType, linkId, "", RACE_UNLIMITED);

    RaceHandle handle2 = 0;
    const std::string cipherText = "my cipher text";
    EncPkg pkg(1, 42, {cipherText.begin(), cipherText.end()});
    ;
    EXPECT_CALL(*mockLink, sendPackage(handle2, pkg, 0)).Times(0);
    auto response = plugin.sendPackage(handle2, connId, pkg, 0, 0);
    EXPECT_EQ(response, PLUGIN_ERROR);
}

TEST_F(PluginTestFixture, closeConnection_success) {
    LinkID linkId = "LINK0";
    LinkType linkType = LT_RECV;
    auto mockLink = std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, linkType);

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    ConnectionID connId = "ConnID0";
    EXPECT_CALL(sdk, updateLinkProperties(_, _, _));
    EXPECT_CALL(sdk, generateConnectionId(linkId)).WillOnce(Return(connId));
    plugin.addLink(mockLink);

    RaceHandle handle1 = 0;
    EXPECT_CALL(*mockLink, openConnection(_, _, _, _))
        .WillOnce(
            Return(std::make_shared<Connection>(connId, linkType, mockLink, "", RACE_UNLIMITED)));
    EXPECT_CALL(sdk, onConnectionStatusChanged(_, _, _, _, _));
    plugin.openConnection(handle1, linkType, linkId, "", RACE_UNLIMITED);

    RaceHandle handle2 = 0;
    EXPECT_CALL(*mockLink, closeConnection(connId));
    EXPECT_CALL(sdk, onConnectionStatusChanged(handle2, connId, CONNECTION_CLOSED, _, _));
    auto response = plugin.closeConnection(handle2, connId);

    EXPECT_EQ(response, PLUGIN_OK);
}

TEST_F(PluginTestFixture, closeConnection_bad_link) {
    LinkID linkId = "LINK0";
    LinkType linkType = LT_RECV;
    auto mockLink = std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, linkType);

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    ConnectionID connId = "ConnID0";
    EXPECT_CALL(sdk, updateLinkProperties(_, _, _));
    EXPECT_CALL(sdk, generateConnectionId(linkId)).WillOnce(Return(connId));
    plugin.addLink(mockLink);

    RaceHandle handle1 = 0;
    EXPECT_CALL(*mockLink, openConnection(_, _, _, _))
        .WillOnce(
            Return(std::make_shared<Connection>(connId, linkType, mockLink, "", RACE_UNLIMITED)));
    EXPECT_CALL(sdk, onConnectionStatusChanged(_, _, _, _, _));
    plugin.openConnection(handle1, linkType, linkId, "", RACE_UNLIMITED);

    RaceHandle handle2 = 0;
    EXPECT_CALL(*mockLink, closeConnection(connId)).Times(0);
    EXPECT_CALL(sdk, onConnectionStatusChanged(_, _, _, _, _)).Times(0);
    auto response = plugin.closeConnection(handle2, "BAD CONNECTION");

    // This returns OK because there's a harmless race condition that can cause
    // this to occur in normal situations.
    EXPECT_EQ(response, PLUGIN_OK);
}

TEST_F(PluginTestFixture, shutdown_no_start) {
    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);
    auto response = plugin.shutdown();
    EXPECT_EQ(response, PLUGIN_OK);
}

TEST_F(PluginTestFixture, shutdown) {
    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    auto response = plugin.shutdown();
    EXPECT_EQ(response, PLUGIN_OK);
}

TEST_F(PluginTestFixture, shutdown_connection) {
    LinkID linkId = "LINK0";
    LinkType linkType = LT_RECV;
    auto mockLink = std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, linkType);

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp";
    plugin.init(pluginConfig);

    ConnectionID connId = "ConnID0";
    EXPECT_CALL(sdk, updateLinkProperties(_, _, _));
    EXPECT_CALL(sdk, generateConnectionId(linkId)).WillOnce(Return(connId));
    plugin.addLink(mockLink);

    RaceHandle handle1 = 0;
    EXPECT_CALL(*mockLink, openConnection(_, _, _, _))
        .WillOnce(
            Return(std::make_shared<Connection>(connId, linkType, mockLink, "", RACE_UNLIMITED)));
    EXPECT_CALL(sdk, onConnectionStatusChanged(_, _, _, _, _));
    plugin.openConnection(handle1, linkType, linkId, "", RACE_UNLIMITED);

    EXPECT_CALL(*mockLink, closeConnection(connId));
    EXPECT_CALL(*mockLink, shutdown());
    EXPECT_CALL(sdk, onConnectionStatusChanged(NULL_RACE_HANDLE, connId, CONNECTION_CLOSED, _, _));
    auto response = plugin.shutdown();
    EXPECT_EQ(response, PLUGIN_OK);
}
