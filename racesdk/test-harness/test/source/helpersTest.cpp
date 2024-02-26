
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

#include "../../source/helpers.h"
#include "gtest/gtest.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// helpers::tokenizeMessage
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(helpersTest, returns_empty_vector_for_empty_string) {
    std::vector<std::string> result = helpers::tokenizeMessage("");

    ASSERT_EQ(result.size(), 0);
}

TEST(helpersTest, returns_tokens) {
    const std::string stringToTokenize = "some string to tokenize";
    std::vector<std::string> result = helpers::tokenizeMessage(stringToTokenize);

    ASSERT_EQ(result.size(), 4);
    EXPECT_EQ(result[0], "some");
    EXPECT_EQ(result[1], "string");
    EXPECT_EQ(result[2], "to");
    EXPECT_EQ(result[3], "tokenize");
}

TEST(helpersTest, function_takes_in_an_optional_delimiter) {
    const std::string stringToTokenize = "some=string=to=tokenize";
    std::vector<std::string> result = helpers::tokenizeMessage(stringToTokenize, "=");

    ASSERT_EQ(result.size(), 4);
    EXPECT_EQ(result[0], "some");
    EXPECT_EQ(result[1], "string");
    EXPECT_EQ(result[2], "to");
    EXPECT_EQ(result[3], "tokenize");
}

TEST(helpersTest, returns_single_token_if_delimiter_not_found) {
    std::vector<std::string> result =
        helpers::tokenizeMessage("some long string that does not have the delimiter", "*");

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "some long string that does not have the delimiter");
}

TEST(helpersTest, returns_empty_string_token_if_delimiter_at_limit) {
    {
        std::vector<std::string> result = helpers::tokenizeMessage("my crazy token:", ":");

        ASSERT_EQ(result.size(), 2);
        EXPECT_EQ(result[0], "my crazy token");
        EXPECT_EQ(result[1], "");
    }
    {
        std::vector<std::string> result = helpers::tokenizeMessage("&my crazy token", "&");

        ASSERT_EQ(result.size(), 2);
        EXPECT_EQ(result[0], "");
        EXPECT_EQ(result[1], "my crazy token");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// helpers::getFirstLink
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(getFirstLink, returns_empty_string_for_empty_vector) {
    EXPECT_EQ(helpers::getFirstLink({}), "");
}

TEST(getFirstLink, returns_first_link) {
    {
        const std::vector<LinkID> linkIds{"LinkID_12", "LinkID_45", "LinkID_0", "LinkID_4"};
        const LinkID result = helpers::getFirstLink(linkIds);

        EXPECT_EQ(result, "LinkID_0");
    }
    {
        const std::vector<LinkID> linkIds{"LinkID_10", "LinkID_11", "LinkID_1", "LinkID_12"};
        const LinkID result = helpers::getFirstLink(linkIds);

        EXPECT_EQ(result, "LinkID_1");
    }

    {
        const std::vector<LinkID> linkIds{"11", "1"};
        const LinkID result = helpers::getFirstLink(linkIds);

        EXPECT_EQ(result, "1");
    }

    {
        const std::vector<LinkID> linkIds{"9", "8", "7", "6", "5", "4", "3", "2", "1", "0"};
        const LinkID result = helpers::getFirstLink(linkIds);

        EXPECT_EQ(result, "0");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// helpers::split
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(split, returns_single_fragment_for_empty_string) {
    EXPECT_EQ(std::vector<std::string>{""}, helpers::split("", "/"));
}

TEST(split, returns_single_fragment_for_empty_delimiter) {
    EXPECT_EQ(std::vector<std::string>{" original value"}, helpers::split(" original value", ""));
}

TEST(split, returns_single_fragment_when_no_delimiter_in_value) {
    EXPECT_EQ(std::vector<std::string>{" original value"}, helpers::split(" original value", "/"));
}

TEST(split, returns_all_fragments_by_delimiter) {
    EXPECT_EQ((std::vector<std::string>{"one", "two", "three"}),
              helpers::split("one/two/three", "/"));
    EXPECT_EQ((std::vector<std::string>{"one/", "wo/", "hree"}),
              helpers::split("one/two/three", "t"));
    EXPECT_EQ((std::vector<std::string>{"one/t", "/three"}), helpers::split("one/two/three", "wo"));
}
