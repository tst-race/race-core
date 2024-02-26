
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

#include "RaceCrypto.h"

#include <string>
#include <vector>

#include "gtest/gtest.h"

// Forces gtest to use our printing method for MsgHash
static void PrintTo(const MsgHash &m, std::ostream *o) {
    *o << m;
}

////////////////////////////////////////////////////////////////
// encryptClrMsg
////////////////////////////////////////////////////////////////

TEST(RaceCrypto, encryptClrMsg) {
    RaceCrypto encryptor;
    // Value based on current encryption. This can be changed if/when this changes.
    std::string plaintext = "abcde";
    const std::vector<std::uint8_t> key{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5,
                                        6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1};
    const RawData result = encryptor.encryptClrMsg(plaintext, key);
    EXPECT_EQ(result.size(), plaintext.length() + 12 + 16);
    std::string decrypted = encryptor.decryptEncPkg(result, key);
    EXPECT_EQ(decrypted, plaintext);
}

TEST(RaceCrypto, encryptClrMsg_empty) {
    RaceCrypto encryptor;
    const std::vector<std::uint8_t> key{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5,
                                        6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1};
    const RawData result = encryptor.encryptClrMsg("", key);
    EXPECT_EQ(result.size(), 12 + 16);
}

////////////////////////////////////////////////////////////////
// parseDelimitedMessage
////////////////////////////////////////////////////////////////

#include <iostream>
TEST(RaceCrypto, parseDelimitedMessage) {
    RaceCrypto encryptor;
    const std::string delimiter = encryptor.getDelimiter();
    const std::string messageToParse = "clrMsg" + delimiter + "hello, world" + delimiter +
                                       "race-client-2" + delimiter + "race-client-1" + delimiter +
                                       "1577836800000000" + delimiter + "1234567890" + delimiter +
                                       "1";
    ClrMsg parsedMsg = encryptor.parseDelimitedMessage(messageToParse);
    EXPECT_EQ(parsedMsg.getMsg(), "hello, world");
    EXPECT_EQ(parsedMsg.getFrom(), "race-client-2");
    EXPECT_EQ(parsedMsg.getTo(), "race-client-1");
    EXPECT_EQ(parsedMsg.getTime(), 1577836800000000);
    EXPECT_EQ(parsedMsg.getNonce(), 1234567890);
    EXPECT_EQ(parsedMsg.getAmpIndex(), 1);
}

TEST(RaceCrypto, parseDelimitedMessage_custom_delimiter) {
    RaceCrypto encryptor;
    encryptor.setDelimiter("~@~");
    const std::string messageToParse =
        "clrMsg~@~hello~@~race-client-1~@~race-client-2~@~1577836800000000~@~1234567890~@~2";
    ClrMsg parsedMsg = encryptor.parseDelimitedMessage(messageToParse);
    EXPECT_EQ(parsedMsg.getMsg(), "hello");
    EXPECT_EQ(parsedMsg.getFrom(), "race-client-1");
    EXPECT_EQ(parsedMsg.getTo(), "race-client-2");
    EXPECT_EQ(parsedMsg.getTime(), 1577836800000000);
    EXPECT_EQ(parsedMsg.getNonce(), 1234567890);
    EXPECT_EQ(parsedMsg.getAmpIndex(), 2);
}

TEST(RaceCrypto, parseDelimitedMessage_empty) {
    RaceCrypto encryptor;
    EXPECT_THROW(encryptor.parseDelimitedMessage(""), std::invalid_argument);
}

TEST(RaceCrypto, parseDelimitedMessage_invalid) {
    const std::string messageToParse = "----------------";
    RaceCrypto encryptor;
    EXPECT_THROW(encryptor.parseDelimitedMessage(messageToParse), std::invalid_argument);
}

////////////////////////////////////////////////////////////////
// getMessageHash
////////////////////////////////////////////////////////////////

// clang-format off
constexpr MsgHash signature {
    {
        // SHA-256 hash expected
        0x30, 0xe8, 0x9e, 0x39, 0x1c, 0x9f, 0xac, 0x1d,
        0x7b, 0x6e, 0xbf, 0x40, 0x97, 0xd6, 0x46, 0xa0,
        0x46, 0x1b, 0xff, 0x92, 0x43, 0x52, 0x12, 0x22,
        0x9d, 0x30, 0xc0, 0xa2, 0xcb, 0xfa, 0x08, 0x2f
    }
};
// clang-format on

TEST(RaceCrypto, getMessageHash) {
    RaceCrypto encryptor;
    // Expected hash relies on a predictable delimiter, so set it before using
    encryptor.setDelimiter(":::");

    ClrMsg input("Hello, World!", "race-client-1", "race-client-2", 0x12345678, 10);

    MsgHash output = encryptor.getMessageHash(input);
    EXPECT_EQ(output, signature);
}

TEST(RaceCrypto, MsgHash_std_hash) {
    if constexpr (sizeof(std::size_t) == 4) {
        EXPECT_EQ(std::hash<MsgHash>{}(signature), 0xe2789f46);
    } else if constexpr (sizeof(std::size_t) == 8) {
        EXPECT_EQ(std::hash<MsgHash>{}(signature), 0x1dac9f1c399ee830);
    }
}
