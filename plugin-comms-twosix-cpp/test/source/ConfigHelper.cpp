
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

#include <stdexcept>

#include "../../source/config/helper.h"
#include "gtest/gtest.h"

TEST(ConfigHelper, linkTypeStringToEnum_valid_values) {
    EXPECT_EQ(confighelper::linkTypeStringToEnum("send"), LT_SEND);
    EXPECT_EQ(confighelper::linkTypeStringToEnum("receive"), LT_RECV);
    EXPECT_EQ(confighelper::linkTypeStringToEnum("bidirectional"), LT_BIDI);
}

TEST(ConfigHelper, linkTypeStringToEnum_invalid_values) {
    EXPECT_THROW(confighelper::linkTypeStringToEnum(""), std::invalid_argument);
    EXPECT_THROW(confighelper::linkTypeStringToEnum("undef"), std::invalid_argument);
    EXPECT_THROW(confighelper::linkTypeStringToEnum("multicast"), std::invalid_argument);
}

TEST(ConfigHelper, parseLink_valid_link) {
    nlohmann::json config;
    config["utilizedBy"] = {"1"};
    config["connectedTo"] = {"2", "3"};
    config["properties"] = {{"type", "send"}};
    config["profile"] = "test";

    LinkConfig result;
    ASSERT_NO_THROW(result = confighelper::parseLink(config, "1"));

    EXPECT_EQ(result.linkProfile, "test");
    EXPECT_EQ(result.personas, (std::vector<std::string>{"2", "3"}));
    EXPECT_EQ(result.linkProps.linkType, LT_SEND);
    EXPECT_EQ(result.linkProps.transmissionType, TT_UNICAST);
}

TEST(ConfigHelper, parseLink_invalid_properties) {
    nlohmann::json config;
    config["utilizedBy"] = {"1", "2"};
    config["connectedTo"] = {"1", "2"};
    config["profile"] = "test";

    ASSERT_THROW(confighelper::parseLink(config, "1"), std::invalid_argument);
}

TEST(ConfigHelper, parseLink_invalid_profile) {
    nlohmann::json config;
    config["utilizedBy"] = {"1", "2"};
    config["connectedTo"] = {"1", "2"};
    config["properties"] = {{"type", "send"}};

    ASSERT_THROW(confighelper::parseLink(config, "1"), std::invalid_argument);
}

TEST(ConfigHelper, parseLink_not_for_me) {
    nlohmann::json config;
    config["utilizedBy"] = {"2"};
    config["connectedTo"] = {"1", "3"};
    config["properties"] = {{"type", "send"}};
    config["profile"] = "test";

    ASSERT_THROW(confighelper::parseLink(config, "1"), std::invalid_argument);
}
