
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

#ifndef __MOCK_PLUGIN_COMMS_H_
#define __MOCK_PLUGIN_COMMS_H_

#include <utility>

#include "../../source/PluginCommsTwoSixCpp.h"
#include "gmock/gmock.h"

class MockPluginComms : public PluginCommsTwoSixCpp {
public:
    explicit MockPluginComms(IRaceSdkComms &raceSdkIn) : PluginCommsTwoSixCpp(&raceSdkIn) {
        ON_CALL(*this, channelFromId(::testing::_))
            .WillByDefault(::testing::Throw(std::out_of_range("mock oor exception")));
    }
    MOCK_METHOD(PluginResponse, init, (const PluginConfig &pluginConfig), (override));

    MOCK_METHOD(PluginResponse, shutdown, (), (override));

    MOCK_METHOD(PluginResponse, sendPackage,
                (RaceHandle handle, ConnectionID connectionId, EncPkg pkg, double timeoutTimestamp,
                 uint64_t batchId),
                (override));
    MOCK_METHOD(PluginResponse, openConnection,
                (RaceHandle handle, LinkType linkType, LinkID linkId, std::string hints,
                 int32_t sendTimeout),
                (override));
    MOCK_METHOD(PluginResponse, closeConnection, (RaceHandle handle, ConnectionID connectionId),
                (override));

    MOCK_METHOD(void, addLink, (const std::shared_ptr<Link> &link), (override));
    MOCK_METHOD(std::shared_ptr<Link>, getLink, (const LinkID &linkId), (override));
    MOCK_METHOD(std::shared_ptr<Connection>, getConnection, (const ConnectionID &connectionId),
                (override));

    MOCK_METHOD(PluginResponse, destroyLink, (RaceHandle handle, LinkID linkId), (override));
    MOCK_METHOD(PluginResponse, createLink, (RaceHandle handle, std::string channelGid),
                (override));
    MOCK_METHOD(PluginResponse, loadLinkAddress,
                (RaceHandle handle, std::string channelGid, std::string linkAddress), (override));
    MOCK_METHOD(PluginResponse, loadLinkAddresses,
                (RaceHandle handle, std::string channelGid, std::vector<std::string> linkAddresses),
                (override));
    MOCK_METHOD(PluginResponse, deactivateChannel, (RaceHandle handle, std::string channelGid),
                (override));
    MOCK_METHOD(PluginResponse, onUserInputReceived,
                (RaceHandle handle, bool answered, const std::string &response), (override));

    MOCK_METHOD(std::shared_ptr<Channel>, channelFromId, (const std::string &id));
    MOCK_METHOD(std::vector<std::shared_ptr<Link>>, linksForChannel,
                (const std::string &channelGid));

private:
};

#endif
