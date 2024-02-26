
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

#include "PluginNMTwoSixClientCpp.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "race/mocks/MockRaceSdkNM.h"

using ::testing::Return;

const std::string linkProfilesStr = R"({
    "twoSixDirectCpp": [
        {
            "description": "link description",
            "personas": ["race-server-00001"],
            "address": "{\"key\":\"value\"}",
            "role": "loader"
        }
    ]
})";
const std::vector<uint8_t> linkProfilesBytes{linkProfilesStr.begin(), linkProfilesStr.end()};

const std::string configStr = R"({
    "entranceCommittee": [
        "race-server-00001"
    ],
    "exitCommittee": [
        "race-server-00002"
    ],
    "invalidEntry": [
        "invalid-value"
    ]
}
)";
const std::vector<uint8_t> configBytes{configStr.begin(), configStr.end()};

const std::string personasStr = R"([
    {
        "displayName": "RACE Client 1",
        "personaType": "client",
        "raceUuid": "race-client-00001",
        "aesKeyFile": "race-client-00001.aes"
    },
    {
        "displayName": "RACE Client 2",
        "personaType": "client",
        "raceUuid": "race-client-00002",
        "aesKeyFile": "race-client-00002.aes"
    },
    {
        "displayName": "RACE Server 1",
        "personaType": "server",
        "raceUuid": "race-server-00001",
        "aesKeyFile": "race-server-00001.aes"
    },
    {
        "displayName": "RACE Server 2",
        "personaType": "server",
        "raceUuid": "race-server-00002",
        "aesKeyFile": "race-server-00002.aes"
    }
]
)";
const std::vector<uint8_t> personasBytes{personasStr.begin(), personasStr.end()};

const std::vector<uint8_t> aes1Bytes{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
const std::vector<uint8_t> aes2Bytes{
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F};
const std::vector<uint8_t> aes3Bytes{
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F};
const std::vector<uint8_t> aes4Bytes{
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F};

class PluginNMTwoSixClientCppTestProtected : public PluginNMTwoSixClientCpp {
public:
    explicit PluginNMTwoSixClientCppTestProtected(IRaceSdkNM *sdk) : PluginNMTwoSixClientCpp(sdk) {}
    LinkID getPreferredLinkIdForSendingTest(const std::vector<LinkID> &potentialLinks) {
        return PluginNMTwoSixClientCpp::getPreferredLinkIdForSendingToPersona(potentialLinks,
                                                                              P_UNDEF);
    }

    std::string getJaegerConfigPath() override {
        return "";
    }
};

class PluginNMTwoSixClientCppTestFixture : public ::testing::Test {
public:
    PluginNMTwoSixClientCppTestFixture() : sdk(), plugin(&sdk) {
        ON_CALL(sdk, getActivePersona()).WillByDefault(Return("race-client-00001"));
        ON_CALL(sdk, readFile("link-profiles.json")).WillByDefault(Return(linkProfilesBytes));
        ON_CALL(sdk, readFile("config.json")).WillByDefault(Return(configBytes));
        ON_CALL(sdk, readFile("personas/race-personas.json")).WillByDefault(Return(personasBytes));
        ON_CALL(sdk, readFile("personas/race-client-00001.aes")).WillByDefault(Return(aes1Bytes));
        ON_CALL(sdk, readFile("personas/race-client-00002.aes")).WillByDefault(Return(aes2Bytes));
        ON_CALL(sdk, readFile("personas/race-server-00001.aes")).WillByDefault(Return(aes3Bytes));
        ON_CALL(sdk, readFile("personas/race-server-00002.aes")).WillByDefault(Return(aes4Bytes));
        ::testing::DefaultValue<SdkResponse>::Set(SDK_OK);
    }
    virtual ~PluginNMTwoSixClientCppTestFixture() {}
    virtual void SetUp() override {}
    virtual void TearDown() override {}

public:
    MockRaceSdkNM sdk;
    PluginNMTwoSixClientCppTestProtected plugin;
};

TEST_F(PluginNMTwoSixClientCppTestFixture, init) {
    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp/";

    plugin.init(pluginConfig);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// getPreferredLinkIdForSendingToPersona
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Set the expected connection type returned in LinkProperties when network manager plugin
 * calls getLinkProperties.
 *
 * @param sdk The mock sdk.
 * @param connectionType The expected return value for tranmission type.
 * @param linkId The ID of the link to return the connection type for.
 */
inline void returnConnectionTypeForLink(MockRaceSdkNM &sdk, const ConnectionType connectionType,
                                        const LinkID &linkId) {
    LinkProperties props;
    props.connectionType = connectionType;
    EXPECT_CALL(sdk, getLinkProperties(linkId)).WillRepeatedly(::testing::Return(props));
}

TEST_F(PluginNMTwoSixClientCppTestFixture,
       getPreferredLinkIdForSendingToPersona_will_return_empty_string_if_no_links_available) {
    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp/";

    ASSERT_NO_THROW(plugin.init(pluginConfig));

    const LinkID result = plugin.getPreferredLinkIdForSendingTest({});

    EXPECT_EQ(result, "");
}

TEST_F(PluginNMTwoSixClientCppTestFixture,
       getPreferredLinkIdForSendingToPersona_will_prefer_indirect_over_direct) {
    {
        returnConnectionTypeForLink(sdk, CT_DIRECT, "1");
        returnConnectionTypeForLink(sdk, CT_DIRECT, "2");
        returnConnectionTypeForLink(sdk, CT_DIRECT, "3");
        returnConnectionTypeForLink(sdk, CT_INDIRECT, "4");
        returnConnectionTypeForLink(sdk, CT_DIRECT, "5");
        returnConnectionTypeForLink(sdk, CT_DIRECT, "6");
    }

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp/";

    ASSERT_NO_THROW(plugin.init(pluginConfig));

    const std::vector<LinkID> potentialLinks = {"1", "2", "3", "4", "5", "6"};
    const LinkID result = plugin.getPreferredLinkIdForSendingTest(potentialLinks);

    EXPECT_EQ(result, "4");
}

TEST_F(PluginNMTwoSixClientCppTestFixture,
       getPreferredLinkIdForSendingToPersona_will_prefer_direct_over_undef) {
    {
        returnConnectionTypeForLink(sdk, CT_UNDEF, "1");
        returnConnectionTypeForLink(sdk, CT_UNDEF, "2");
        returnConnectionTypeForLink(sdk, CT_DIRECT, "3");
        returnConnectionTypeForLink(sdk, CT_UNDEF, "4");
        returnConnectionTypeForLink(sdk, CT_UNDEF, "5");
        returnConnectionTypeForLink(sdk, CT_UNDEF, "6");
    }

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp/";

    ASSERT_NO_THROW(plugin.init(pluginConfig));

    const std::vector<LinkID> potentialLinks = {"1", "2", "3", "4", "5", "6"};
    const LinkID result = plugin.getPreferredLinkIdForSendingTest(potentialLinks);

    EXPECT_EQ(result, "3");
}

TEST_F(PluginNMTwoSixClientCppTestFixture,
       getPreferredLinkIdForSendingToPersona_will_not_use_undef_if_only_type_available) {
    {
        returnConnectionTypeForLink(sdk, CT_UNDEF, "1");
        returnConnectionTypeForLink(sdk, CT_UNDEF, "2");
        returnConnectionTypeForLink(sdk, CT_UNDEF, "3");
        returnConnectionTypeForLink(sdk, CT_UNDEF, "4");
        returnConnectionTypeForLink(sdk, CT_UNDEF, "5");
        returnConnectionTypeForLink(sdk, CT_UNDEF, "6");
    }

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp/";

    ASSERT_NO_THROW(plugin.init(pluginConfig));

    const std::vector<LinkID> potentialLinks = {"1", "2", "3", "4", "5", "6"};
    const LinkID result = plugin.getPreferredLinkIdForSendingTest(potentialLinks);

    EXPECT_TRUE(std::find(potentialLinks.begin(), potentialLinks.end(), result) ==
                potentialLinks.end());
}

TEST_F(PluginNMTwoSixClientCppTestFixture, reopenReceiveConnection) {
    RaceHandle handle = 42;
    LinkID linkId = "LinkID-0";
    ConnectionID connId = "Conn-1";
    LinkType linkType = LT_RECV;
    std::vector<std::string> personas{"persona1"};
    LinkProperties linkProperties;
    linkProperties.linkType = linkType;
    linkProperties.connectionType = CT_INDIRECT;

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp/";

    ASSERT_NO_THROW(plugin.init(pluginConfig));
    EXPECT_CALL(sdk, getLinkProperties(linkId)).WillRepeatedly(::testing::Return(linkProperties));
    EXPECT_CALL(sdk, getLinksForPersonas(personas, linkType))
        .WillRepeatedly(::testing::Return(std::vector<LinkID>{linkId}));
    EXPECT_CALL(sdk, openConnection(linkType, linkId, "{}", 0, RACE_UNLIMITED, 0))
        .WillOnce(::testing::Return(SdkResponse{SDK_OK, 0.0, handle}));
    plugin.openRecvConns(personas);
    plugin.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, linkId, LinkProperties());
    EXPECT_CALL(sdk, openConnection(linkType, linkId, "{}", 0, RACE_UNLIMITED, 0));
    plugin.onConnectionStatusChanged(NULL_RACE_HANDLE, connId, CONNECTION_CLOSED, linkId,
                                     LinkProperties());
}

TEST_F(PluginNMTwoSixClientCppTestFixture, DISABLED_reopenSendConnection) {
    RaceHandle handle = 42;
    LinkID linkId = "LinkID-0";
    ConnectionID connId = "Conn-1";
    LinkType linkType = LT_RECV;
    std::vector<std::string> personas{"persona1"};
    LinkProperties linkProperties;
    linkProperties.linkType = linkType;
    linkProperties.connectionType = CT_INDIRECT;

    PluginConfig pluginConfig;
    pluginConfig.tmpDirectory = "/tmp/";

    ASSERT_NO_THROW(plugin.init(pluginConfig));
    EXPECT_CALL(sdk, getLinkProperties(linkId)).WillRepeatedly(::testing::Return(linkProperties));
    EXPECT_CALL(sdk, getLinksForPersonas(personas, linkType))
        .WillRepeatedly(::testing::Return(std::vector<LinkID>{linkId}));
    EXPECT_CALL(sdk, openConnection(linkType, linkId, "{}", 0, RACE_UNLIMITED, 0))
        .WillOnce(::testing::Return(SdkResponse{SDK_OK, 0.0, handle}));
    plugin.openRecvConns(personas);
    plugin.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, linkId, LinkProperties());
    EXPECT_CALL(sdk, openConnection(linkType, linkId, "{}", 0, RACE_UNLIMITED, 0));
    plugin.onConnectionStatusChanged(NULL_RACE_HANDLE, connId, CONNECTION_CLOSED, linkId,
                                     LinkProperties());
}
