
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

#include "TransmissionType.h"
#include "gtest/gtest.h"

TEST(TransmissionType, toString) {
    EXPECT_EQ(transmissionTypeToString(TT_UNDEF), "TT_UNDEF");
    EXPECT_EQ(transmissionTypeToString(TT_UNICAST), "TT_UNICAST");
    EXPECT_EQ(transmissionTypeToString(TT_MULTICAST), "TT_MULTICAST");
    EXPECT_EQ(transmissionTypeToString(static_cast<TransmissionType>(99)),
              "ERROR: INVALID TRANSMISSION TYPE: 99");
}
