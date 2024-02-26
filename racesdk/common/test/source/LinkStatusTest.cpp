
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

#include "LinkStatus.h"
#include "gtest/gtest.h"

TEST(LinkStatus, toString) {
    EXPECT_EQ(linkStatusToString(LINK_UNDEF), "LINK_UNDEF");
    EXPECT_EQ(linkStatusToString(LINK_CREATED), "LINK_CREATED");
    EXPECT_EQ(linkStatusToString(LINK_LOADED), "LINK_LOADED");
    EXPECT_EQ(linkStatusToString(LINK_DESTROYED), "LINK_DESTROYED");
    EXPECT_EQ(linkStatusToString(static_cast<LinkStatus>(99)), "ERROR: INVALID LINK STATUS: 99");
}
