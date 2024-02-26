
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

#include "LinkType.h"
#include "gtest/gtest.h"

TEST(LinkType, linkTypeToString) {
    EXPECT_EQ(linkTypeToString(LT_UNDEF), "LT_UNDEF");
    EXPECT_EQ(linkTypeToString(LT_SEND), "LT_SEND");
    EXPECT_EQ(linkTypeToString(LT_RECV), "LT_RECV");
    EXPECT_EQ(linkTypeToString(LT_BIDI), "LT_BIDI");
    EXPECT_EQ(linkTypeToString(static_cast<LinkType>(99)), "ERROR: INVALID LINK TYPE: 99");
}
