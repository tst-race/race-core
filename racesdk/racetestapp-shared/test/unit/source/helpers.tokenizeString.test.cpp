
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

#include "gtest/gtest.h"
#include "racetestapp/raceTestAppHelpers.h"

TEST(tokenizeString, returns_empty_vector_for_empty_string) {
    {
        std::vector<std::string> result = rtah::tokenizeString("");
        ASSERT_EQ(result.size(), 0);
    }
    {
        std::vector<std::string> result = rtah::tokenizeString("", "~@~");
        ASSERT_EQ(result.size(), 0);
    }
}

TEST(tokenizeString, returns_single_token_if_delimiter_not_found) {
    {
        const std::vector<std::string> result =
            rtah::tokenizeString("some long string that does not have the delimiter", "*");

        ASSERT_EQ(result.size(), 1);
        EXPECT_EQ(result[0], "some long string that does not have the delimiter");
    }
    {
        const std::vector<std::string> result =
            rtah::tokenizeString("some long string that does not have the delimiter", "~@~");

        ASSERT_EQ(result.size(), 1);
        EXPECT_EQ(result[0], "some long string that does not have the delimiter");
    }
}

TEST(tokenizeString, returns_empty_string_token_if_delimiter_at_limit) {
    {
        std::vector<std::string> result = rtah::tokenizeString("my crazy token:", ":");

        ASSERT_EQ(result.size(), 2);
        EXPECT_EQ(result[0], "my crazy token");
        EXPECT_EQ(result[1], "");
    }
    {
        std::vector<std::string> result = rtah::tokenizeString("my crazy token~@~", "~@~");

        ASSERT_EQ(result.size(), 2);
        EXPECT_EQ(result[0], "my crazy token");
        EXPECT_EQ(result[1], "");
    }
    {
        std::vector<std::string> result = rtah::tokenizeString("&my crazy token", "&");

        ASSERT_EQ(result.size(), 2);
        EXPECT_EQ(result[0], "");
        EXPECT_EQ(result[1], "my crazy token");
    }
    {
        std::vector<std::string> result = rtah::tokenizeString("~@~my crazy token", "~@~");

        ASSERT_EQ(result.size(), 2);
        EXPECT_EQ(result[0], "");
        EXPECT_EQ(result[1], "my crazy token");
    }
}

TEST(tokenizeString, returns_tokens) {
    const std::string stringToTokenize = "some string to tokenize";
    std::vector<std::string> result = rtah::tokenizeString(stringToTokenize);

    ASSERT_EQ(result.size(), 4);
    EXPECT_EQ(result[0], "some");
    EXPECT_EQ(result[1], "string");
    EXPECT_EQ(result[2], "to");
    EXPECT_EQ(result[3], "tokenize");
}

TEST(tokenizeString, function_takes_in_an_optional_delimiter) {
    const std::string stringToTokenize = "some=string=to=tokenize";
    std::vector<std::string> result = rtah::tokenizeString(stringToTokenize, "=");

    ASSERT_EQ(result.size(), 4);
    EXPECT_EQ(result[0], "some");
    EXPECT_EQ(result[1], "string");
    EXPECT_EQ(result[2], "to");
    EXPECT_EQ(result[3], "tokenize");
}

TEST(tokenizeString, handles_multi_character_delimter) {
    {
        const std::string stringToTokenize = "some::::string::::to::::tokenize";
        std::vector<std::string> result = rtah::tokenizeString(stringToTokenize, "::::");

        ASSERT_EQ(result.size(), 4);
        EXPECT_EQ(result[0], "some");
        EXPECT_EQ(result[1], "string");
        EXPECT_EQ(result[2], "to");
        EXPECT_EQ(result[3], "tokenize");
    }
    {
        const std::string stringToTokenize = "some~@~string~@~to~@~tokenize";
        std::vector<std::string> result = rtah::tokenizeString(stringToTokenize, "~@~");

        ASSERT_EQ(result.size(), 4);
        EXPECT_EQ(result[0], "some");
        EXPECT_EQ(result[1], "string");
        EXPECT_EQ(result[2], "to");
        EXPECT_EQ(result[3], "tokenize");
    }
}
