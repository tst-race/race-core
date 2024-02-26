
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
#include "racetestapp/raceTestAppHelpers.h"

class CreateRandomStringParamTest : public testing::TestWithParam<size_t> {};

TEST_P(CreateRandomStringParamTest, creates_string_of_size) {
    const size_t randomStringSize = GetParam();
    const std::string result = rtah::createRandomString(randomStringSize);
    EXPECT_EQ(result.length(), randomStringSize);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    random_string_size_range,
    CreateRandomStringParamTest,
    ::testing::Range(0lu, 1000lu, 7lu)
);

INSTANTIATE_TEST_SUITE_P(
    stress_test,
    CreateRandomStringParamTest,
    ::testing::Values(5000000lu, 10000000lu)
);
// clang-format on

TEST(createRandomString, should_throw_for_large_length) {
    {
        const size_t randomStringSize = 10000001;
        EXPECT_THROW(rtah::createRandomString(randomStringSize), std::invalid_argument);
    }
    {
        const size_t randomStringSize = 99999999999;
        EXPECT_THROW(rtah::createRandomString(randomStringSize), std::invalid_argument);
    }
}
