
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

#include "../../source/utils/base64.h"

#include <stdexcept>

#include "gtest/gtest.h"

TEST(base64, valid_encode) {
    RawData input{0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68};
    EXPECT_EQ(base64::encode(input), "yQ/aoiFo");

    input.resize(5);
    EXPECT_EQ(base64::encode(input), "yQ/aoiE=");

    input.resize(4);
    EXPECT_EQ(base64::encode(input), "yQ/aog==");
}

TEST(base64, small_encode) {
    RawData input{0xC2, 0xAE, 0xD5};
    EXPECT_EQ(base64::encode(input), "wq7V");

    input.resize(2);
    EXPECT_EQ(base64::encode(input), "wq4=");

    input.resize(1);
    EXPECT_EQ(base64::encode(input), "wg==");
}

TEST(base64, valid_decode) {
    RawData expected{0xB7, 0xE1, 0x51, 0x62, 0x8A, 0xED};
    EXPECT_EQ(base64::decode("t+FRYort"), expected);

    expected.resize(5);
    EXPECT_EQ(base64::decode("t+FRYoo="), expected);

    expected.resize(4);
    EXPECT_EQ(base64::decode("t+FRYg=="), expected);
}

TEST(base64, small_decode) {
    RawData expected{0xE4, 0xC5, 0xE3};
    EXPECT_EQ(base64::decode("5MXj"), expected);

    expected.resize(2);
    EXPECT_EQ(base64::decode("5MU="), expected);

    expected.resize(1);
    EXPECT_EQ(base64::decode("5A=="), expected);
}

TEST(base64, empty_value) {
    EXPECT_EQ(base64::encode({}), "");
    EXPECT_EQ(base64::decode(""), RawData{});
}

TEST(base64, invalid_characters) {
    EXPECT_THROW(base64::decode("!abcdefg"), std::invalid_argument);
    EXPECT_THROW(base64::decode("abcdef!="), std::invalid_argument);
}

TEST(base64, invalid_high_characters) {
    EXPECT_THROW(base64::decode("abc\x80"), std::invalid_argument);
    EXPECT_THROW(base64::decode("abc\xFF"), std::invalid_argument);
}

TEST(base64, invalid_equals_location) {
    EXPECT_THROW(base64::decode("a=bcdef="), std::invalid_argument);
    EXPECT_THROW(base64::decode("abcdef=g"), std::invalid_argument);
}

TEST(base64, invalid_length) {
    EXPECT_THROW(base64::decode("abcdefg"), std::invalid_argument);
    EXPECT_THROW(base64::decode("abcdef"), std::invalid_argument);
    EXPECT_THROW(base64::decode("abcdef="), std::invalid_argument);
    EXPECT_THROW(base64::decode("abcde=="), std::invalid_argument);
    EXPECT_THROW(base64::decode("abcd=="), std::invalid_argument);
}
