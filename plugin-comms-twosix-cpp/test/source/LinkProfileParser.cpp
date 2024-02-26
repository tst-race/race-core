
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

#include "../../source/base/LinkProfileParser.h"

#include "../../source/config/LinkConfig.h"
#include "../../source/direct/DirectLink.h"
#include "../../source/direct/DirectLinkProfileParser.h"
#include "../../source/whiteboard/TwosixWhiteboardLink.h"
#include "../../source/whiteboard/TwosixWhiteboardLinkProfileParser.h"
#include "MockChannel.h"
#include "MockPluginComms.h"
#include "gtest/gtest.h"
#include "race/mocks/MockRaceSdkComms.h"

TEST(LinkProfileParser, parse_empty_fail) {
    const std::string linkProfile = R"()";

    auto linkParser = LinkProfileParser::parse(linkProfile);
    ASSERT_EQ(linkParser, nullptr);
}

TEST(LinkProfileParser, parse_array_fail) {
    const std::string linkProfile = R"([])";

    auto linkParser = LinkProfileParser::parse(linkProfile);
    ASSERT_EQ(linkParser, nullptr);
}

TEST(LinkProfileParser, parse_empty_object_fail) {
    const std::string linkProfile = R"({})";

    auto linkParser = LinkProfileParser::parse(linkProfile);
    ASSERT_EQ(linkParser, nullptr);
}

TEST(LinkProfileParser, parse_multicast_object_no_service_name_fail) {
    const std::string linkProfile = R"({
        "multicast": true
    })";

    auto linkParser = LinkProfileParser::parse(linkProfile);
    ASSERT_EQ(linkParser, nullptr);
}

TEST(LinkProfileParser, parse_multicast_object_twosix_fail) {
    const std::string linkProfile = R"({
        "multicast": true,
        "service_name": "twosix-whiteboard"
    })";

    auto linkParser = LinkProfileParser::parse(linkProfile);
    ASSERT_EQ(linkParser, nullptr);
}

TEST(LinkProfileParser, parse_unicast_object_twosix_fail) {
    const std::string linkProfile = R"({
        "multicast": false
    })";

    auto linkParser = LinkProfileParser::parse(linkProfile);
    ASSERT_EQ(linkParser, nullptr);
}

TEST(LinkProfileParser, parse_direct_success) {
    const std::string linkProfile = R"({
        "multicast": false,
        "hostname": "test-host",
        "port": 1234
    })";

    LinkConfig linkConfig;
    linkConfig.linkProfile = linkProfile;
    linkConfig.linkProps.linkType = LT_RECV;
    linkConfig.linkProps.transmissionType = TT_UNICAST;
    linkConfig.linkProps.connectionType = CT_DIRECT;

    auto linkParser = LinkProfileParser::parse(linkProfile);
    ASSERT_NE(linkParser, nullptr);
    auto linkParser2 = dynamic_cast<DirectLinkProfileParser *>(linkParser.get());
    ASSERT_NE(linkParser2, nullptr);
    EXPECT_EQ(linkParser2->hostname, "test-host");
    EXPECT_EQ(linkParser2->port, 1234);

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    auto link = linkParser->createLink(&sdk, &plugin, &channel, linkConfig, "testChannelGid");
    ASSERT_NE(dynamic_cast<DirectLink *>(link.get()), nullptr);
}

TEST(LinkProfileParser, parse_direct_missing_optional_success) {
    const std::string linkProfile = R"({
        "hostname": "test-host2",
        "port": 12345
    })";

    LinkConfig linkConfig;
    linkConfig.linkProfile = linkProfile;
    linkConfig.linkProps.linkType = LT_RECV;
    linkConfig.linkProps.transmissionType = TT_UNICAST;
    linkConfig.linkProps.connectionType = CT_DIRECT;

    auto linkParser = LinkProfileParser::parse(linkProfile);
    ASSERT_NE(linkParser, nullptr);
    auto linkParser2 = dynamic_cast<DirectLinkProfileParser *>(linkParser.get());
    ASSERT_NE(linkParser2, nullptr);
    EXPECT_EQ(linkParser2->hostname, "test-host2");
    EXPECT_EQ(linkParser2->port, 12345);

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    auto link = linkParser->createLink(&sdk, &plugin, &channel, linkConfig, "testChannelGid");
    ASSERT_NE(dynamic_cast<DirectLink *>(link.get()), nullptr);
}

TEST(LinkProfileParser, parse_twosix_whiteboard_success) {
    const std::string linkProfile = R"({
        "multicast": true,
        "service_name": "twosix-whiteboard",
        "hostname": "test-host",
        "port": 1234,
        "hashtag": "tag",
        "checkFrequency": 2
    })";

    LinkConfig linkConfig;
    linkConfig.linkProfile = linkProfile;
    linkConfig.linkProps.linkType = LT_RECV;
    linkConfig.linkProps.transmissionType = TT_MULTICAST;
    linkConfig.linkProps.connectionType = CT_INDIRECT;

    auto linkParser = LinkProfileParser::parse(linkProfile);
    ASSERT_NE(linkParser, nullptr);
    auto linkParser2 = dynamic_cast<TwosixWhiteboardLinkProfileParser *>(linkParser.get());
    ASSERT_NE(linkParser2, nullptr);
    EXPECT_EQ(linkParser2->hostname, "test-host");
    EXPECT_EQ(linkParser2->port, 1234);
    EXPECT_EQ(linkParser2->hashtag, "tag");
    EXPECT_EQ(linkParser2->checkFrequency, 2);

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    auto link = linkParser->createLink(&sdk, &plugin, &channel, linkConfig, "testChannelGid");
    ASSERT_NE(dynamic_cast<TwosixWhiteboardLink *>(link.get()), nullptr);
}

TEST(LinkProfileParser, parse_twosix_whiteboard_fix_tag_success) {
    const std::string linkProfile = R"({
        "multicast": true,
        "service_name": "twosix-whiteboard",
        "hostname": "test-host",
        "port": 1234,
        "hashtag": "some/tag",
        "checkFrequency": 2
    })";

    LinkConfig linkConfig;
    linkConfig.linkProfile = linkProfile;
    linkConfig.linkProps.linkType = LT_RECV;
    linkConfig.linkProps.transmissionType = TT_MULTICAST;
    linkConfig.linkProps.connectionType = CT_INDIRECT;

    auto linkParser = LinkProfileParser::parse(linkProfile);
    ASSERT_NE(linkParser, nullptr);
    auto linkParser2 = dynamic_cast<TwosixWhiteboardLinkProfileParser *>(linkParser.get());
    ASSERT_NE(linkParser2, nullptr);
    EXPECT_EQ(linkParser2->hostname, "test-host");
    EXPECT_EQ(linkParser2->port, 1234);

    // the tag has had the / removed, as it would cause problems when creating the url to access
    EXPECT_EQ(linkParser2->hashtag, "sometag");
    EXPECT_EQ(linkParser2->checkFrequency, 2);

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    auto link = linkParser->createLink(&sdk, &plugin, &channel, linkConfig, "testChannelGid");
    ASSERT_NE(dynamic_cast<TwosixWhiteboardLink *>(link.get()), nullptr);
}

TEST(LinkProfileParser, parse_twosix_whiteboard_missing_optional_success) {
    const std::string linkProfile = R"({
        "multicast": true,
        "service_name": "twosix-whiteboard",
        "hostname": "test-host2",
        "port": 12345,
        "hashtag": "tag2"
    })";

    LinkConfig linkConfig;
    linkConfig.linkProfile = linkProfile;
    linkConfig.linkProps.linkType = LT_RECV;
    linkConfig.linkProps.transmissionType = TT_MULTICAST;
    linkConfig.linkProps.connectionType = CT_INDIRECT;

    auto linkParser = LinkProfileParser::parse(linkProfile);
    ASSERT_NE(linkParser, nullptr);
    auto linkParser2 = dynamic_cast<TwosixWhiteboardLinkProfileParser *>(linkParser.get());
    ASSERT_NE(linkParser2, nullptr);
    EXPECT_EQ(linkParser2->hostname, "test-host2");
    EXPECT_EQ(linkParser2->port, 12345);
    EXPECT_EQ(linkParser2->hashtag, "tag2");
    EXPECT_EQ(linkParser2->checkFrequency, 1000);

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    auto link = linkParser->createLink(&sdk, &plugin, &channel, linkConfig, "testChannelGid");
    ASSERT_NE(dynamic_cast<TwosixWhiteboardLink *>(link.get()), nullptr);
}
