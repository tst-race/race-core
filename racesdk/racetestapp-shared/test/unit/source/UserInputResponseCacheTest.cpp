
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

#include "../../../include/racetestapp/UserInputResponseCache.h"
#include "../../common/MockRaceSdkApp.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST_F(RaceTestAppSharedTestFixture, should_throw_when_no_cache_exists) {
    EXPECT_CALL(mockSdk, readFile("user-input-response-cache.json"))
        .WillOnce(::testing::Return(std::vector<std::uint8_t>()));

    UserInputResponseCache cache(mockSdk);
    cache.readCache();
    ASSERT_THROW(cache.getResponse("PluginTwoSix", "prompt"), std::out_of_range);
}

TEST_F(RaceTestAppSharedTestFixture, should_throw_when_invalid_cache_content) {
    std::string content = "{ key: not-valid, }";

    EXPECT_CALL(mockSdk, readFile("user-input-response-cache.json"))
        .WillOnce(::testing::Return(std::vector<std::uint8_t>(content.begin(), content.end())));

    UserInputResponseCache cache(mockSdk);
    cache.readCache();
    ASSERT_THROW(cache.getResponse("PluginTwoSix", "prompt"), std::out_of_range);
}

TEST_F(RaceTestAppSharedTestFixture, should_throw_when_no_cached_response) {
    std::string content = R"({ "PluginTwoSix.key": "response-value" })";

    EXPECT_CALL(mockSdk, readFile("user-input-response-cache.json"))
        .WillOnce(::testing::Return(std::vector<std::uint8_t>(content.begin(), content.end())));

    UserInputResponseCache cache(mockSdk);
    cache.readCache();
    ASSERT_THROW(cache.getResponse("PluginTwoSix", "prompt"), std::out_of_range);
}

TEST_F(RaceTestAppSharedTestFixture, should_return_cached_response) {
    std::string content = R"({ "PluginTwoSix.prompt": "expected-response" })";

    EXPECT_CALL(mockSdk, readFile("user-input-response-cache.json"))
        .WillOnce(::testing::Return(std::vector<std::uint8_t>(content.begin(), content.end())));

    UserInputResponseCache cache(mockSdk);
    cache.readCache();
    ASSERT_EQ("expected-response", cache.getResponse("PluginTwoSix", "prompt"));
}

TEST_F(RaceTestAppSharedTestFixture, should_save_response_to_cache) {
    std::string initialContent = R"({})";
    std::string expectedContent = R"({"PluginTwoSix.prompt":"cached-response"})";

    EXPECT_CALL(mockSdk, readFile("user-input-response-cache.json"))
        .WillOnce(::testing::Return(
            std::vector<std::uint8_t>(initialContent.begin(), initialContent.end())));
    EXPECT_CALL(mockSdk, writeFile("user-input-response-cache.json",
                                   std::vector<std::uint8_t>(expectedContent.begin(),
                                                             expectedContent.end())))
        .WillOnce(::testing::Return(SdkResponse(SDK_OK)));

    UserInputResponseCache cache(mockSdk);
    cache.readCache();
    ASSERT_THROW(cache.getResponse("PluginTwoSix", "prompt"), std::out_of_range);
    ASSERT_TRUE(cache.cacheResponse("PluginTwoSix", "prompt", "cached-response"));
    ASSERT_EQ("cached-response", cache.getResponse("PluginTwoSix", "prompt"));
}

TEST_F(RaceTestAppSharedTestFixture, should_return_false_when_unable_to_write_cache) {
    EXPECT_CALL(mockSdk, readFile("user-input-response-cache.json"))
        .WillOnce(::testing::Return(std::vector<std::uint8_t>()));
    EXPECT_CALL(mockSdk, writeFile("user-input-response-cache.json", ::testing::_))
        .WillOnce(::testing::Return(SdkResponse(SDK_INVALID_ARGUMENT)));

    UserInputResponseCache cache(mockSdk);
    cache.readCache();
    ASSERT_THROW(cache.getResponse("PluginTwoSix", "prompt"), std::out_of_range);
    ASSERT_FALSE(cache.cacheResponse("PluginTwoSix", "prompt", "cached-response"));
    ASSERT_EQ("cached-response", cache.getResponse("PluginTwoSix", "prompt"));
}

TEST_F(RaceTestAppSharedTestFixture, should_clear_cache) {
    std::string initialContent = R"({"PluginTwoSix.prompt":"cached-response"})";
    std::string expectedContent = R"({})";

    EXPECT_CALL(mockSdk, readFile("user-input-response-cache.json"))
        .WillOnce(::testing::Return(
            std::vector<std::uint8_t>(initialContent.begin(), initialContent.end())));
    EXPECT_CALL(mockSdk, writeFile("user-input-response-cache.json",
                                   std::vector<std::uint8_t>(expectedContent.begin(),
                                                             expectedContent.end())))
        .WillOnce(::testing::Return(SdkResponse(SDK_OK)));

    UserInputResponseCache cache(mockSdk);
    cache.readCache();
    ASSERT_EQ("cached-response", cache.getResponse("PluginTwoSix", "prompt"));
    ASSERT_TRUE(cache.clearCache());
    ASSERT_THROW(cache.getResponse("PluginTwoSix", "prompt"), std::out_of_range);
}
