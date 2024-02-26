
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
#include "../../../include/racetestapp/UserInputResponseParser.h"
#include "../../common/MockRaceSdkApp.h"
#include "../../common/MockRaceTestAppOutput.h"
#include "OpenTracingHelpers.h"
#include "gtest/gtest.h"
#include "racetestapp/RaceApp.h"

TEST_F(RaceTestAppSharedTestFixture, handleReceivedMessage_should_output_message) {
    MockRaceTestAppOutput output;
    {
        ::testing::InSequence s;

        EXPECT_CALL(output, writeOutput(::testing::HasSubstr("Received message: "))).Times(1);
    }

    RaceApp app(output, mockSdk, createTracer("", "test persona"));

    const ClrMsg message("some message", "from sender", "to recipient", 1234, 4321);
    app.handleReceivedMessage(message);
}

////////////////////////////////////////////////////////////////
// User input
////////////////////////////////////////////////////////////////

class MockUserInputResponseParser : public UserInputResponseParser {
public:
    MockUserInputResponseParser(const std::string &path) : UserInputResponseParser(path) {}
    MOCK_METHOD(UserResponse, getResponse,
                (const std::string &pluginId, const std::string &prompt));
};

[[maybe_unused]] static std::ostream &operator<<(
    std::ostream &os, const UserInputResponseParser::UserResponse &userResponse) {
    // userResponse.platform
    os << "{"
       << "UserResponse: " << userResponse.answered << ", " << userResponse.response << ", "
       << userResponse.delay_ms << "}" << std::endl;

    return os;
}

class MockUserInputResponseCache : public UserInputResponseCache {
public:
    MockUserInputResponseCache(IRaceSdkApp &_raceSdk) : UserInputResponseCache(_raceSdk) {}
    MOCK_METHOD(std::string, getResponse, (const std::string &pluginId, const std::string &prompt));
    MOCK_METHOD(bool, cacheResponse,
                (const std::string &pluginId, const std::string &prompt,
                 const std::string &reponse));
};

class TestableRaceApp : public RaceApp {
public:
    TestableRaceApp(IRaceTestAppOutput &_appOutput, IRaceSdkApp &_raceSdk,
                    std::shared_ptr<opentracing::Tracer> _tracer) :
        RaceApp(_appOutput, _raceSdk, _tracer) {
        const std::string path = "dummy/path";
        responseParser = std::make_unique<MockUserInputResponseParser>(path);
        responseCache = std::make_unique<MockUserInputResponseCache>(_raceSdk);
    }
    MockUserInputResponseParser *getUserInputParser() {
        return reinterpret_cast<MockUserInputResponseParser *>(responseParser.get());
    }

    MockUserInputResponseCache *getUserInputCache() {
        return reinterpret_cast<MockUserInputResponseCache *>(responseCache.get());
    }
};

TEST_F(RaceTestAppSharedTestFixture, requestUserInput_invalid_key) {
    // Set up app for testing
    MockRaceTestAppOutput output;
    TestableRaceApp app(output, mockSdk, createTracer("", "test persona"));

    RaceHandle handle = 11223344L;
    EXPECT_CALL(mockSdk, onUserInputReceived(handle, false, ""))
        .Times(1)
        .WillOnce(::testing::Return(SDK_OK));
    EXPECT_EQ(
        app.requestUserInput(handle, "plugin-id", "not-a-valid-user-input-key", "prompt", false)
            .status,
        SDK_OK);
}

TEST_F(RaceTestAppSharedTestFixture, requestUserInput_valid_key) {
    // Set up app for testing
    MockRaceTestAppOutput output;
    TestableRaceApp app(output, mockSdk, createTracer("", "test persona"));

    // Mock UserInputResponseParser to return a valid response given the expected key
    RaceHandle handle = 11223344L;
    MockUserInputResponseParser *mockResponseParser = app.getUserInputParser();
    MockUserInputResponseParser::UserResponse expectedResponse;
    expectedResponse.answered = true;
    expectedResponse.response = "valid-response";
    EXPECT_CALL(*mockResponseParser, getResponse("plugin-id", "valid-key"))
        .Times(1)
        .WillOnce(::testing::Return(expectedResponse));

    // Ensure that app calls the sdk with the response provided by the mock UserInputResponseParser
    EXPECT_CALL(mockSdk, onUserInputReceived(handle, true, "valid-response"))
        .Times(1)
        .WillOnce(::testing::Return(SDK_OK));

    app.requestUserInput(handle, "plugin-id", "valid-key", "prompt", false);
}

////////////////////////////////////////////////////////////////
// User input
////////////////////////////////////////////////////////////////

TEST_F(RaceTestAppSharedTestFixture, notify_sdk_unanswered_response_when_parser_error) {
    // Set up app for testing
    MockRaceTestAppOutput output;
    TestableRaceApp app(output, mockSdk, createTracer("", "test persona"));

    // Mock UserInputResponseParser to return an invalid response
    MockUserInputResponseParser *parser = app.getUserInputParser();

    EXPECT_CALL(*parser, getResponse("PluginTwoSix", "key"))
        .WillOnce(::testing::Throw(UserInputResponseParser::parsing_exception("error")));
    EXPECT_CALL(mockSdk, onUserInputReceived(0x11223344l, false, ""));

    app.requestUserInput(0x11223344l, "PluginTwoSix", "key", "prompt", false);
}

TEST_F(RaceTestAppSharedTestFixture, notify_sdk_answered_response) {
    // Set up app for testing
    MockRaceTestAppOutput output;
    TestableRaceApp app(output, mockSdk, createTracer("", "test persona"));

    // Mock UserInputResponseParser to return a valid response given the expected key
    MockUserInputResponseParser *parser = app.getUserInputParser();

    EXPECT_CALL(*parser, getResponse("PluginTwoSix", "key"))
        .WillOnce(
            ::testing::Return(UserInputResponseParser::UserResponse{true, "expected-response", 0}));
    EXPECT_CALL(mockSdk, onUserInputReceived(0x11223344l, true, "expected-response"));

    app.requestUserInput(0x11223344l, "PluginTwoSix", "key", "prompt", false);
}

TEST_F(RaceTestAppSharedTestFixture, notify_sdk_answered_response_after_delay) {
    // Set up app for testing
    MockRaceTestAppOutput output;
    TestableRaceApp app(output, mockSdk, createTracer("", "test persona"));

    // Mock UserInputResponseParser to return a valid response given the expected key
    MockUserInputResponseParser *parser = app.getUserInputParser();

    EXPECT_CALL(*parser, getResponse("PluginTwoSix", "key"))
        .WillOnce(::testing::Return(
            UserInputResponseParser::UserResponse{true, "expected-response", 200}));
    EXPECT_CALL(mockSdk, onUserInputReceived(0x11223344l, true, "expected-response"));

    app.requestUserInput(0x11223344l, "PluginTwoSix", "key", "prompt", false);
}

TEST_F(RaceTestAppSharedTestFixture, write_response_to_cache) {
    // Set up app for testing
    MockRaceTestAppOutput output;
    TestableRaceApp app(output, mockSdk, createTracer("", "test persona"));

    // Mock UserInputResponseParser to return a valid response given the expected key
    MockUserInputResponseParser *parser = app.getUserInputParser();
    MockUserInputResponseCache *cache = app.getUserInputCache();

    EXPECT_CALL(*cache, getResponse("PluginTwoSix", "key"))
        .WillOnce(::testing::Throw(std::out_of_range("no value")));
    EXPECT_CALL(*parser, getResponse("PluginTwoSix", "key"))
        .WillOnce(
            ::testing::Return(UserInputResponseParser::UserResponse{true, "expected-response", 0}));
    EXPECT_CALL(*cache, cacheResponse("PluginTwoSix", "key", "expected-response"));
    EXPECT_CALL(mockSdk, onUserInputReceived(0x11223344l, true, "expected-response"));

    app.requestUserInput(0x11223344l, "PluginTwoSix", "key", "prompt", true);
}

TEST_F(RaceTestAppSharedTestFixture, use_cached_response) {
    // Set up app for testing
    MockRaceTestAppOutput output;
    TestableRaceApp app(output, mockSdk, createTracer("", "test persona"));

    // Mock UserInputResponseParser to return a valid response given the expected key
    MockUserInputResponseParser *parser = app.getUserInputParser();
    MockUserInputResponseCache *cache = app.getUserInputCache();

    EXPECT_CALL(*cache, getResponse("PluginTwoSix", "key"))
        .WillOnce(::testing::Return("cached-response"));
    EXPECT_CALL(*parser, getResponse(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*cache, cacheResponse(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(mockSdk, onUserInputReceived(0x11223344l, true, "cached-response"));

    app.requestUserInput(0x11223344l, "PluginTwoSix", "key", "prompt", true);
}

TEST_F(RaceTestAppSharedTestFixture, no_cache_update_if_no_answer) {
    // Set up app for testing
    MockRaceTestAppOutput output;
    TestableRaceApp app(output, mockSdk, createTracer("", "test persona"));

    // Mock UserInputResponseParser to return a valid response given the expected key
    MockUserInputResponseParser *parser = app.getUserInputParser();
    MockUserInputResponseCache *cache = app.getUserInputCache();

    EXPECT_CALL(*cache, getResponse("PluginTwoSix", "key"))
        .WillOnce(::testing::Throw(std::out_of_range("no value")));
    EXPECT_CALL(*parser, getResponse("PluginTwoSix", "key"))
        .WillOnce(::testing::Throw(UserInputResponseParser::parsing_exception("error")));
    EXPECT_CALL(*cache, cacheResponse(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(mockSdk, onUserInputReceived(0x11223344l, false, ""));

    app.requestUserInput(0x11223344l, "PluginTwoSix", "key", "prompt", true);
}