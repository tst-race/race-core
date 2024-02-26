
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

#include "ChannelProperties.h"
#include "gtest/gtest.h"

/**
 * @brief Validate that the size of ChannelProperties has not changed. If it has then these tests
 * will very likely need to be updated.
 *
 */
inline void validateChannelPropertiesSize() {
    const size_t expectedSize = 296;
    EXPECT_EQ(sizeof(ChannelProperties), expectedSize)
        << "If this fails, then this test likely needs updating because new fields have been added "
           "to ChannelProperties. If this is the case please add explicit tests for these fields "
           "and "
           "then update `expectedSize` to the value of the current size.";
}

TEST(ChannelProperties, constructor) {
    validateChannelPropertiesSize();

    ChannelProperties properties;

    EXPECT_EQ(properties.linkDirection, LD_UNDEF);
    EXPECT_EQ(properties.transmissionType, TT_UNDEF);
    EXPECT_EQ(properties.connectionType, CT_UNDEF);
    EXPECT_EQ(properties.sendType, ST_UNDEF);
    EXPECT_EQ(properties.multiAddressable, false);
    EXPECT_EQ(properties.reliable, false);
    EXPECT_EQ(properties.duration_s, -1);
    EXPECT_EQ(properties.period_s, -1);
    EXPECT_EQ(properties.mtu, -1);
    EXPECT_EQ(properties.supported_hints, std::vector<std::string>());
    EXPECT_EQ(properties.channelGid, std::string());

    EXPECT_EQ(properties.creatorExpected.send.bandwidth_bps, -1);
    EXPECT_EQ(properties.creatorExpected.send.latency_ms, -1);
    EXPECT_EQ(properties.creatorExpected.send.loss, -1.);
    EXPECT_EQ(properties.creatorExpected.receive.bandwidth_bps, -1);
    EXPECT_EQ(properties.creatorExpected.receive.latency_ms, -1);
    EXPECT_EQ(properties.creatorExpected.receive.loss, -1);

    EXPECT_EQ(properties.loaderExpected.send.bandwidth_bps, -1);
    EXPECT_EQ(properties.loaderExpected.send.latency_ms, -1);
    EXPECT_EQ(properties.loaderExpected.send.loss, -1.);
    EXPECT_EQ(properties.loaderExpected.receive.bandwidth_bps, -1);
    EXPECT_EQ(properties.loaderExpected.receive.latency_ms, -1);
    EXPECT_EQ(properties.loaderExpected.receive.loss, -1);

    EXPECT_EQ(properties.maxLinks, -1);
    EXPECT_EQ(properties.creatorsPerLoader, -1);
    EXPECT_EQ(properties.loadersPerCreator, -1);
    EXPECT_EQ(properties.roles, std::vector<ChannelRole>());
    EXPECT_EQ(properties.currentRole.roleName, std::string());
    EXPECT_EQ(properties.currentRole.mechanicalTags, std::vector<std::string>());
    EXPECT_EQ(properties.currentRole.behavioralTags, std::vector<std::string>());
    EXPECT_EQ(properties.currentRole.linkSide, LS_UNDEF);

    EXPECT_EQ(properties.maxSendsPerInterval, -1);
    EXPECT_EQ(properties.secondsPerInterval, -1);
    EXPECT_EQ(properties.intervalEndTime, 0);
    EXPECT_EQ(properties.sendsRemainingInInterval, -1);
}

#define TEST_COMPARISON(X, A, B) \
    a.X = b.X = A;               \
    EXPECT_TRUE(a == b);         \
    EXPECT_FALSE(a != b);        \
    EXPECT_EQ(a, b);             \
    b.X = B;                     \
    EXPECT_FALSE(a == b);        \
    EXPECT_TRUE(a != b);         \
    EXPECT_NE(a, b);             \
    b.X = A;

TEST(ChannelProperties, comparison) {
    validateChannelPropertiesSize();

    ChannelProperties a, b;

    // cppcheck-suppress redundantAssignment
    TEST_COMPARISON(linkDirection, LD_CREATOR_TO_LOADER, LD_LOADER_TO_CREATOR);
    // cppcheck-suppress redundantAssignment
    TEST_COMPARISON(linkDirection, LD_CREATOR_TO_LOADER, LD_BIDI);
    // cppcheck-suppress redundantAssignment
    TEST_COMPARISON(transmissionType, TT_UNICAST, TT_MULTICAST);
    // cppcheck-suppress redundantAssignment
    TEST_COMPARISON(connectionType, CT_DIRECT, CT_INDIRECT);
    // cppcheck-suppress redundantAssignment
    TEST_COMPARISON(connectionType, CT_DIRECT, CT_MIXED);
    TEST_COMPARISON(sendType, ST_STORED_ASYNC, ST_EPHEM_SYNC);

    TEST_COMPARISON(multiAddressable, false, true);
    TEST_COMPARISON(reliable, false, true);
    TEST_COMPARISON(duration_s, 0, 1);
    TEST_COMPARISON(period_s, 0, 1);
    TEST_COMPARISON(mtu, 0, 1);
    std::vector<std::string> hints1{"batch"};
    std::vector<std::string> hints2{"polling_interval"};
    TEST_COMPARISON(supported_hints, hints1, hints2);
    TEST_COMPARISON(channelGid, "channel1", "channel2");

    TEST_COMPARISON(creatorExpected.send.bandwidth_bps, 0, 1);
    TEST_COMPARISON(creatorExpected.send.latency_ms, 0, 1);
    TEST_COMPARISON(creatorExpected.send.loss, 0, 1);
    TEST_COMPARISON(creatorExpected.receive.bandwidth_bps, 0, 1);
    TEST_COMPARISON(creatorExpected.receive.latency_ms, 0, 1);
    TEST_COMPARISON(creatorExpected.receive.loss, 0, 1);

    TEST_COMPARISON(loaderExpected.send.bandwidth_bps, 0, 1);
    TEST_COMPARISON(loaderExpected.send.latency_ms, 0, 1);
    TEST_COMPARISON(loaderExpected.send.loss, 0, 1);
    TEST_COMPARISON(loaderExpected.receive.bandwidth_bps, 0, 1);
    TEST_COMPARISON(loaderExpected.receive.latency_ms, 0, 1);
    TEST_COMPARISON(loaderExpected.receive.loss, 0, 1);

    TEST_COMPARISON(maxLinks, 0, 1);
    TEST_COMPARISON(creatorsPerLoader, 0, 1);
    TEST_COMPARISON(loadersPerCreator, 0, 1);

    ChannelRole role;
    role.roleName = "role-name";
    role.mechanicalTags = {"tag1", "tag2", "tag3"};
    role.behavioralTags = {"tag4", "tag5"};
    role.linkSide = LS_CREATOR;
    TEST_COMPARISON(currentRole.roleName, "role1", "role2");
    TEST_COMPARISON(currentRole.mechanicalTags, {"tag1"}, {"tag2"});
    TEST_COMPARISON(currentRole.behavioralTags, {"tag1"}, {"tag2"});
    TEST_COMPARISON(currentRole.linkSide, LS_CREATOR, LS_BOTH);
    TEST_COMPARISON(roles, std::vector<ChannelRole>(), std::vector<ChannelRole>{role});

    TEST_COMPARISON(maxSendsPerInterval, 40, 20);
    TEST_COMPARISON(secondsPerInterval, 3600, 86400);
    TEST_COMPARISON(intervalEndTime, 12345678, 8675309);
    TEST_COMPARISON(sendsRemainingInInterval, 20, 17);
}

TEST(ChannelProperties, linkDirectionToString) {
    EXPECT_EQ(linkDirectionToString(LD_UNDEF), "LD_UNDEF");
    EXPECT_EQ(linkDirectionToString(LD_CREATOR_TO_LOADER), "LD_CREATOR_TO_LOADER");
    EXPECT_EQ(linkDirectionToString(LD_LOADER_TO_CREATOR), "LD_LOADER_TO_CREATOR");
    EXPECT_EQ(linkDirectionToString(LD_BIDI), "LD_BIDI");
    EXPECT_EQ(linkDirectionToString(static_cast<LinkDirection>(99)),
              "ERROR: INVALID LINK DIRECTION: 99");
}

class ChannelStaticPropertiesEqualTest : public ::testing::Test {
protected:
    ChannelProperties a;
    ChannelProperties b;
};

TEST_F(ChannelStaticPropertiesEqualTest, channelStaticPropertiesEqualMatchingProperties) {
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), true);
}

TEST_F(ChannelStaticPropertiesEqualTest, channelStaticPropertiesEqualNonmatchingProperties) {
    // channelGid
    b.channelGid = "testDifferent";
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.channelGid = a.channelGid;
    // linkDirection
    b.linkDirection = LD_BIDI;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.linkDirection = a.linkDirection;
    // transmissionType
    b.transmissionType = TT_UNICAST;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.transmissionType = a.transmissionType;
    // connectionType
    b.connectionType = CT_DIRECT;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.connectionType = a.connectionType;
    // sendType
    b.sendType = ST_STORED_ASYNC;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.sendType = a.sendType;
    // multiAddressable
    b.multiAddressable = !a.multiAddressable;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.multiAddressable = a.multiAddressable;
    // reliable
    b.reliable = !a.reliable;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.reliable = a.reliable;
    // bootstrap
    b.bootstrap = !a.bootstrap;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.bootstrap = a.bootstrap;
    // isFlushable
    b.isFlushable = !a.isFlushable;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.isFlushable = a.isFlushable;
    // duration_s
    b.duration_s = (a.duration_s + 1);
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.duration_s = a.duration_s;
    // period_s
    b.period_s = (a.period_s + 1);
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.period_s = a.period_s;
    // supported_hints
    b.supported_hints.push_back("test_string");
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.supported_hints = a.supported_hints;
    // mtu
    b.mtu = (a.mtu + 1);
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.mtu = a.mtu;
    // creatorExpected
    b.creatorExpected.send.bandwidth_bps = (a.creatorExpected.send.bandwidth_bps + 1);
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.creatorExpected.send.bandwidth_bps = a.creatorExpected.send.bandwidth_bps;
    // loaderExpected
    b.loaderExpected.send.bandwidth_bps = (a.loaderExpected.send.bandwidth_bps + 1);
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.loaderExpected.send.bandwidth_bps = a.loaderExpected.send.bandwidth_bps;

    // maxLinks
    b.maxLinks = a.maxLinks + 1;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.maxLinks = a.maxLinks;
    // creatorsPerLoader
    b.creatorsPerLoader = a.creatorsPerLoader + 1;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.creatorsPerLoader = a.creatorsPerLoader;
    // loadersPerCreator
    b.loadersPerCreator = a.loadersPerCreator + 1;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.loadersPerCreator = a.loadersPerCreator;

    // roles
    b.roles.push_back(ChannelRole());
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), false);
    b.roles = a.roles;

    // check all properites are reset
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), true);
}

TEST_F(ChannelStaticPropertiesEqualTest,
       channelStaticPropertiesEqualMatchingStaticNonmatchingDynamic) {
    b.channelStatus = CHANNEL_UNSUPPORTED;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), true);
    b.currentRole.linkSide = LS_BOTH;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), true);
    b.maxSendsPerInterval = 42;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), true);
    b.secondsPerInterval = 86400;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), true);
    b.intervalEndTime = 314159;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), true);
    b.sendsRemainingInInterval = 7;
    EXPECT_EQ(channelStaticPropertiesEqual(a, b), true);
}