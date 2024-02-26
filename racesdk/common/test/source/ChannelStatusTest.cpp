
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

#include "ChannelStatus.h"
#include "gtest/gtest.h"

TEST(ChannelStatus, toString) {
    EXPECT_EQ(channelStatusToString(CHANNEL_UNDEF), "CHANNEL_UNDEF");
    EXPECT_EQ(channelStatusToString(CHANNEL_AVAILABLE), "CHANNEL_AVAILABLE");
    EXPECT_EQ(channelStatusToString(CHANNEL_UNAVAILABLE), "CHANNEL_UNAVAILABLE");
    EXPECT_EQ(channelStatusToString(CHANNEL_ENABLED), "CHANNEL_ENABLED");
    EXPECT_EQ(channelStatusToString(CHANNEL_DISABLED), "CHANNEL_DISABLED");
    EXPECT_EQ(channelStatusToString(CHANNEL_STARTING), "CHANNEL_STARTING");
    EXPECT_EQ(channelStatusToString(CHANNEL_FAILED), "CHANNEL_FAILED");
    EXPECT_EQ(channelStatusToString(static_cast<ChannelStatus>(99)),
              "ERROR: INVALID CHANNEL STATUS: 99");
}
