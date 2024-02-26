
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

#include <sstream>

#include "../../../include/racetestapp/UserInputResponseParser.h"
#include "gtest/gtest.h"

class Parser : public UserInputResponseParser {
public:
    Parser() : UserInputResponseParser("file.json") {}

    // Avoids compiler error about a hidden overloaded virtual function
    using UserInputResponseParser::getResponse;

    // Expose in public scope for testing
    UserResponse getResponse(std::istream &input, const std::string &pluginId,
                             const std::string &prompt) {
        return UserInputResponseParser::getResponse(input, pluginId, prompt);
    }
};

TEST(UserInputResponseParser, bad_stream) {
    Parser parser;
    std::stringstream input;
    ASSERT_THROW(parser.getResponse(input, "PluginTwoSix", "prompt"), Parser::parsing_exception);
}

TEST(UserInputResponseParser, bad_json_content) {
    Parser parser;
    std::stringstream input;
    input << R"(
        {
            not: [
                valid,
                { json )
            ],
        }
    )";
    ASSERT_THROW(parser.getResponse(input, "PluginTwoSix", "prompt"), Parser::parsing_exception);
}

TEST(UserInputResponseParser, wrong_json_format) {
    Parser parser;
    std::stringstream input;
    input << R"(
        [
            "valid json, but wrong shape"
        ]
    )";
    ASSERT_THROW(parser.getResponse(input, "PluginTwoSix", "prompt"), Parser::parsing_exception);
}

TEST(UserInputResponseParser, missing_plugin_id) {
    Parser parser;
    std::stringstream input;
    input << R"(
        {
            "PluginId": {
                "prompt": "response"
            }
        }
    )";
    ASSERT_THROW(parser.getResponse(input, "PluginTwoSix", "prompt"), Parser::parsing_exception);
}

TEST(UserInputResponseParser, missing_prompt) {
    Parser parser;
    std::stringstream input;
    input << R"(
        {
            "PluginTwoSix": {
                "key": "response"
            }
        }
    )";
    ASSERT_THROW(parser.getResponse(input, "PluginTwoSix", "prompt"), Parser::parsing_exception);
}

TEST(UserInputResponseParser, wrong_response_format) {
    Parser parser;
    std::stringstream input;
    input << R"(
        {
            "PluginTwoSix": {
                "prompt": ["wrong", "format"]
            }
        }
    )";
    ASSERT_THROW(parser.getResponse(input, "PluginTwoSix", "prompt"), Parser::parsing_exception);
}

TEST(UserInputResponseParser, simple_string_format) {
    Parser parser;
    std::stringstream input;
    input << R"(
        {
            "PluginTwoSix": {
                "prompt": "expected-response"
            }
        }
    )";
    auto response = parser.getResponse(input, "PluginTwoSix", "prompt");
    EXPECT_TRUE(response.answered);
    EXPECT_EQ(0, response.delay_ms);
    EXPECT_EQ("expected-response", response.response);
}

TEST(UserInputResponseParser, wrong_object_answered_format) {
    Parser parser;
    std::stringstream input;
    input << R"(
        {
            "PluginTwoSix": {
                "prompt": {
                    "answered": "true"
                }
            }
        }
    )";
    ASSERT_THROW(parser.getResponse(input, "PluginTwoSix", "prompt"), Parser::parsing_exception);
}

TEST(UserInputResponseParser, wrong_object_delay_format) {
    Parser parser;
    std::stringstream input;
    input << R"(
        {
            "PluginTwoSix": {
                "prompt": {
                    "delayMs": "1234"
                }
            }
        }
    )";
    ASSERT_THROW(parser.getResponse(input, "PluginTwoSix", "prompt"), Parser::parsing_exception);
}

TEST(UserInputResponseParser, wrong_object_response_format) {
    Parser parser;
    std::stringstream input;
    input << R"(
        {
            "PluginTwoSix": {
                "prompt": {
                    "response": true
                }
            }
        }
    )";
    ASSERT_THROW(parser.getResponse(input, "PluginTwoSix", "prompt"), Parser::parsing_exception);
}

TEST(UserInputResponseParser, default_response_object) {
    Parser parser;
    std::stringstream input;
    input << R"(
        {
            "PluginTwoSix": {
                "prompt": {}
            }
        }
    )";
    auto response = parser.getResponse(input, "PluginTwoSix", "prompt");
    EXPECT_TRUE(response.answered);
    EXPECT_EQ(0, response.delay_ms);
    EXPECT_EQ("", response.response);
}

TEST(UserInputResponseParser, response_object) {
    Parser parser;
    std::stringstream input;
    input << R"(
        {
            "PluginTwoSix": {
                "prompt": {
                    "answered": false,
                    "delayMs": 1500,
                    "response": "expected-response"
                }
            }
        }
    )";
    auto response = parser.getResponse(input, "PluginTwoSix", "prompt");
    EXPECT_FALSE(response.answered);
    EXPECT_EQ(1500, response.delay_ms);
    EXPECT_EQ("expected-response", response.response);
}