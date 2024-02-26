
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

#include "EncPkg.h"
#include "gtest/gtest.h"

TEST(EncPkg, constructor1) {
    EncPkg package(273, 546, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

    ASSERT_EQ(package.getCipherText(), RawData({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    ASSERT_EQ(package.getTraceId(), 273);
    ASSERT_EQ(package.getSpanId(), 546);
    ASSERT_EQ(package.getPackageType(), PKG_TYPE_UNDEF);
    package.setPackageType(PKG_TYPE_TEST_HARNESS);
    ASSERT_EQ(package.getRawData(),
              RawData({0x11, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  // little endian trace id
                       0x22, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  // little endian span id
                       2,                                        // package type
                       0,    1,   2,   3,   4,   5,   6,   7,   8, 9}));
}

TEST(EncPkg, constructor2) {
    EncPkg package(RawData({0x11, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  // little endian trace id
                            0x22, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  // little endian span id
                            1,                                        // package type
                            0,    1,   2,   3,   4,   5,   6,   7,   8, 9}));

    ASSERT_EQ(package.getCipherText(), RawData({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    ASSERT_EQ(package.getTraceId(), 273);
    ASSERT_EQ(package.getSpanId(), 546);
    ASSERT_EQ(package.getPackageType(), PKG_TYPE_NM);
    ASSERT_EQ(package.getRawData(),
              RawData({0x11, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  // little endian trace id
                       0x22, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  // little endian span id
                       1,                                        // package type
                       0,    1,   2,   3,   4,   5,   6,   7,   8, 9}));
}

TEST(EncPkg, eq_true) {
    // different trace ids shouldn't matter
    EncPkg package1(12, 34, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    EncPkg package2(56, 78, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

    ASSERT_EQ(package1 == package2, true);
}

TEST(EncPkg, eq_false) {
    // different trace ids shouldn't matter
    EncPkg package1(1234, 5678, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    EncPkg package2(1234, 5678, {0, 1, 2, 3, 4, 5, 5, 7, 8, 9});

    ASSERT_EQ(package1 == package2, false);
}
