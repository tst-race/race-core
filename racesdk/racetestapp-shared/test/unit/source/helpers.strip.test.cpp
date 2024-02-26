
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

TEST(strip, should_strip_leading_and_trailing_whitespace) {
    std::string input = "   \tsome message\n\n";
    rtah::strip(input);
    EXPECT_EQ(input, "some message");
}

TEST(strip, should_not_change_strings_without_trailing_and_leading_whitespace) {
    std::string input = "some\t\t other\n\n message";
    rtah::strip(input);
    EXPECT_EQ(input, "some\t\t other\n\n message");
}
