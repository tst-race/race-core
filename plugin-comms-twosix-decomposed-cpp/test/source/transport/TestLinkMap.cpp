
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

#include <gtest/gtest.h>
#include <race/mocks/MockTransportSdk.h>

#include "LinkMap.h"

class TestLinkMap : public ::testing::Test {
public:
    MockTransportSdk sdk;
    LinkMap map;

    std::shared_ptr<Link> createLink(const std::string &linkId) {
        return std::make_shared<Link>(linkId, LinkAddress(), LinkProperties(), &sdk);
    }
};

TEST_F(TestLinkMap, size) {
    map.add(createLink("LinkID_1"));
    ASSERT_EQ(1, map.size());
    map.add(createLink("LinkID_2"));
    ASSERT_EQ(2, map.size());
    map.clear();
    ASSERT_EQ(0, map.size());
}

TEST_F(TestLinkMap, get) {
    ASSERT_THROW(map.get("LinkID_3"), std::out_of_range);
    map.add(createLink("LinkID_3"));
    ASSERT_NE(nullptr, map.get("LinkID_3"));
    ASSERT_THROW(map.get("LinkID_4"), std::out_of_range);
}

TEST_F(TestLinkMap, remove) {
    ASSERT_EQ(nullptr, map.remove("LinkID_5"));
    map.add(createLink("LinkID_5"));
    ASSERT_NE(nullptr, map.remove("LinkID_5"));
}