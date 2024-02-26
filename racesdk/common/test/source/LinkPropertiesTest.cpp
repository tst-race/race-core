
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

#include "LinkProperties.h"
#include "gtest/gtest.h"

/**
 * @brief Validate that the size of LinkProperties has not changed. If it has then these tests will
 * very likely need to be updated.
 *
 */
inline void validateLinkPropertiesSize() {
    const size_t expectedSize = 192;
    EXPECT_EQ(sizeof(LinkProperties), expectedSize)
        << "If this fails, then this test likely needs updating because new fields have been added "
           "to LinkProperties. If this is the case please add explicit tests for these fields and "
           "then update `expectedSize` to the value of the current size.";
}

TEST(LinkProperties, constructor) {
    validateLinkPropertiesSize();

    LinkProperties properties;

    EXPECT_EQ(properties.linkType, LT_UNDEF);
    EXPECT_EQ(properties.transmissionType, TT_UNDEF);
    EXPECT_EQ(properties.connectionType, CT_UNDEF);
    EXPECT_EQ(properties.sendType, ST_UNDEF);
    EXPECT_EQ(properties.reliable, false);
    EXPECT_EQ(properties.duration_s, -1);
    EXPECT_EQ(properties.period_s, -1);
    EXPECT_EQ(properties.mtu, -1);
    EXPECT_EQ(properties.supported_hints, std::vector<std::string>());
    EXPECT_EQ(properties.channelGid, std::string());
    EXPECT_EQ(properties.linkAddress, std::string());

    EXPECT_EQ(properties.worst.send.bandwidth_bps, -1);
    EXPECT_EQ(properties.worst.send.latency_ms, -1);
    EXPECT_EQ(properties.worst.send.loss, -1.);
    EXPECT_EQ(properties.worst.receive.bandwidth_bps, -1);
    EXPECT_EQ(properties.worst.receive.latency_ms, -1);
    EXPECT_EQ(properties.worst.receive.loss, -1);

    EXPECT_EQ(properties.best.send.bandwidth_bps, -1);
    EXPECT_EQ(properties.best.send.latency_ms, -1);
    EXPECT_EQ(properties.best.send.loss, -1.);
    EXPECT_EQ(properties.best.receive.bandwidth_bps, -1);
    EXPECT_EQ(properties.best.receive.latency_ms, -1);
    EXPECT_EQ(properties.best.receive.loss, -1);

    EXPECT_EQ(properties.expected.send.bandwidth_bps, -1);
    EXPECT_EQ(properties.expected.send.latency_ms, -1);
    EXPECT_EQ(properties.expected.send.loss, -1.);
    EXPECT_EQ(properties.expected.receive.bandwidth_bps, -1);
    EXPECT_EQ(properties.expected.receive.latency_ms, -1);
    EXPECT_EQ(properties.expected.receive.loss, -1);
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

TEST(LinkProperties, comparison) {
    validateLinkPropertiesSize();

    LinkProperties a, b;

    TEST_COMPARISON(linkType, LT_SEND, LT_RECV);
    TEST_COMPARISON(transmissionType, TT_UNICAST, TT_MULTICAST);
    TEST_COMPARISON(connectionType, CT_DIRECT, CT_INDIRECT);
    TEST_COMPARISON(sendType, ST_STORED_ASYNC, ST_EPHEM_SYNC);

    TEST_COMPARISON(reliable, false, true);
    TEST_COMPARISON(duration_s, 0, 1);
    TEST_COMPARISON(period_s, 0, 1);
    TEST_COMPARISON(mtu, 0, 1);
    std::vector<std::string> hints1{"batch"};
    std::vector<std::string> hints2{"polling_interval"};
    TEST_COMPARISON(supported_hints, hints1, hints2);
    std::string channel1 = "channel1";
    std::string channel2 = "channel2";
    TEST_COMPARISON(channelGid, channel1, channel2);
    std::string addr1 = "address1";
    std::string addr2 = "address2";
    TEST_COMPARISON(linkAddress, addr1, addr2);

    TEST_COMPARISON(worst.send.bandwidth_bps, 0, 1);
    TEST_COMPARISON(worst.send.latency_ms, 0, 1);
    TEST_COMPARISON(worst.send.loss, 0, 1);
    TEST_COMPARISON(worst.receive.bandwidth_bps, 0, 1);
    TEST_COMPARISON(worst.receive.latency_ms, 0, 1);
    TEST_COMPARISON(worst.receive.loss, 0, 1);

    TEST_COMPARISON(best.send.bandwidth_bps, 0, 1);
    TEST_COMPARISON(best.send.latency_ms, 0, 1);
    TEST_COMPARISON(best.send.loss, 0, 1);
    TEST_COMPARISON(best.receive.bandwidth_bps, 0, 1);
    TEST_COMPARISON(best.receive.latency_ms, 0, 1);
    TEST_COMPARISON(best.receive.loss, 0, 1);

    TEST_COMPARISON(expected.send.bandwidth_bps, 0, 1);
    TEST_COMPARISON(expected.send.latency_ms, 0, 1);
    TEST_COMPARISON(expected.send.loss, 0, 1);
    TEST_COMPARISON(expected.receive.bandwidth_bps, 0, 1);
    TEST_COMPARISON(expected.receive.latency_ms, 0, 1);
    TEST_COMPARISON(expected.receive.loss, 0, 1);
}
