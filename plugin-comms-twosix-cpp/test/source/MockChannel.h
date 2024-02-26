
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

#ifndef _MOCK_CHANNEL_H_
#define _MOCK_CHANNEL_H_

#include "../../source/base/Channel.h"
#include "gmock/gmock.h"

class MockChannel : public Channel {
public:
    explicit MockChannel(PluginCommsTwoSixCpp &plugin) : Channel(plugin, "MockChannel") {}

    MOCK_METHOD(PluginResponse, createLink, (RaceHandle handle), (override));
    MOCK_METHOD(PluginResponse, loadLinkAddress,
                (RaceHandle handle, const std::string &linkAddress), (override));
    MOCK_METHOD(PluginResponse, loadLinkAddresses,
                (RaceHandle handle, const std::vector<std::string> &linkAddresses), (override));
    MOCK_METHOD(PluginResponse, activateChannel, (RaceHandle handle), (override));
    MOCK_METHOD(PluginResponse, deactivateChannel, (RaceHandle handle), (override));

    MOCK_METHOD(void, onLinkDestroyed, (Link * link), (override));
    MOCK_METHOD(bool, onUserInputReceived,
                (RaceHandle handle, bool answered, const std::string &response), (override));

protected:
    MOCK_METHOD(std::shared_ptr<Link>, createLink, (const LinkID &linkId), (override));
    MOCK_METHOD(std::shared_ptr<Link>, createLinkFromAddress,
                (const LinkID &linkId, const std::string &linkAddress), (override));
    MOCK_METHOD(std::shared_ptr<Link>, loadLink,
                (const LinkID &linkId, const std::string &linkAddress), (override));
    MOCK_METHOD(void, onGenesisLinkCreated, (Link * link), (override));
    MOCK_METHOD(PluginResponse, activateChannelInternal, (RaceHandle handle), (override));
    MOCK_METHOD(LinkProperties, getDefaultLinkProperties, (), (override));
};

#endif