
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

#ifndef __MOCK_LINK_MANAGER_H_
#define __MOCK_LINK_MANAGER_H_

#include <utility>

#include "LinkManager.h"
#include "gmock/gmock.h"

class MockLinkManager : public LinkManager {
public:
    MockLinkManager() : LinkManager(nullptr) {}

    MOCK_METHOD(PluginResponse, onChannelStatusChanged,
                (RaceHandle handle, const std::string &channelGid, ChannelStatus status),
                (override));
    MOCK_METHOD(PluginResponse, onLinkStatusChanged,
                (RaceHandle handle, const LinkID &linkId, LinkStatus status,
                 const LinkProperties &properties),
                (override));

    MOCK_METHOD(SdkResponse, createLink,
                (const std::string &channelGid, const std::vector<std::string> &personas),
                (override));
    MOCK_METHOD(SdkResponse, createLinkFromAddress,
                (const std::string &channelGid, const std::string &linkAddress,
                 const std::vector<std::string> &personas),
                (override));
    MOCK_METHOD(SdkResponse, loadLinkAddress,
                (const std::string &channelGid, const std::string &linkAddress,
                 const std::vector<std::string> &personas),
                (override));
    MOCK_METHOD(SdkResponse, loadLinkAddresses,
                (const std::string &channelGid, const std::vector<std::string> &linkAddresses,
                 const std::vector<std::string> &personas),
                (override));

    MOCK_METHOD(SdkResponse, setPersonasForLink,
                (const LinkID &linkId, const std::vector<std::string> &personas), (override));

private:
};

#endif
