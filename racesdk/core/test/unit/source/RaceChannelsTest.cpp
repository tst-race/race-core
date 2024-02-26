
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

#include "../../../include/RaceChannels.h"
#include "../../common/MockRaceSdk.h"
#include "../../common/race_printers.h"
#include "gmock/gmock.h"

TEST(RaceChannels, add_channel) {
    RaceChannels channels;
    std::string channelGid = "channel1";
    ChannelProperties props;
    props.channelGid = channelGid;
    channels.add(props);
    EXPECT_EQ(channels.getChannelProperties(channelGid), props);
}

TEST(RaceChannels, nonexistent_is_not_available) {
    RaceChannels channels;
    EXPECT_FALSE(channels.isAvailable("nonexistent"));
}

TEST(RaceChannels, added_channel_is_not_available) {
    RaceChannels channels;
    std::string channelGid = "channel1";
    ChannelProperties props;
    props.channelGid = channelGid;
    channels.add(props);
    EXPECT_FALSE(channels.isAvailable(channelGid));
}

TEST(RaceChannels, update_works) {
    RaceChannels channels;
    std::string channelGid = "channel1";
    ChannelProperties props;
    props.channelGid = channelGid;
    channels.add(props);
    channels.update(channelGid, CHANNEL_AVAILABLE, props);
    EXPECT_TRUE(channels.isAvailable(channelGid));
}

TEST(RaceChannels, getWrapperIdForChannel_throws_on_nonexistent_channel) {
    RaceChannels channels;
    std::string channelGid = "channel1";
    ASSERT_THROW(channels.getWrapperIdForChannel(channelGid), std::out_of_range);
}

TEST(RaceChannels, getPluginsForChannel_throws_on_nonexistent_channel) {
    RaceChannels channels;
    std::string channelGid = "channel1";
    ASSERT_THROW(channels.getPluginsForChannel(channelGid), std::out_of_range);
}

TEST(RaceChannels, getWrapperIdForChannel_works) {
    RaceChannels channels;
    std::string channelGid = "channel1";
    std::string pluginId = "plugin1";
    ChannelProperties props;
    props.channelGid = channelGid;
    channels.add(props);
    channels.setWrapperIdForChannel(channelGid, pluginId);
    EXPECT_EQ(channels.getWrapperIdForChannel(channelGid), pluginId);
}

TEST(RaceChannels, getPluginsForChannel_works) {
    RaceChannels channels;
    std::string channelGid = "channel1";
    std::string pluginId1 = "plugin1";
    std::string pluginId2 = "plugin2";
    ChannelProperties props;
    props.channelGid = channelGid;
    channels.add(props);
    channels.setPluginsForChannel(channelGid, {pluginId1, pluginId2});
    std::vector<std::string> expected = {pluginId1, pluginId2};
    EXPECT_EQ(channels.getPluginsForChannel(channelGid), expected);
}

TEST(RaceChannels, getSupportedChannels_works) {
    RaceChannels channels;
    std::string channelGid1 = "channel1";
    std::string channelGid2 = "channel2";
    ChannelProperties props1;
    ChannelProperties props2;
    props1.channelGid = channelGid1;
    props2.channelGid = channelGid2;
    channels.add(props1);
    channels.add(props2);
    channels.update(channelGid1, CHANNEL_AVAILABLE, props1);
    channels.update(channelGid2, CHANNEL_AVAILABLE, props2);
    auto supported = channels.getSupportedChannels();
    EXPECT_EQ(supported.size(), 2);
    EXPECT_EQ(supported.count(channelGid1), 1);
    EXPECT_EQ(supported.count(channelGid2), 1);
}

TEST(RaceChannels, getSupportedChannels_skips_unavailable) {
    RaceChannels channels;
    std::string channelGid1 = "channel1";
    std::string channelGid2 = "channel2";
    ChannelProperties props1;
    ChannelProperties props2;
    props1.channelGid = channelGid1;
    props2.channelGid = channelGid2;
    channels.add(props1);
    channels.add(props2);
    channels.update(channelGid1, CHANNEL_AVAILABLE, props1);
    channels.update(channelGid2, CHANNEL_UNAVAILABLE, props2);
    auto supported = channels.getSupportedChannels();
    EXPECT_EQ(supported.size(), 1);
    EXPECT_EQ(supported.count(channelGid1), 1);
}

TEST(RaceChannels, getLinksForChannel_single_link) {
    RaceChannels channels;
    std::string channelGid1 = "testchannel1";
    LinkID linkId1 = "testlink1";
    channels.setLinkId(channelGid1, linkId1);
    std::vector<LinkID> expectedLinks = {linkId1};
    std::vector<LinkID> actualLinks = channels.getLinksForChannel(channelGid1);
    EXPECT_THAT(expectedLinks, ::testing::ContainerEq(actualLinks));
}

TEST(RaceChannels, getLinksForChannel_repeat_links) {
    RaceChannels channels;
    std::string channelGid1 = "testchannel1";
    LinkID linkId1 = "testlink1";
    LinkID linkId2 = "testlink2";
    channels.setLinkId(channelGid1, linkId1);
    channels.setLinkId(channelGid1, linkId2);
    // add linkId1 again to test that repeats are excluded
    channels.setLinkId(channelGid1, linkId1);
    std::vector<LinkID> expectedLinks = {linkId2, linkId1};
    std::vector<LinkID> actualLinks = channels.getLinksForChannel(channelGid1);
    EXPECT_THAT(expectedLinks, ::testing::ContainerEq(actualLinks));
}

TEST(RaceChannels, getLinksForChannel_multiple_links) {
    RaceChannels channels;
    std::string channelGid1 = "testchannel1";
    std::string channelGid2 = "testchannel2";
    LinkID linkId1 = "testlink1";
    LinkID linkId2 = "testlink2";
    LinkID linkId3 = "testlink3";
    channels.setLinkId(channelGid1, linkId1);
    channels.setLinkId(channelGid1, linkId2);
    channels.setLinkId(channelGid2, linkId1);
    channels.setLinkId(channelGid2, linkId2);
    channels.setLinkId(channelGid2, linkId3);
    std::unordered_set<LinkID> expectedLinks1 = {linkId2, linkId1};
    std::vector<LinkID> actualLinks1 = channels.getLinksForChannel(channelGid1);
    EXPECT_THAT(expectedLinks1, ::testing::ContainerEq(std::unordered_set<LinkID>(
                                    actualLinks1.begin(), actualLinks1.end())));
    std::vector<LinkID> expectedLinks2 = {linkId1, linkId2, linkId3};
    std::vector<LinkID> actualLinks2 = channels.getLinksForChannel(channelGid2);
    std::sort(actualLinks2.begin(), actualLinks2.end());
    EXPECT_THAT(expectedLinks2, ::testing::ContainerEq(actualLinks2));
}

TEST(RaceChannels, getLinksForChannel_removing_links) {
    RaceChannels channels;
    std::string channelGid1 = "testchannel1";
    LinkID linkId1 = "testlink1";
    LinkID linkId2 = "testlink2";
    channels.setLinkId(channelGid1, linkId1);
    channels.setLinkId(channelGid1, linkId2);
    channels.removeLinkId(channelGid1, linkId1);
    std::vector<LinkID> expectedLinks = {linkId2};
    std::vector<LinkID> actualLinks = channels.getLinksForChannel(channelGid1);
    EXPECT_THAT(expectedLinks, ::testing::ContainerEq(actualLinks));
}

TEST(RaceChannels, getLinksForChannel_remove_nonexistent_links) {
    RaceChannels channels;
    std::string channelGid1 = "testchannel1";
    LinkID linkId1 = "testlink1";
    LinkID linkId2 = "testlink2";
    channels.setLinkId(channelGid1, linkId1);
    channels.removeLinkId(channelGid1, linkId2);
    std::vector<LinkID> expectedLinks = {linkId1};
    std::vector<LinkID> actualLinks = channels.getLinksForChannel(channelGid1);
    EXPECT_THAT(expectedLinks, ::testing::ContainerEq(actualLinks));
}

TEST(RaceChannels, getLinksForChannel_remove_links_from_nonexistent_channel) {
    RaceChannels channels;
    std::string channelGid1 = "testchannel1";
    LinkID linkId1 = "testlink1";
    channels.removeLinkId(channelGid1, linkId1);
    std::vector<LinkID> expectedLinks = {};
    std::vector<LinkID> actualLinks = channels.getLinksForChannel(channelGid1);
    EXPECT_THAT(expectedLinks, ::testing::ContainerEq(actualLinks));
}

TEST(RaceChannels, activateChannel) {
    RaceChannels channels;
    ChannelProperties props;
    props.channelStatus = CHANNEL_ENABLED;
    props.channelGid = "testchannel1";
    props.roles.push_back(ChannelRole{});
    props.roles.back().linkSide = LS_BOTH;

    channels.add(props);
    channels.activate(props.channelGid, "");
    EXPECT_EQ(channels.getChannelProperties(props.channelGid).channelStatus, CHANNEL_STARTING);
}

TEST(RaceChannels, activateChannel_invalid_channel) {
    RaceChannels channels;
    ChannelProperties props;
    props.channelStatus = CHANNEL_ENABLED;
    props.channelGid = "testchannel1";
    props.roles.push_back(ChannelRole{});
    props.roles.back().linkSide = LS_BOTH;

    channels.add(props);
    channels.activate("invalid channel", "");
    EXPECT_EQ(channels.getChannelProperties(props.channelGid).channelStatus, CHANNEL_ENABLED);
}

TEST(RaceChannels, activateChannel_not_enabled) {
    RaceChannels channels;
    ChannelProperties props;
    props.channelStatus = CHANNEL_DISABLED;
    props.channelGid = "testchannel1";
    props.roles.push_back(ChannelRole{});
    props.roles.back().linkSide = LS_BOTH;

    channels.add(props);
    channels.activate(props.channelGid, "");
    EXPECT_EQ(channels.getChannelProperties(props.channelGid).channelStatus, CHANNEL_DISABLED);
}

TEST(RaceChannels, activateChannel_invalid_role) {
    RaceChannels channels;
    ChannelProperties props;
    props.channelStatus = CHANNEL_ENABLED;
    props.channelGid = "testchannel1";
    props.roles.push_back(ChannelRole{});
    props.roles.back().linkSide = LS_BOTH;

    channels.add(props);
    channels.activate(props.channelGid, "invalid role");
    EXPECT_EQ(channels.getChannelProperties(props.channelGid).channelStatus, CHANNEL_ENABLED);
}

TEST(RaceChannels, activateChannel_mechanical_tag_conflict) {
    RaceChannels channels;
    ChannelProperties props1;
    props1.channelStatus = CHANNEL_ENABLED;
    props1.channelGid = "testchannel1";
    props1.roles.push_back(ChannelRole{});
    props1.roles.back().linkSide = LS_BOTH;
    props1.roles.back().mechanicalTags = {"mechanical_tag_1"};

    channels.add(props1);
    channels.activate(props1.channelGid, "");

    ChannelProperties props2;
    props2.channelStatus = CHANNEL_ENABLED;
    props2.channelGid = "testchannel2";
    props2.roles.push_back(ChannelRole{});
    props2.roles.back().linkSide = LS_BOTH;
    props2.roles.back().mechanicalTags = {"mechanical_tag_1"};

    channels.add(props2);
    channels.activate(props2.channelGid, "");
    EXPECT_EQ(channels.getChannelProperties(props2.channelGid).channelStatus, CHANNEL_ENABLED);
}

TEST(RaceChannels, activateChannel_mechanical_tag_no_conflict) {
    RaceChannels channels;
    ChannelProperties props1;
    props1.channelStatus = CHANNEL_ENABLED;
    props1.channelGid = "testchannel1";
    props1.roles.push_back(ChannelRole{});
    props1.roles.back().linkSide = LS_BOTH;
    props1.roles.back().mechanicalTags = {"mechanical_tag_1"};

    channels.add(props1);
    channels.activate(props1.channelGid, "");

    ChannelProperties props2;
    props2.channelStatus = CHANNEL_ENABLED;
    props2.channelGid = "testchannel2";
    props2.roles.push_back(ChannelRole{});
    props2.roles.back().linkSide = LS_BOTH;
    props2.roles.back().mechanicalTags = {"mechanical_tag_2"};

    channels.add(props2);
    channels.activate(props2.channelGid, "");
    EXPECT_EQ(channels.getChannelProperties(props1.channelGid).channelStatus, CHANNEL_STARTING);
}

TEST(RaceChannels, activateChannel_behavioral_tag_conflict) {
    RaceChannels channels;
    channels.setAllowedTags({"tag1", "tag2", "tag3"});
    ChannelProperties props1;
    props1.channelStatus = CHANNEL_ENABLED;
    props1.channelGid = "testchannel1";
    props1.roles.push_back(ChannelRole{});
    props1.roles.back().linkSide = LS_BOTH;
    props1.roles.back().behavioralTags = {"tag5"};

    channels.add(props1);
    channels.activate(props1.channelGid, "");

    EXPECT_EQ(channels.getChannelProperties(props1.channelGid).channelStatus, CHANNEL_ENABLED);
}

TEST(RaceChannels, activateChannel_behavioral_tag_no_conflict) {
    RaceChannels channels;
    channels.setAllowedTags({"tag1", "tag2", "tag3"});

    ChannelProperties props1;
    props1.channelStatus = CHANNEL_ENABLED;
    props1.channelGid = "testchannel1";
    props1.roles.push_back(ChannelRole{});
    props1.roles.back().linkSide = LS_BOTH;
    props1.roles.back().behavioralTags = {"tag1"};

    channels.add(props1);
    channels.activate(props1.channelGid, "");

    EXPECT_EQ(channels.getChannelProperties(props1.channelGid).channelStatus, CHANNEL_STARTING);
}

TEST(RaceChannels, getPluginChannelIds) {
    RaceChannels channels;
    std::vector<std::string> channelIds;
    channels.setAllowedTags({"tag1", "tag2", "tag3"});
    std::string pluginID = "pluginId";
    ChannelProperties props1;
    props1.channelStatus = CHANNEL_ENABLED;
    props1.channelGid = "testchannel1";
    props1.roles.push_back(ChannelRole{});
    props1.roles.back().linkSide = LS_BOTH;
    props1.roles.back().behavioralTags = {"tag1"};

    channels.add(props1);
    channels.setPluginsForChannel(props1.channelGid, {pluginID});
    channels.setWrapperIdForChannel(props1.channelGid, pluginID);
    channelIds = channels.getPluginChannelIds(pluginID);

    EXPECT_EQ(channelIds.size(), 1);
    EXPECT_EQ(*channelIds.begin(), props1.channelGid);
}

TEST(RaceChannels, setUserEnabledChannels) {
    MockRaceSdk sdk;
    std::string expected = "[\n    \"channel1\",\n    \"channel2\"\n]";
    EXPECT_CALL(sdk,
                writeFile(::testing::_, std::vector<uint8_t>(expected.begin(), expected.end())));

    RaceChannels channels{{}, &sdk};

    EXPECT_FALSE(channels.isUserEnabled("channel1"));
    EXPECT_FALSE(channels.isUserEnabled("channel2"));

    channels.setUserEnabledChannels({"channel1", "channel2", "channel1"});

    EXPECT_TRUE(channels.isUserEnabled("channel1"));
    EXPECT_TRUE(channels.isUserEnabled("channel2"));
}

TEST(RaceChannels, setUserEnabled) {
    MockRaceSdk sdk;
    std::string expected = "[\n    \"channel1\"\n]";
    SdkResponse response;
    response.status = SDK_OK;
    EXPECT_CALL(sdk,
                writeFile(::testing::_, std::vector<uint8_t>(expected.begin(), expected.end())))
        .WillOnce(::testing::Return(response));
    expected = "[\n    \"channel1\",\n    \"channel2\"\n]";
    EXPECT_CALL(sdk,
                writeFile(::testing::_, std::vector<uint8_t>(expected.begin(), expected.end())))
        .WillOnce(::testing::Return(response));

    RaceChannels channels{{}, &sdk};

    EXPECT_FALSE(channels.isUserEnabled("channel1"));
    EXPECT_FALSE(channels.isUserEnabled("channel2"));

    channels.setUserEnabled("channel1");

    EXPECT_TRUE(channels.isUserEnabled("channel1"));
    EXPECT_FALSE(channels.isUserEnabled("channel2"));

    channels.setUserEnabled("channel1");
    channels.setUserEnabled("channel2");

    EXPECT_TRUE(channels.isUserEnabled("channel1"));
    EXPECT_TRUE(channels.isUserEnabled("channel2"));
}

TEST(RaceChannels, setUserDisabled) {
    MockRaceSdk sdk;
    std::string contents = "[\n    \"channel1\",\n    \"channel2\"\n]";
    EXPECT_CALL(sdk, readFile(::testing::_))
        .WillOnce(::testing::Return(std::vector<uint8_t>(contents.begin(), contents.end())))
        .WillRepeatedly(::testing::Return(std::vector<uint8_t>()));

    std::string expected = "[\n    \"channel1\"\n]";
    EXPECT_CALL(sdk,
                writeFile(::testing::_, std::vector<uint8_t>(expected.begin(), expected.end())));
    expected = "[]";
    EXPECT_CALL(sdk,
                writeFile(::testing::_, std::vector<uint8_t>(expected.begin(), expected.end())));

    RaceChannels channels{{}, &sdk};

    EXPECT_TRUE(channels.isUserEnabled("channel1"));
    EXPECT_TRUE(channels.isUserEnabled("channel2"));

    channels.setUserDisabled("channel2");

    EXPECT_TRUE(channels.isUserEnabled("channel1"));
    EXPECT_FALSE(channels.isUserEnabled("channel2"));

    channels.setUserDisabled("channel2");
    channels.setUserDisabled("channel1");

    EXPECT_FALSE(channels.isUserEnabled("channel1"));
    EXPECT_FALSE(channels.isUserEnabled("channel2"));
}