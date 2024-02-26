
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

#include "../../source/MessageSerializer/helpers.h"
#include "gtest/gtest.h"

////////////////////////////////////////////////////////////////////////////////
// msh::convertToHexString
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Fixture class for testing convertFromHexString
 *
 */
class ConvertToHexStringParamTest :
    public testing::TestWithParam<std::pair<size_t, std::string>> {};

TEST_P(ConvertToHexStringParamTest, converts_values) {
    const std::pair<size_t, std::string> testCaseValues = GetParam();
    const std::string result = msh::convertToHexString(testCaseValues.first);
    EXPECT_EQ(result, testCaseValues.second);
}

INSTANTIATE_TEST_SUITE_P(success_cases, ConvertToHexStringParamTest,
                         ::testing::Values(std::make_pair(0, "0"), std::make_pair(1, "1"),
                                           std::make_pair(15, "f"), std::make_pair(4095, "fff")));

class ConvertToHexStringWithPaddingParamTest : public ConvertToHexStringParamTest {};

TEST_P(ConvertToHexStringWithPaddingParamTest, converts_values) {
    const size_t paddingLength = 5;
    const std::pair<size_t, std::string> testCaseValues = GetParam();
    const std::string result = msh::convertToHexString(testCaseValues.first, paddingLength);
    EXPECT_EQ(result, testCaseValues.second);
}

INSTANTIATE_TEST_SUITE_P(success_cases, ConvertToHexStringWithPaddingParamTest,
                         ::testing::Values(std::make_pair(0, "00000"), std::make_pair(1, "00001"),
                                           std::make_pair(15, "0000f"),
                                           std::make_pair(4095, "00fff")));

////////////////////////////////////////////////////////////////////////////////
// msh::convertFromHexString
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Fixture class for testing convertToHexString.
 *
 */
class ConvertFromHexStringParamTest :
    public testing::TestWithParam<std::pair<std::string, size_t>> {};

TEST_P(ConvertFromHexStringParamTest, converts_values) {
    const std::pair<std::string, size_t> testCaseValues = GetParam();
    const size_t result = msh::convertFromHexString(testCaseValues.first);
    EXPECT_EQ(result, testCaseValues.second);
}

INSTANTIATE_TEST_SUITE_P(success_cases, ConvertFromHexStringParamTest,
                         ::testing::Values(std::make_pair("0", 0), std::make_pair("1", 1),
                                           std::make_pair("f", 15), std::make_pair("fff", 4095),
                                           std::make_pair("000", 0), std::make_pair("001", 1),
                                           std::make_pair("00f", 15), std::make_pair("f00", 3840)));

INSTANTIATE_TEST_SUITE_P(failure_cases, ConvertFromHexStringParamTest,
                         ::testing::Values(std::make_pair("", 0), std::make_pair("g", 0),
                                           std::make_pair("-1", 0), std::make_pair("0-1", 0),
                                           std::make_pair("z", 0), std::make_pair("oops", 0),
                                           std::make_pair("-0", 0), std::make_pair("-f", 0),
                                           std::make_pair("@f", 0),
                                           std::make_pair("some message", 0)));

////////////////////////////////////////////////////////////////////////////////
// msh::appendDataToSerializedMessage
////////////////////////////////////////////////////////////////////////////////

TEST(appendDataToSerializedMessage, appends_to_empty_string) {
    std::string serializedMessage;
    msh::appendDataToSerializedMessage(serializedMessage, "some data to append to the message", 7);

    EXPECT_EQ(serializedMessage, "0000022some data to append to the message");
}

TEST(appendDataToSerializedMessage, appends_to_existing_data) {
    std::string serializedMessage = "some existing data in the message";
    msh::appendDataToSerializedMessage(serializedMessage, "some data to append to the message", 7);

    EXPECT_EQ(serializedMessage,
              "some existing data in the message0000022some data to append to the message");
}

////////////////////////////////////////////////////////////////////////////////
// msh::convertClrMsgToVector
////////////////////////////////////////////////////////////////////////////////

TEST(convertClrMsgToVector, should_handle_empty_ClrMsg) {
    const ClrMsg msg("", "", "", 0, 0);
    const std::vector<std::string> result = msh::convertClrMsgToVector(msg);

    EXPECT_EQ(result[0], "");
    EXPECT_EQ(result[1], "");
    EXPECT_EQ(result[2], "");
    EXPECT_EQ(result[3], "0");
    EXPECT_EQ(result[4], "0");
}

TEST(convertClrMsgToVector, should_convert_ClrMsg) {
    const ClrMsg msg("some crazy message", "the sender who this message is from",
                     "the recipient of this awesome message", 9223372036854775807, 2147483647);
    const std::vector<std::string> result = msh::convertClrMsgToVector(msg);

    EXPECT_EQ(result[0], "some crazy message");
    EXPECT_EQ(result[1], "the sender who this message is from");
    EXPECT_EQ(result[2], "the recipient of this awesome message");
    EXPECT_EQ(result[3], "9223372036854775807");
    EXPECT_EQ(result[4], "2147483647");
}

////////////////////////////////////////////////////////////////////////////////
// msh::convertVectorToClrMsg
////////////////////////////////////////////////////////////////////////////////

TEST(convertVectorToClrMsg, should_throw_for_invalid_vector) {
    {
        const std::vector<std::string> input;
        EXPECT_THROW(msh::convertVectorToClrMsg(input), std::invalid_argument);
    }
    {
        const std::vector<std::string> input = {""};
        EXPECT_THROW(msh::convertVectorToClrMsg(input), std::invalid_argument);
    }
    {
        const std::vector<std::string> input = {"", ""};
        EXPECT_THROW(msh::convertVectorToClrMsg(input), std::invalid_argument);
    }
    {
        const std::vector<std::string> input = {"", "", ""};
        EXPECT_THROW(msh::convertVectorToClrMsg(input), std::invalid_argument);
    }
    {
        const std::vector<std::string> input = {"", "", "", ""};
        EXPECT_THROW(msh::convertVectorToClrMsg(input), std::invalid_argument);
    }
    {
        const std::vector<std::string> input = {"", "", "", "", "", ""};
        EXPECT_THROW(msh::convertVectorToClrMsg(input), std::invalid_argument);
    }
    {
        const std::vector<std::string> input = {"", "", "", "", "", "", ""};
        EXPECT_THROW(msh::convertVectorToClrMsg(input), std::invalid_argument);
    }
    {
        const std::vector<std::string> input = {"", "", "", "1234", ""};
        EXPECT_THROW(msh::convertVectorToClrMsg(input), std::invalid_argument);
    }
    {
        const std::vector<std::string> input = {"", "", "", "", "3214"};
        EXPECT_THROW(msh::convertVectorToClrMsg(input), std::invalid_argument);
    }
}

TEST(convertVectorToClrMsg, should_convert_vector) {
    const std::vector<std::string> input = {
        "some crazy message", "the sender who this message is from",
        "the recipient of this awesome message", "9223372036854775807", "2147483647"};

    const ClrMsg result = msh::convertVectorToClrMsg(input);

    EXPECT_EQ(result.getMsg(), "some crazy message");
    EXPECT_EQ(result.getFrom(), "the sender who this message is from");
    EXPECT_EQ(result.getTo(), "the recipient of this awesome message");
    EXPECT_EQ(result.getTime(), 9223372036854775807);
    EXPECT_EQ(result.getNonce(), 2147483647);
}

////////////////////////////////////////////////////////////////////////////////
// msh::convertClrMsgToVector + msh::convertVectorToClrMsg
////////////////////////////////////////////////////////////////////////////////

TEST(convertClrMsgToVector_convertVectorToClrMsg, output_should_equal_input) {
    const std::vector<std::string> input = {
        "some crazy message", "the sender who this message is from",
        "the recipient of this awesome message", "9223372036854775807", "2147483647"};
    const std::vector<std::string> result =
        msh::convertClrMsgToVector(msh::convertVectorToClrMsg(input));

    ASSERT_EQ(result.size(), input.size());
}

////////////////////////////////////////////////////////////////////////////////
// msh::convertVectorToClrMsg + msh::convertClrMsgToVector
////////////////////////////////////////////////////////////////////////////////

TEST(convertVectorToClrMsg_convertClrMsgToVector, output_should_equal_input) {
    const ClrMsg input("some crazy message", "the sender who this message is from",
                       "the recipient of this awesome message", 9223372036854775807, 2147483647);
    const ClrMsg result = msh::convertVectorToClrMsg(msh::convertClrMsgToVector(input));

    EXPECT_EQ(result.getMsg(), input.getMsg());
    EXPECT_EQ(result.getFrom(), input.getFrom());
    EXPECT_EQ(result.getTo(), input.getTo());
    EXPECT_EQ(result.getTime(), input.getTime());
    EXPECT_EQ(result.getNonce(), input.getNonce());
}
