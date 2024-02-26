
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

#include <ClrMsg.h>

#include "../../source/MessageSerializer/MessageSerializer.h"
#include "gtest/gtest.h"

// ClrMsg constructor function signature
// ClrMsg(const std::string &msg,
//        const std::string &from,
//        const std::string &to,
//        std::int64_t      msgTime,
//        std::int32_t      msgNonce);

////////////////////////////////////////////////////////////////////////////////
// MessageSerializer::serialize
////////////////////////////////////////////////////////////////////////////////

// valgrind complains about the built-in gtest generic object printer, so we need our own
std::ostream &operator<<(std::ostream &os, const ClrMsg &msg);
std::ostream &operator<<(std::ostream &os, const ClrMsg &msg) {
    os << "message: " << msg.getMsg() << ", ";
    os << "from: " << msg.getFrom() << ", ";
    os << "to: " << msg.getTo() << ", ";
    os << "send time: " << msg.getTime() << ", ";
    os << "nonce: " << msg.getNonce() << ", ";

    return os;
}

class MessageSerializerSerializeParamTest :
    public testing::TestWithParam<std::pair<ClrMsg, std::string>> {};

TEST_P(MessageSerializerSerializeParamTest, serializes_clear_message) {
    const std::pair<ClrMsg, std::string> testCaseValues = GetParam();
    const std::string result = MessageSerializer::serialize(testCaseValues.first);
    EXPECT_EQ(result, testCaseValues.second);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    success_cases,
    MessageSerializerSerializeParamTest,
    ::testing::Values(
        std::make_pair(
            ClrMsg(
                "some message",
                "from some sender",
                "to some recipient",
                9223372036854775807,
                2147483647
            ),
            "000000c"               // header
            "some message"          // message
            "0000010"               // header
            "from some sender"      // from
            "0000011"               // header
            "to some recipient"     // to
            "0000013"               // header
            "9223372036854775807"   // time
            "000000a"               // header
            "2147483647"            // nonce
        ),
        std::make_pair(
            ClrMsg("", "", "", 0, 0),
            "0000000"   // header
            ""          // message
            "0000000"   // header
            ""          // from
            "0000000"   // header
            ""          // to
            "0000001"   // header
            "0"         // time
            "0000001"   // header
            "0"         // nonce
        )
    )
);
// clang-format on

TEST(MessageSerializerSerialize, should_throw_exception_if_message_exceeds_size_limit) {
    // TODO: this value is subject to change. Look for MESSAGE_SIZE_LIMIT in MessageSerializer.cpp
    // for the current value.
    const size_t messageSize = 268435456;
    {
        const ClrMsg largeMessage(std::string(messageSize, ' '), "from some sender",
                                  "to some recipient", 9223372036854775807, 2147483647);
        EXPECT_THROW(MessageSerializer::serialize(largeMessage), std::invalid_argument);
    }
    {
        const ClrMsg largeMessage("some message", std::string(messageSize, ' '),
                                  "to some recipient", 9223372036854775807, 2147483647);
        EXPECT_THROW(MessageSerializer::serialize(largeMessage), std::invalid_argument);
    }
    {
        const ClrMsg largeMessage("some message", "from some sender", std::string(messageSize, ' '),
                                  9223372036854775807, 2147483647);
        EXPECT_THROW(MessageSerializer::serialize(largeMessage), std::invalid_argument);
    }
}

////////////////////////////////////////////////////////////////////////////////
// MessageSerializer::deserialize
////////////////////////////////////////////////////////////////////////////////

class MessageSerializerDeserializeParamTest :
    public testing::TestWithParam<std::pair<std::string, ClrMsg>> {};

TEST_P(MessageSerializerDeserializeParamTest, deserializes_clear_message) {
    const std::pair<std::string, ClrMsg> testCaseValues = GetParam();
    const ClrMsg result = MessageSerializer::deserialize(testCaseValues.first);

    EXPECT_EQ(result.getMsg(), testCaseValues.second.getMsg());
    EXPECT_EQ(result.getFrom(), testCaseValues.second.getFrom());
    EXPECT_EQ(result.getTo(), testCaseValues.second.getTo());
    EXPECT_EQ(result.getTime(), testCaseValues.second.getTime());
    EXPECT_EQ(result.getNonce(), testCaseValues.second.getNonce());
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    success_cases,
    MessageSerializerDeserializeParamTest,
    ::testing::Values(
        std::make_pair(
            "000000c"               // header
            "some message"          // message
            "0000010"               // header
            "from some sender"      // from
            "0000011"               // header
            "to some recipient"     // to
            "0000013"               // header
            "9223372036854775807"   // time
            "000000a"               // header
            "2147483647",           // nonce
            ClrMsg(
                "some message",
                "from some sender",
                "to some recipient",
                9223372036854775807,
                2147483647
            )
        ),
        std::make_pair(
            "0000000"   // header
            ""          // message
            "0000000"   // header
            ""          // from
            "0000000"   // header
            ""          // to
            "0000001"   // header
            "0"         // time
            "0000001"   // header
            "0",        // nonce
            ClrMsg("", "", "", 0, 0)
        )
    )
);
// clang-format on

class MessageSerializerDeserializeInvalidMessageParamTest :
    public testing::TestWithParam<std::string> {};

TEST_P(MessageSerializerDeserializeInvalidMessageParamTest, should_throw_on_error) {
    const std::string serializedMessage = GetParam();
    EXPECT_THROW(MessageSerializer::deserialize(serializedMessage), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(exception_cases, MessageSerializerDeserializeInvalidMessageParamTest,
                         ::testing::Values("", "abcdef"));
