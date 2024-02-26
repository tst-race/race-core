
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

#include "../../source/base/Channel.h"

#include <memory>

#include "MockChannel.h"
#include "MockLink.h"
#include "MockPluginComms.h"
#include "gtest/gtest.h"
#include "race/mocks/MockRaceSdkComms.h"

using ::testing::_;
using ::testing::Return;
using P = std::vector<std::string>;

class TestChannel : public Channel {
public:
    explicit TestChannel(PluginCommsTwoSixCpp &plugin) : Channel(plugin, "TestChannel") {}
    virtual PluginResponse createLink(RaceHandle handle) override {
        return Channel::createLink(handle);
    }
    MOCK_METHOD(std::shared_ptr<Link>, createLink, (const LinkID &linkId), (override));
    MOCK_METHOD(std::shared_ptr<Link>, createLinkFromAddress,
                (const LinkID &linkId, const std::string &linkAddress), (override));
    MOCK_METHOD(std::shared_ptr<Link>, createBootstrapLink,
                (const LinkID &linkId, const std::string &passphrase), (override));
    MOCK_METHOD(std::shared_ptr<Link>, loadLink,
                (const LinkID &linkId, const std::string &linkAddress), (override));
    MOCK_METHOD(void, onLinkDestroyedInternal, (Link * link), (override));

    MOCK_METHOD(void, onGenesisLinkCreated, (Link * link), (override));

    MOCK_METHOD(PluginResponse, activateChannelInternal, (RaceHandle handle), (override));
    MOCK_METHOD(LinkProperties, getDefaultLinkProperties, (), (override));
};

class ChannelTestFixture : public ::testing::Test {
public:
    ChannelTestFixture() : plugin(sdk), channel(plugin) {
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
    virtual ~ChannelTestFixture() {}
    virtual void SetUp() override {}
    virtual void TearDown() override {}

public:
    MockRaceSdkComms sdk;
    MockPluginComms plugin;
    TestChannel channel;

private:
    int linkCounter{0};
    int connectionCounter{0};
};

TEST_F(ChannelTestFixture, createLink) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_BOTH;

    EXPECT_CALL(channel, createLink(_)).WillOnce([&](const std::string &linkId) {
        return std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, LT_BIDI);
    });
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(sdk, onChannelStatusChanged(_, _, _, _, _)).Times(0);
    EXPECT_CALL(plugin, addLink(_)).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    channel.createLink(handle);
}

TEST_F(ChannelTestFixture, createLink_cr_creator) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_CREATOR;

    EXPECT_CALL(channel, createLink(_)).WillOnce([&](const std::string &linkId) {
        return std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, LT_BIDI);
    });
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    EXPECT_CALL(sdk, onChannelStatusChanged(_, _, _, _, _)).Times(0);
    channel.createLink(handle);
}

TEST_F(ChannelTestFixture, createLink_channel_not_available) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    channel.status = CHANNEL_UNAVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_BOTH;

    EXPECT_CALL(channel, createLink(_)).Times(0);
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).Times(0).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    channel.createLink(handle);
}

TEST_F(ChannelTestFixture, createLink_too_many_links) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 0;
    channel.properties.currentRole.linkSide = LS_BOTH;

    EXPECT_CALL(channel, createLink(_)).Times(0);
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).Times(0).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    channel.createLink(handle);
}

TEST_F(ChannelTestFixture, createLink_cr_loader) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_LOADER;

    EXPECT_CALL(channel, createLink(_)).Times(0);
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).Times(0).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    channel.createLink(handle);
}

TEST_F(ChannelTestFixture, createLinkFromAddress) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string address = "some address";
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_BOTH;

    EXPECT_CALL(channel, createLinkFromAddress(_, _))
        .WillOnce([&](const std::string &linkId, const std::string &) {
            return std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, LT_BIDI);
        });
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    EXPECT_CALL(sdk, onChannelStatusChanged(_, _, _, _, _)).Times(0);
    channel.Channel::createLinkFromAddress(handle, address);
}

TEST_F(ChannelTestFixture, createLinkFromAddress_cr_creator) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string address = "some address";
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_CREATOR;

    EXPECT_CALL(channel, createLinkFromAddress(_, _))
        .WillOnce([&](const std::string &linkId, const std::string &) {
            return std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, LT_BIDI);
        });
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    EXPECT_CALL(sdk, onChannelStatusChanged(_, _, _, _, _)).Times(0);
    channel.Channel::createLinkFromAddress(handle, address);
}

TEST_F(ChannelTestFixture, createLinkFromAddress_channel_not_available) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string address = "some address";
    channel.status = CHANNEL_UNAVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_BOTH;

    EXPECT_CALL(channel, createLinkFromAddress(_, _)).Times(0);
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).Times(0).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    channel.Channel::createLinkFromAddress(handle, address);
}

TEST_F(ChannelTestFixture, createLinkFromAddress_too_many_links) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string address = "some address";
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 0;
    channel.properties.currentRole.linkSide = LS_BOTH;

    EXPECT_CALL(channel, createLinkFromAddress(_, _)).Times(0);
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).Times(0).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    channel.Channel::createLinkFromAddress(handle, address);
}

TEST_F(ChannelTestFixture, createLinkFromAddress_incorrect_role) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string address = "some address";
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_LOADER;

    EXPECT_CALL(channel, createLinkFromAddress(_, _)).Times(0);
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).Times(0).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    channel.Channel::createLinkFromAddress(handle, address);
}

TEST_F(ChannelTestFixture, loadLinkAddress) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string address = "some address";
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_BOTH;

    EXPECT_CALL(channel, loadLink(_, _))
        .WillOnce([&](const std::string &linkId, const std::string &) {
            return std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, LT_BIDI);
        });
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    EXPECT_CALL(sdk, onChannelStatusChanged(_, _, _, _, _)).Times(0);
    channel.Channel::loadLinkAddress(handle, address);
}

TEST_F(ChannelTestFixture, loadLinkAddress_cr_loader) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string address = "some address";
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_LOADER;

    EXPECT_CALL(channel, loadLink(_, _))
        .WillOnce([&](const std::string &linkId, const std::string &) {
            return std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, LT_BIDI);
        });
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    EXPECT_CALL(sdk, onChannelStatusChanged(_, _, _, _, _)).Times(0);
    channel.Channel::loadLinkAddress(handle, address);
}

TEST_F(ChannelTestFixture, loadLinkAddress_channel_not_available) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string address = "some address";
    channel.status = CHANNEL_UNAVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_BOTH;

    EXPECT_CALL(channel, loadLink(_, _)).Times(0);
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).Times(0).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    channel.Channel::loadLinkAddress(handle, address);
}

TEST_F(ChannelTestFixture, loadLinkAddress_too_many_links) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string address = "some address";
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 0;
    channel.properties.currentRole.linkSide = LS_BOTH;

    EXPECT_CALL(channel, loadLink(_, _)).Times(0);
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).Times(0).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    channel.Channel::loadLinkAddress(handle, address);
}

TEST_F(ChannelTestFixture, loadLinkAddress_cr_creator) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string address = "some address";
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_CREATOR;

    EXPECT_CALL(channel, loadLink(_, _)).Times(0);
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).Times(0).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    channel.Channel::loadLinkAddress(handle, address);
}

TEST_F(ChannelTestFixture, createBootstrapLink) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string passphrase = "some passphrase";
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_BOTH;

    EXPECT_CALL(channel, createBootstrapLink(_, _))
        .WillOnce([&](const std::string &linkId, const std::string &) {
            return std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, LT_BIDI);
        });
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    EXPECT_CALL(sdk, onChannelStatusChanged(_, _, _, _, _)).Times(0);
    channel.Channel::createBootstrapLink(handle, passphrase);
}

TEST_F(ChannelTestFixture, createBootstrapLink_cr_creator) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string passphrase = "some passphrase";
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_CREATOR;

    EXPECT_CALL(channel, createBootstrapLink(_, _))
        .WillOnce([&](const std::string &linkId, const std::string &) {
            return std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, LT_BIDI);
        });
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    EXPECT_CALL(sdk, onChannelStatusChanged(_, _, _, _, _)).Times(0);
    channel.Channel::createBootstrapLink(handle, passphrase);
}

TEST_F(ChannelTestFixture, createBootstrapLink_cr_loader) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string passphrase = "some passphrase";
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_LOADER;

    EXPECT_CALL(channel, createBootstrapLink(_, _))
        .WillOnce([&](const std::string &linkId, const std::string &) {
            return std::make_shared<MockLink>(&sdk, &plugin, &channel, linkId, LT_BIDI);
        });
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    EXPECT_CALL(sdk, onChannelStatusChanged(_, _, _, _, _)).Times(0);
    channel.Channel::createBootstrapLink(handle, passphrase);
}

TEST_F(ChannelTestFixture, createBootstrapLink_channel_not_available) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string passphrase = "some passphrase";
    channel.status = CHANNEL_UNAVAILABLE;
    channel.properties.maxLinks = 1;
    channel.properties.currentRole.linkSide = LS_BOTH;

    EXPECT_CALL(channel, createBootstrapLink(_, _)).Times(0);
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).Times(0).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    channel.Channel::createBootstrapLink(handle, passphrase);
}

TEST_F(ChannelTestFixture, createBootstrapLink_too_many_links) {
    std::shared_ptr<Link> link;
    RaceHandle handle = 1;
    std::string passphrase = "some passphrase";
    channel.status = CHANNEL_AVAILABLE;
    channel.properties.maxLinks = 0;
    channel.properties.currentRole.linkSide = LS_BOTH;

    EXPECT_CALL(channel, createBootstrapLink(_, _)).Times(0);
    EXPECT_CALL(sdk, onLinkStatusChanged(handle, _, _, _, _));
    EXPECT_CALL(plugin, addLink(_)).Times(0).WillOnce([&](std::shared_ptr<Link> new_link) {
        link = new_link;
    });
    channel.Channel::createBootstrapLink(handle, passphrase);
}
