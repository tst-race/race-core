
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

#ifndef __COMMS_TWOSIX_TRANSPORT_MOCK_LINK_H__
#define __COMMS_TWOSIX_TRANSPORT_MOCK_LINK_H__

#include <gmock/gmock.h>

#include "Link.h"

class MockLink : public Link {
public:
    using Link::Link;

    MOCK_METHOD(ComponentStatus, enqueueContent,
                (uint64_t actionId, const std::vector<uint8_t> &content), (override));
    MOCK_METHOD(ComponentStatus, dequeueContent, (uint64_t actionId), (override));
    MOCK_METHOD(ComponentStatus, fetch, (), (override));
    MOCK_METHOD(ComponentStatus, post, (std::vector<RaceHandle> handles, uint64_t actionId),
                (override));
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, shutdown, (), (override));
};

#endif  // __COMMS_TWOSIX_TRANSPORT_MOCK_LINK_H__