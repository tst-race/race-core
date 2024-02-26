
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

TEST(makeClrMsg, should_create_clear_message) {
    const ClrMsg result = rtah::makeClrMsg("some message", "from a sender", "to a recipient");

    EXPECT_EQ(result.getMsg(), "some message");
    EXPECT_EQ(result.getFrom(), "from a sender");
    EXPECT_EQ(result.getTo(), "to a recipient");
    EXPECT_GT(result.getTime(), 0);
    EXPECT_EQ(result.getNonce(), 10);
}
