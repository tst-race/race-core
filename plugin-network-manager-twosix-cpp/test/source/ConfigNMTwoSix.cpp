
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

#include "ConfigNMTwoSix.h"

#include "filesystem.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "race/mocks/MockRaceSdkNM.h"

using ::testing::Return;

std::vector<uint8_t> stringToBytes(const std::string &str) {
    return std::vector<uint8_t>{str.begin(), str.end()};
}

static bool operator==(const ExpectedMulticastLink &lhs, const ExpectedMulticastLink &rhs) {
    return lhs.personas == rhs.personas and lhs.channelGid == rhs.channelGid and
           lhs.linkSide == rhs.linkSide;
}

auto expectWrite(const std::string &expectedJson) {
    return [&expectedJson](auto & /*fileName*/, auto &actualBytes) {
        std::string actualJson(actualBytes.begin(), actualBytes.end());
        EXPECT_EQ(expectedJson, actualJson);
        return SDK_OK;
    };
}

TEST(ConfigNMTwoSixClient, load_defaults) {
    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, readFile("config.json")).WillOnce(Return(stringToBytes(R"({})")));

    ConfigNMTwoSixClient config;
    ASSERT_TRUE(loadClientConfig(sdk, config));

    EXPECT_EQ(0, config.channelRoles.size());
    EXPECT_EQ(0, config.entranceCommittee.size());
    EXPECT_EQ(0, config.exitCommittee.size());
    EXPECT_EQ(0, config.expectedLinks.size());
    EXPECT_EQ(0, config.expectedMulticastLinks.size());
    EXPECT_EQ(10000, config.maxSeenMessages);
    EXPECT_TRUE(config.useLinkWizard);
    EXPECT_EQ(0, config.bootstrapHandle);
    EXPECT_EQ("", config.bootstrapIntroducer);
    EXPECT_NEAR(60.0, config.lookbackSeconds, 0.01);
    EXPECT_EQ(0, config.otherConnections.size());
}

TEST(ConfigNMTwoSixClient, load_all_keys_defined) {
    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, readFile("config.json")).WillOnce(Return(stringToBytes(R"({
            "channelRoles": {
                "twoSixBootstrapCpp": "roleA",
                "twoSixIndirectCpp": "roleB"
            },
            "entranceCommittee": [
                "race-server-1"
            ],
            "exitCommittee": [
                "race-server-2"
            ],
            "expectedLinks": {
                "race-server-00001": {
                    "twoSixIndirectCpp": "LS_BOTH"
                },
                "race-server-00003": {
                    "twoSixIndirectCpp": "LS_BOTH"
                }
            },
            "expectedMulticastLinks": [
                {
                    "personas": ["race-server-1", "race-server-2"],
                    "channelGid": "twoSixIndirectCpp",
                    "linkSide": "LS_BOTH"
                }
            ],
            "otherConnections": [
                "race-server-1"
            ],
            "invalidEntry": [
                "invalid-value"
            ],
            "maxSeenMessages": 200,
            "useLinkWizard": false,
            "bootstrapHandle": 8675309,
            "bootstrapIntroducer": "race-client-1",
            "lookbackSeconds": 15
        })")));

    ConfigNMTwoSixClient config;
    ASSERT_TRUE(loadClientConfig(sdk, config));

    std::unordered_map<std::string, std::string> expectedChannelRoles = {
        {"twoSixBootstrapCpp", "roleA"}, {"twoSixIndirectCpp", "roleB"}};
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
        expectedExpectedLinks = {{"race-server-00001", {{"twoSixIndirectCpp", "LS_BOTH"}}},
                                 {"race-server-00003", {{"twoSixIndirectCpp", "LS_BOTH"}}}};
    std::vector<ExpectedMulticastLink> expectedMulticastLinks{
        {{"race-server-1", "race-server-2"}, "twoSixIndirectCpp", "LS_BOTH"}};
    EXPECT_EQ(config.channelRoles, expectedChannelRoles);
    EXPECT_EQ(config.expectedLinks, expectedExpectedLinks);
    EXPECT_EQ(config.expectedMulticastLinks, expectedMulticastLinks);
    EXPECT_THAT(config.entranceCommittee, ::testing::ElementsAre("race-server-1"));
    EXPECT_THAT(config.exitCommittee, ::testing::ElementsAre("race-server-2"));
    EXPECT_EQ(200, config.maxSeenMessages);
    EXPECT_FALSE(config.useLinkWizard);
    EXPECT_EQ(8675309, config.bootstrapHandle);
    EXPECT_EQ("race-client-1", config.bootstrapIntroducer);
    EXPECT_NEAR(15.0, config.lookbackSeconds, 0.01);
    EXPECT_THAT(config.otherConnections, ::testing::ElementsAre("race-server-1"));
}

TEST(ConfigNMTwoSixClient, load_file_doesnt_exist) {
    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, readFile("config.json")).WillOnce(Return(stringToBytes("")));

    ConfigNMTwoSixClient config;
    ASSERT_FALSE(loadClientConfig(sdk, config));
}

TEST(ConfigNMTwoSixClient, load_invalid_format) {
    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, readFile("config.json")).WillOnce(Return(stringToBytes(R"([
            {
                "key": "value"
            }
        ])")));

    ConfigNMTwoSixClient config;
    ASSERT_FALSE(loadClientConfig(sdk, config));
}

static const std::string expectedClientJson = R"({
    "bootstrapHandle": 8675309,
    "bootstrapIntroducer": "race-client-2",
    "channelRoles": {
        "twoSixBootstrapCpp": "roleA",
        "twoSixIndirectCpp": "roleB"
    },
    "entranceCommittee": [
        "race-server-1",
        "race-server-2"
    ],
    "exitCommittee": [],
    "expectedLinks": {
        "race-server-00001": {
            "twoSixIndirectCpp": "LS_BOTH"
        },
        "race-server-00003": {
            "twoSixIndirectCpp": "LS_BOTH"
        }
    },
    "expectedMulticastLinks": [
        {
            "channelGid": "twoSixIndirectCpp",
            "linkSide": "LS_BOTH",
            "personas": [
                "race-server-1",
                "race-server-2"
            ]
        }
    ],
    "lookbackSeconds": 60.0,
    "maxSeenMessages": 10000,
    "otherConnections": [],
    "useLinkWizard": true
})";

TEST(ConfigNMTwoSixClient, write) {
    ConfigNMTwoSixClient writeConfig;
    writeConfig.channelRoles = {{"twoSixBootstrapCpp", "roleA"}, {"twoSixIndirectCpp", "roleB"}};
    writeConfig.entranceCommittee = {"race-server-1", "race-server-2"};
    writeConfig.expectedLinks = {{"race-server-00001", {{"twoSixIndirectCpp", "LS_BOTH"}}},
                                 {"race-server-00003", {{"twoSixIndirectCpp", "LS_BOTH"}}}};
    writeConfig.expectedMulticastLinks = {
        {{"race-server-1", "race-server-2"}, "twoSixIndirectCpp", "LS_BOTH"}};
    writeConfig.bootstrapIntroducer = "race-client-2";
    writeConfig.bootstrapHandle = 8675309;

    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, writeFile("config.json", ::testing::_))
        .WillOnce(::testing::Invoke(expectWrite(expectedClientJson)));

    ASSERT_TRUE(writeClientConfig(sdk, writeConfig));
}

TEST(ConfigNMTwoSixServer, load_defaults) {
    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, readFile("config.json")).WillOnce(Return(stringToBytes(R"({})")));

    ConfigNMTwoSixServer config;
    ASSERT_TRUE(loadServerConfig(sdk, config));

    EXPECT_EQ(0, config.channelRoles.size());
    EXPECT_EQ(0, config.exitClients.size());
    EXPECT_EQ(0, config.expectedLinks.size());
    EXPECT_EQ(0, config.committeeClients.size());
    EXPECT_EQ("", config.committeeName);
    EXPECT_EQ(0, config.reachableCommittees.size());
    EXPECT_EQ(1000000, config.maxStaleUuids);
    EXPECT_EQ(1000000, config.maxFloodedUuids);
    EXPECT_EQ(2, config.floodingFactor);
    EXPECT_EQ(0, config.rings.size());
    EXPECT_TRUE(config.useLinkWizard);
    EXPECT_EQ(0, config.bootstrapHandle);
    EXPECT_EQ("", config.bootstrapIntroducer);
    EXPECT_NEAR(60.0, config.lookbackSeconds, 0.01);
    EXPECT_EQ(0, config.otherConnections.size());
}

TEST(ConfigNMTwoSixServer, load_all_keys_defined) {
    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, readFile("config.json")).WillOnce(Return(stringToBytes(R"({
            "exitClients": [
                "race-client-1"
            ],
            "channelRoles": {
                "twoSixBootstrapCpp": "roleA",
                "twoSixIndirectCpp": "roleB"
            },
            "committeeClients": [
                "race-client-2"
            ],
            "reachableCommittees": {
                "committee-1": [
                    "race-server-2"
                ]
            },
            "invalidEntry": [
                "invalid-value"
            ],
            "committeeName": "committee-0",
            "maxStaleUuids": 10000,
            "maxFloodedUuids": 15000,
            "floodingFactor": 5,
            "rings": [
                {
                    "length": 2,
                    "next": "race-server-2"
                }
            ],
            "useLinkWizard": false,
            "bootstrapHandle": 314159,
            "bootstrapIntroducer": "race-server-0",
            "lookbackSeconds": 30,
            "otherConnections": [
                "race-server-3"
            ],
            "expectedLinks": {
                "race-server-00001": {
                    "twoSixIndirectCpp": "LS_BOTH"
                },
                "race-server-00003": {
                    "twoSixIndirectCpp": "LS_BOTH"
                }
            }
        })")));

    ConfigNMTwoSixServer config;
    ASSERT_TRUE(loadServerConfig(sdk, config));

    std::unordered_map<std::string, std::string> expectedChannelRoles = {
        {"twoSixBootstrapCpp", "roleA"}, {"twoSixIndirectCpp", "roleB"}};
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
        expectedExpectedLinks = {{"race-server-00001", {{"twoSixIndirectCpp", "LS_BOTH"}}},
                                 {"race-server-00003", {{"twoSixIndirectCpp", "LS_BOTH"}}}};
    EXPECT_EQ(config.channelRoles, expectedChannelRoles);
    EXPECT_EQ(config.expectedLinks, expectedExpectedLinks);
    EXPECT_THAT(config.exitClients, ::testing::ElementsAre("race-client-1"));
    EXPECT_THAT(config.committeeClients, ::testing::ElementsAre("race-client-2"));
    EXPECT_EQ("committee-0", config.committeeName);
    EXPECT_THAT(config.reachableCommittees,
                ::testing::ElementsAre(
                    ::testing::Pair("committee-1", ::testing::ElementsAre("race-server-2"))));
    EXPECT_EQ(10000, config.maxStaleUuids);
    EXPECT_EQ(15000, config.maxFloodedUuids);
    EXPECT_EQ(5, config.floodingFactor);
    EXPECT_THAT(config.rings, ::testing::ElementsAre(::testing::AllOf(
                                  ::testing::Field(&RingEntry::length, 2),
                                  ::testing::Field(&RingEntry::next, "race-server-2"))));
    EXPECT_FALSE(config.useLinkWizard);
    EXPECT_EQ(314159, config.bootstrapHandle);
    EXPECT_EQ("race-server-0", config.bootstrapIntroducer);
    EXPECT_NEAR(30.0, config.lookbackSeconds, 0.01);
    EXPECT_THAT(config.otherConnections, ::testing::ElementsAre("race-server-3"));
}

TEST(ConfigNMTwoSixServer, load_file_doesnt_exist) {
    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, readFile("config.json")).WillOnce(Return(stringToBytes("")));

    ConfigNMTwoSixServer config;
    ASSERT_FALSE(loadServerConfig(sdk, config));
}

TEST(ConfigNMTwoSixServer, load_invalid_format) {
    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, readFile("config.json")).WillOnce(Return(stringToBytes(R"([
            {
                "key": "value"
            }
        ])")));

    ConfigNMTwoSixServer config;
    ASSERT_FALSE(loadServerConfig(sdk, config));
}

static const std::string expectedServerJson = R"({
    "channelRoles": {
        "twoSixBootstrapCpp": "roleA",
        "twoSixIndirectCpp": "roleB"
    },
    "committeeClients": [],
    "committeeName": "",
    "exitClients": [
        "race-client-2",
        "race-client-1"
    ],
    "expectedLinks": {
        "race-server-00001": {
            "twoSixIndirectCpp": "LS_BOTH"
        },
        "race-server-00003": {
            "twoSixIndirectCpp": "LS_BOTH"
        }
    },
    "floodingFactor": 2,
    "lookbackSeconds": 60.0,
    "maxFloodedUuids": 1000000,
    "maxStaleUuids": 1000000,
    "otherConnections": [],
    "reachableCommittees": {},
    "rings": [],
    "useLinkWizard": true
})";

TEST(ConfigNMTwoSixServer, write) {
    ConfigNMTwoSixServer writeConfig;
    writeConfig.channelRoles = {{"twoSixBootstrapCpp", "roleA"}, {"twoSixIndirectCpp", "roleB"}};
    writeConfig.expectedLinks = {{"race-server-00001", {{"twoSixIndirectCpp", "LS_BOTH"}}},
                                 {"race-server-00003", {{"twoSixIndirectCpp", "LS_BOTH"}}}};
    writeConfig.exitClients = {"race-client-1", "race-client-2"};

    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, writeFile("config.json", ::testing::_))
        .WillOnce(::testing::Invoke(expectWrite(expectedServerJson)));

    ASSERT_TRUE(writeServerConfig(sdk, writeConfig));
}
