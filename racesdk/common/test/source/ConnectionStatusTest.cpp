
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

#include "ConnectionStatus.h"
#include "gtest/gtest.h"

TEST(ConnectionStatus, toString) {
    EXPECT_EQ(connectionStatusToString(CONNECTION_INVALID), "CONNECTION_INVALID");
    EXPECT_EQ(connectionStatusToString(CONNECTION_OPEN), "CONNECTION_OPEN");
    EXPECT_EQ(connectionStatusToString(CONNECTION_CLOSED), "CONNECTION_CLOSED");
    EXPECT_EQ(connectionStatusToString(CONNECTION_AWAITING_CONTACT), "CONNECTION_AWAITING_CONTACT");
    EXPECT_EQ(connectionStatusToString(CONNECTION_INIT_FAILED), "CONNECTION_INIT_FAILED");
    EXPECT_EQ(connectionStatusToString(CONNECTION_AVAILABLE), "CONNECTION_AVAILABLE");
    EXPECT_EQ(connectionStatusToString(CONNECTION_UNAVAILABLE), "CONNECTION_UNAVAILABLE");
    EXPECT_EQ(connectionStatusToString(static_cast<ConnectionStatus>(99)),
              "ERROR: INVALID CONNECTION STATUS: 99");
}
