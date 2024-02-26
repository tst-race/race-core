
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

#include "ConnectionType.h"
#include "gtest/gtest.h"

TEST(ConnectionType, connectionTypeToString) {
    EXPECT_EQ(connectionTypeToString(CT_UNDEF), "CT_UNDEF");
    EXPECT_EQ(connectionTypeToString(CT_DIRECT), "CT_DIRECT");
    EXPECT_EQ(connectionTypeToString(CT_INDIRECT), "CT_INDIRECT");
    EXPECT_EQ(connectionTypeToString(CT_MIXED), "CT_MIXED");
    EXPECT_EQ(connectionTypeToString(CT_LOCAL), "CT_LOCAL");
    EXPECT_EQ(connectionTypeToString(static_cast<ConnectionType>(99)),
              "ERROR: INVALID CONNECTION TYPE: 99");
}
