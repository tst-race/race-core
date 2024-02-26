
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
#include "racetestapp/Message.h"

class CreateMessageInvalidInputParamTest : public testing::TestWithParam<std::string> {};

TEST_P(CreateMessageInvalidInputParamTest, createMessage_should_throw_for_invalid_input) {
    EXPECT_THROW(rta::Message::createMessage(GetParam()), std::invalid_argument);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    invalid_input,
    CreateMessageInvalidInputParamTest,
    ::testing::Values(
        "my invalid input"
    )
);
// clang-format on

class MessageTestResult {
public:
    MessageTestResult(const std::string &_message, const std::string &_recipient, bool isNMBypass,
                      const std::string &networkManagerBypassRoute) :
        messageSize(0),
        message(_message),
        recipient(_recipient),
        period(0),
        count(1),
        isNMBypass(isNMBypass),
        networkManagerBypassRoute(networkManagerBypassRoute) {}

    MessageTestResult(const size_t _messageSize, const std::string &_recipient,
                      const std::uint32_t _period, const std::uint64_t _count,
                      const std::string &_messagePrefix, bool isNMBypass,
                      const std::string &networkManagerBypassRoute) :
        messageSize(_messageSize),
        recipient(_recipient),
        period(_period),
        count(_count),
        messagePrefix(_messagePrefix),
        isNMBypass(isNMBypass),
        networkManagerBypassRoute(networkManagerBypassRoute) {}

    size_t messageSize;
    std::string message;
    std::string recipient;
    std::uint32_t period;
    std::uint64_t count;
    std::string messagePrefix;
    bool isNMBypass;
    std::string networkManagerBypassRoute;
};

std::ostream &operator<<(std::ostream &os, const MessageTestResult &result);
std::ostream &operator<<(std::ostream &os, const MessageTestResult &result) {
    os << "messageSize: " << result.messageSize << " recipient: " << result.recipient
       << " period: " << result.period << " count: " << result.count << "\n";
    os << "message: " << result.message << "\n";
    os << "isNMBypass: " << result.isNMBypass
       << " networkManagerBypassRoute: " << result.networkManagerBypassRoute << "\n";
    return os;
}

class CreateMessageSendParamTest :
    public testing::TestWithParam<std::pair<std::string, MessageTestResult>> {};

TEST_P(CreateMessageSendParamTest, should_parse_send_message) {
    auto result = rta::Message::createMessage(nlohmann::json::parse(GetParam().first));

    const MessageTestResult &expected = GetParam().second;
    ASSERT_EQ(result.size(), expected.count);
    EXPECT_EQ(result[0].messageContent, expected.message);
    EXPECT_EQ(result[0].personaOfRecipient, expected.recipient);
    EXPECT_LE(result[0].sendTime, decltype(result[0].sendTime)::clock::now());
    EXPECT_EQ(result[0].isNMBypass, expected.isNMBypass);
    EXPECT_EQ(result[0].networkManagerBypassRoute, expected.networkManagerBypassRoute);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    send_input_message,
    CreateMessageSendParamTest,
    ::testing::Values(
        std::make_pair(
            R"({
                "type":"send-message",
                "payload":{"send-type":"manual","recipient":"to someone","message":"hello someone","test-id":"","network-manager-bypass-route":""}
            })",
            MessageTestResult("hello someone", "to someone", false, "")),
        std::make_pair(
            R"({
                "type":"send-message",
                "payload":{"send-type":"manual","recipient":"to someone","message":"hello someone","test-id":"test-id","network-manager-bypass-route":""}
            })",
            MessageTestResult("test-id hello someone", "to someone", false, "")),
        std::make_pair(
            R"({
                "type":"send-message",
                "payload":{"send-type":"manual","recipient":"to someone","message":"hello someone","test-id":"","network-manager-bypass-route":"channel-id"}
            })",
            MessageTestResult("hello someone", "to someone", true, "channel-id"))
    )
);
// clang-format on

class CreateMessageAutoParamTest :
    public testing::TestWithParam<std::pair<std::string, MessageTestResult>> {};

TEST_P(CreateMessageAutoParamTest, should_parse_auto_message) {
    auto result = rta::Message::createMessage(nlohmann::json::parse(GetParam().first));

    const MessageTestResult &expected = GetParam().second;
    EXPECT_EQ(result.size(), expected.count);

    for (size_t i = 0; i < result.size(); ++i) {
        EXPECT_EQ(result[i].messageContent.size() + result[i].generated.size(),
                  expected.messageSize);
        // TODO: test sequence number
        EXPECT_EQ(result[i].messageContent.rfind(expected.messagePrefix, 0) == 0, true);
        EXPECT_EQ(result[i].personaOfRecipient, expected.recipient);
        EXPECT_LE(result[i].sendTime, decltype(result[0].sendTime)::clock::now() +
                                          std::chrono::milliseconds(i * expected.period));
        EXPECT_EQ(result[i].sendTime,
                  result[0].sendTime + std::chrono::milliseconds(i * expected.period));
        // Test send time is reasonably close to now?
    }
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    auto_input_message,
    CreateMessageAutoParamTest,
    ::testing::Values(
        std::make_pair(
            R"({
                "type":"send-message",
                "payload":{"send-type":"auto","recipient":"recipient","size":18,"period":5,"quantity":7,"test-id":"","network-manager-bypass-route":""}
            })",
            MessageTestResult(18, "recipient", 5, 7, "", false, "")),
        std::make_pair(
            R"({
                "type":"send-message",
                "payload":{"send-type":"auto","recipient":"recipient","size":1,"period":3,"quantity":5,"test-id":"test-id2","network-manager-bypass-route":""}
            })",
            MessageTestResult(13, "recipient", 3, 5, "test-id2 ", false, "")),
        std::make_pair(
            R"({
                "type":"send-message",
                "payload":{"send-type":"auto","recipient":"recipient","size":1,"period":2,"quantity":4,"test-id":"","network-manager-bypass-route":""}
            })",
            MessageTestResult(4, "recipient", 2, 4, "", false, "")),
        std::make_pair(
            R"({
                "type":"send-message",
                "payload":{"send-type":"auto","recipient":"recipient","size":1,"period":2,"quantity":4,"test-id":"","network-manager-bypass-route":"plugin-id/channel-id"}
            })",
            MessageTestResult(4, "recipient", 2, 4, "", true, "plugin-id/channel-id"))
    )
);
// clang-format on

TEST(createMessage, should_throw_for_large_message_size) {
    EXPECT_THROW(rta::Message::createMessage(R"({
        "type": "send-message",
        "payload": {
            "send-type": "auto",
            "recipient": "recipient",
            "size": 10000100,
            "period": 1,
            "quantity": 2,
            "test-id": "",
            "network-manager-bypass-route": ""
        }
    })"_json),
                 std::invalid_argument);
}

TEST(createMessage, should_throw_for_invalid_format) {
    // No payload
    EXPECT_THROW(rta::Message::createMessage(R"({
        "type": "send-message"
    })"_json),
                 std::invalid_argument);
    // Unrecognized send-type
    EXPECT_THROW(rta::Message::createMessage(R"({
        "type": "send-message",
        "payload": {
            "send-type": "type that doesn't exist"
        }
    })"_json),
                 std::invalid_argument);
    // Bad manual payload
    EXPECT_THROW(rta::Message::createMessage(R"({
        "type": "send-message",
        "payload": {
            "send-type": "manual"
        }
    })"_json),
                 std::invalid_argument);
    // Bad auto payload
    EXPECT_THROW(rta::Message::createMessage(R"({
        "type": "send-message",
        "payload": {
            "send-type": "auto"
        }
    })"_json),
                 std::invalid_argument);
    // Bad plan payload
    EXPECT_THROW(rta::Message::createMessage(R"({
        "type": "send-message",
        "payload": {
            "send-type": "plan"
        }
    })"_json),
                 std::invalid_argument);
}
