
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

#include "PackageStatus.h"
#include "gtest/gtest.h"

TEST(PackageStatus, toString) {
    EXPECT_EQ(packageStatusToString(PACKAGE_INVALID), "PACKAGE_INVALID");
    EXPECT_EQ(packageStatusToString(PACKAGE_SENT), "PACKAGE_SENT");
    EXPECT_EQ(packageStatusToString(PACKAGE_RECEIVED), "PACKAGE_RECEIVED");
    EXPECT_EQ(packageStatusToString(PACKAGE_FAILED_GENERIC), "PACKAGE_FAILED_GENERIC");
    EXPECT_EQ(packageStatusToString(PACKAGE_FAILED_NETWORK_ERROR), "PACKAGE_FAILED_NETWORK_ERROR");
    EXPECT_EQ(packageStatusToString(PACKAGE_FAILED_TIMEOUT), "PACKAGE_FAILED_TIMEOUT");
    EXPECT_EQ(packageStatusToString(static_cast<PackageStatus>(99)),
              "ERROR: INVALID PACKAGE STATUS: 99");
}
