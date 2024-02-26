
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

#include "base64.h"

#include <stdexcept>

constexpr char b64_encode_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
static_assert(sizeof(b64_encode_table) == 64 + 1 /* null byte */);

// Lookup table that converts ASCII to Base64, with -1 meaning "invalid".
constexpr std::int8_t b64_decode_table[] = {
    // clang-format off
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    // clang-format on
};
static_assert(sizeof(b64_decode_table) == 256);

std::string base64::encode(const RawData &data) {
    std::string b64;
    b64.reserve((data.size() + 2) / 3 * 4);
    auto it = data.begin();
    const std::size_t size = data.size();

    for (std::size_t i = 0; i < size / 3; it += 3, ++i) {
        b64.push_back(b64_encode_table[((it[0] & 0xFC) >> 2)]);
        b64.push_back(b64_encode_table[((it[0] & 0x03) << 4) | ((it[1] & 0xF0) >> 4)]);
        b64.push_back(b64_encode_table[((it[1] & 0x0F) << 2) | ((it[2] & 0xC0) >> 6)]);
        b64.push_back(b64_encode_table[((it[2] & 0x3F) << 0)]);
    }

    switch (size % 3) {
        case 1:
            b64.push_back(b64_encode_table[((it[0] & 0xFC) >> 2)]);
            b64.push_back(b64_encode_table[((it[0] & 0x03) << 4)]);
            b64.push_back('=');
            b64.push_back('=');
            break;
        case 2:
            b64.push_back(b64_encode_table[((it[0] & 0xFC) >> 2)]);
            b64.push_back(b64_encode_table[((it[0] & 0x03) << 4) | ((it[1] & 0xF0) >> 4)]);
            b64.push_back(b64_encode_table[((it[1] & 0x0F) << 2)]);
            b64.push_back('=');
            break;

        default:
            break;
    }

    return b64;
}

template <int I, class T>
static void decode_block(RawData &data, T &it) {
    std::int8_t a, b, c, d;

    if ((I >= 1 && (a = b64_decode_table[std::uint8_t(*it++)]) < 0) ||
        (I >= 1 && (b = b64_decode_table[std::uint8_t(*it++)]) < 0) ||
        (I >= 2 && (c = b64_decode_table[std::uint8_t(*it++)]) < 0) ||
        (I >= 3 && (d = b64_decode_table[std::uint8_t(*it++)]) < 0)) {
        throw std::invalid_argument("Unexpected character in Base64 string");
    }

    if constexpr (I >= 1) {
        data.push_back(((std::uint8_t(a) & 0x3F) << 2) | ((std::uint8_t(b) & 0x30) >> 4));
    }
    if constexpr (I >= 2) {
        data.push_back(((std::uint8_t(b) & 0x0F) << 4) | ((std::uint8_t(c) & 0x3C) >> 2));
    }
    if constexpr (I >= 3) {
        data.push_back(((std::uint8_t(c) & 0x03) << 6) | ((std::uint8_t(d) & 0x3F) >> 0));
    }
}

RawData base64::decode(const std::string &b64) {
    if (b64.size() % 4 != 0) {
        throw std::invalid_argument("Invalid length for base64 encoded string");
    }
    if (b64.size() == 0) {
        return {};
    }

    RawData data;
    data.reserve(b64.size() / 4 * 3);
    auto it = b64.begin();
    const std::size_t size = b64.size();

    for (std::size_t i = 0; i < size / 4 - 1; ++i) {
        decode_block<3>(data, it);
    }

    if (it[3] != '=') {
        decode_block<3>(data, it);
    } else if (it[2] != '=') {
        decode_block<2>(data, it);
    } else {
        decode_block<1>(data, it);
    }

    return data;
}
