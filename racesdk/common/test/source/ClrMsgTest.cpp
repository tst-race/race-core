
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

#include <iostream>

#include "ClrMsg.h"
#include "gtest/gtest.h"

// valgrind complains about the built-in gtest generic object printer, so we need our own
std::ostream &operator<<(std::ostream &os, const ClrMsg &msg);
std::ostream &operator<<(std::ostream &os, const ClrMsg &msg) {
    os << "message: " << msg.getMsg() << ", ";
    os << "from: " << msg.getFrom() << ", ";
    os << "to: " << msg.getTo() << ", ";
    os << "send time: " << msg.getTime() << ", ";
    os << "nonce: " << msg.getNonce() << ", ";
    os << "ampIndex: " << msg.getAmpIndex() << ", ";

    return os;
}

TEST(ClrMsg, constructor1) {
    ClrMsg message("this is a message", "this is the sender", "this is the recipient", 1, 0, 0);

    ASSERT_EQ(message.getMsg(), "this is a message");
    ASSERT_EQ(message.getFrom(), "this is the sender");
    ASSERT_EQ(message.getTo(), "this is the recipient");
    ASSERT_EQ(message.getTime(), 1);
    ASSERT_EQ(message.getNonce(), 0);
    ASSERT_EQ(message.getAmpIndex(), 0);
    ASSERT_EQ(message.getTraceId(), 0);
    ASSERT_EQ(message.getSpanId(), 0);
}

TEST(ClrMsg, constructor2) {
    ClrMsg message("this is a message", "this is the sender", "this is the recipient", 1, 0, 0,
                   1234, 5678);

    ASSERT_EQ(message.getMsg(), "this is a message");
    ASSERT_EQ(message.getFrom(), "this is the sender");
    ASSERT_EQ(message.getTo(), "this is the recipient");
    ASSERT_EQ(message.getTime(), 1);
    ASSERT_EQ(message.getNonce(), 0);
    ASSERT_EQ(message.getAmpIndex(), 0);
    ASSERT_EQ(message.getTraceId(), 1234);
    ASSERT_EQ(message.getSpanId(), 5678);
}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

class ClrMsgEqTest : public testing::TestWithParam<std::pair<ClrMsg, ClrMsg>> {};
TEST_P(ClrMsgEqTest, eq_true) {
    const std::pair<ClrMsg, ClrMsg> params = GetParam();
    EXPECT_EQ(params.first == params.second, true);
}

INSTANTIATE_TEST_SUITE_P(
    success_cases, ClrMsgEqTest,
    ::testing::Values(std::make_pair(ClrMsg{"a", "b", "c", 1, 0}, ClrMsg{"a", "b", "c", 1, 0}),
                      std::make_pair(ClrMsg{"d", "e", "f", 1, 0}, ClrMsg{"d", "e", "f", 1, 0})));

class ClrMsgEqTest2 : public testing::TestWithParam<std::pair<ClrMsg, ClrMsg>> {};
TEST_P(ClrMsgEqTest2, eq_false) {
    const std::pair<ClrMsg, ClrMsg> params = GetParam();
    EXPECT_EQ(params.first == params.second, false);
}

INSTANTIATE_TEST_SUITE_P(
    success_cases, ClrMsgEqTest2,
    ::testing::Values(std::make_pair(ClrMsg{"a", "b", "c", 1, 0}, ClrMsg{"z", "b", "c", 1, 0}),
                      std::make_pair(ClrMsg{"a", "b", "c", 1, 0}, ClrMsg{"a", "z", "c", 1, 0}),
                      std::make_pair(ClrMsg{"a", "b", "c", 1, 0}, ClrMsg{"a", "b", "z", 1, 0}),
                      std::make_pair(ClrMsg{"a", "b", "c", 1, 0}, ClrMsg{"a", "b", "c", 2, 0}),
                      std::make_pair(ClrMsg{"a", "b", "c", 1, 0}, ClrMsg{"a", "b", "c", 1, 2}),
                      std::make_pair(ClrMsg{"d", "e", "f", 1, 0}, ClrMsg{"z", "z", "f", 1, 0}),
                      std::make_pair(ClrMsg{"d", "e", "f", 1, 0}, ClrMsg{"d", "z", "z", 1, 0}),
                      std::make_pair(ClrMsg{"d", "e", "f", 1, 0}, ClrMsg{"d", "e", "z", 2, 0}),
                      std::make_pair(ClrMsg{"d", "e", "f", 1, 0}, ClrMsg{"d", "e", "f", 2, 2}),
                      std::make_pair(ClrMsg{"d", "e", "f", 1, 0}, ClrMsg{"z", "e", "f", 1, 2})));

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
