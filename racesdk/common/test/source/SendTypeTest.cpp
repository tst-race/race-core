
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

#include "SendType.h"
#include "gtest/gtest.h"

TEST(SendType, sendTypeToString) {
    EXPECT_EQ(sendTypeToString(ST_UNDEF), "ST_UNDEF");
    EXPECT_EQ(sendTypeToString(ST_STORED_ASYNC), "ST_STORED_ASYNC");
    EXPECT_EQ(sendTypeToString(ST_EPHEM_SYNC), "ST_EPHEM_SYNC");
    EXPECT_EQ(sendTypeToString(static_cast<SendType>(99)), "ERROR: INVALID SEND TYPE: 99");
}
