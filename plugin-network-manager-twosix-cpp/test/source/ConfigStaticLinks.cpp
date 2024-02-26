
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

#include "ConfigStaticLinks.h"

#include "filesystem.h"
#include "gtest/gtest.h"
#include "race/mocks/MockRaceSdkNM.h"

using ::testing::Return;

TEST(ConfigStaticLinks, load_file_missing) {
    std::string linkProfilesStr = R"()";
    std::vector<uint8_t> fileBytes{linkProfilesStr.begin(), linkProfilesStr.end()};

    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, readFile("link-profiles.json")).WillOnce(Return(fileBytes));
    auto links = ConfigStaticLinks::loadLinkProfiles(sdk, "link-profiles.json");
    EXPECT_TRUE(links.empty());
}

TEST(ConfigStaticLinks, load_bad_json) {
    std::string linkProfilesStr = R"({
    twoSixDirectCpp: {
        {
            "description": "",
            "personas": [],
            "address": "",
            "role": "loader",
        }
    }
})";
    std::vector<uint8_t> fileBytes{linkProfilesStr.begin(), linkProfilesStr.end()};

    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, readFile("link-profiles.json")).WillOnce(Return(fileBytes));
    auto links = ConfigStaticLinks::loadLinkProfiles(sdk, "link-profiles.json");
    EXPECT_TRUE(links.empty());
}

TEST(ConfigStaticLinks, load_wrong_schema) {
    std::string linkProfilesStr = R"({
    "twoSixDirectCpp": {
        "race-client-00001": [
            {
                "description": "link description",
                "personas": ["race-server-00001"],
                "address": "{\"key\":\"value\"}",
                "role": "loader"
            }
        ]
    }
})";
    std::vector<uint8_t> fileBytes{linkProfilesStr.begin(), linkProfilesStr.end()};

    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, readFile("link-profiles.json")).WillOnce(Return(fileBytes));
    auto links = ConfigStaticLinks::loadLinkProfiles(sdk, "link-profiles.json");
    EXPECT_TRUE(links.empty());
}

TEST(ConfigStaticLinks, load_good_file) {
    std::string linkProfilesStr = R"({
    "twoSixDirectCpp": [
        {
            "description": "link description",
            "personas": ["race-server-00001"],
            "address": "{\"key\":\"value\"}",
            "role": "loader"
        }
    ]
})";
    std::vector<uint8_t> fileBytes{linkProfilesStr.begin(), linkProfilesStr.end()};

    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, readFile("link-profiles.json")).WillOnce(Return(fileBytes));
    auto links = ConfigStaticLinks::loadLinkProfiles(sdk, "link-profiles.json");
    EXPECT_EQ(1u, links.size());
    auto iter = links.find("twoSixDirectCpp");
    EXPECT_NE(iter, links.end());
    ASSERT_EQ(1u, iter->second.size());
    auto &linkProfile = *iter->second.begin();
    EXPECT_EQ("link description", linkProfile.description);
    EXPECT_EQ(std::vector<std::string>{"race-server-00001"}, linkProfile.personas);
    EXPECT_EQ("{\"key\":\"value\"}", linkProfile.address);
    EXPECT_EQ("loader", linkProfile.role);
}

TEST(ConfigStaticLinks, load_extra_keys) {
    std::string linkProfilesStr = R"({
    "twoSixDirectCpp": [
        {
            "description": "link description",
            "personas": ["race-server-00001"],
            "address": "{\"key\":\"value\"}",
            "role": "loader",
            "source": "genesis"
        }
    ]
})";
    std::vector<uint8_t> fileBytes{linkProfilesStr.begin(), linkProfilesStr.end()};

    MockRaceSdkNM sdk;
    EXPECT_CALL(sdk, readFile("link-profiles.json")).WillOnce(Return(fileBytes));
    auto links = ConfigStaticLinks::loadLinkProfiles(sdk, "link-profiles.json");
    EXPECT_EQ(1u, links.size());
    auto iter = links.find("twoSixDirectCpp");
    EXPECT_NE(iter, links.end());
    ASSERT_EQ(1u, iter->second.size());
    auto &linkProfile = *iter->second.begin();
    EXPECT_EQ("link description", linkProfile.description);
    EXPECT_EQ(std::vector<std::string>{"race-server-00001"}, linkProfile.personas);
    EXPECT_EQ("{\"key\":\"value\"}", linkProfile.address);
    EXPECT_EQ("loader", linkProfile.role);
}

TEST(ConfigLoaderPersonas, write) {
    std::string linkProfilesStr = R"({
    "channel1": [
        {
            "address": "address",
            "description": "description",
            "personas": [
                "persona1",
                "persona2"
            ],
            "role": "role"
        },
        {
            "address_list": [
                "address2.1",
                "address2.2"
            ],
            "description": "description2",
            "personas": [
                "persona1",
                "persona2"
            ],
            "role": "role2"
        }
    ],
    "channel2": [
        {
            "address": "address3",
            "description": "description3",
            "personas": [
                "persona3",
                "persona4"
            ],
            "role": "role3"
        }
    ]
})";

    std::vector<uint8_t> bytes{linkProfilesStr.begin(), linkProfilesStr.end()};

    MockRaceSdkNM sdk;
    ConfigStaticLinks::ChannelLinkProfilesMap linkProfiles = {
        {"channel1",
         {{"address", {}, "description", {"persona1", "persona2"}, "role"},
          {"", {"address2.1", "address2.2"}, "description2", {"persona1", "persona2"}, "role2"}}},
        {"channel2", {{"address3", {}, "description3", {"persona3", "persona4"}, "role3"}}}};

    EXPECT_CALL(sdk, writeFile("link-profiles.json", bytes)).WillOnce(Return(SDK_OK));
    EXPECT_EQ(ConfigStaticLinks::writeLinkProfiles(sdk, "link-profiles.json", linkProfiles), true);
}