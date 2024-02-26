
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

#ifndef __MOCK_PLUGIN_NETWORK_MANAGER_H_
#define __MOCK_PLUGIN_NETWORK_MANAGER_H_

#include <utility>

#include "MockLinkManager.h"
#include "PluginNMTwoSix.h"
#include "gmock/gmock.h"

class MockPluginNM : public PluginNMTwoSix {
public:
    explicit MockPluginNM(IRaceSdkNM &raceSdkIn) : PluginNMTwoSix(&raceSdkIn, P_UNDEF) {}
    MOCK_METHOD(PluginResponse, init, (const PluginConfig &pluginConfig), (override));
    MOCK_METHOD(PluginResponse, processClrMsg, (RaceHandle handle, const ClrMsg &msg), (override));
    MOCK_METHOD(PluginResponse, processEncPkg,
                (RaceHandle handle, const EncPkg &ePkg, const std::vector<ConnectionID> &connIDs),
                (override));

    MOCK_METHOD(void, onStaticLinksCreated, (), (override));
    MOCK_METHOD(LinkID, getPreferredLinkIdForSendingToPersona,
                (const std::vector<LinkID> &potentialLinks, const PersonaType recipientPersonaType),
                (override));
    MOCK_METHOD(bool, hasNecessaryConnections, (), (override));
    MOCK_METHOD(RaceHandle, sendMsg, (const std::string &dstUuid, const ClrMsg &msg), (override));
    MOCK_METHOD(RaceHandle, sendFormattedMsg,
                (const std::string &dstUuid, const std::string &msgString,
                 const std::uint64_t traceId, const std::uint64_t spanId),
                (override));
    MOCK_METHOD(bool, invokeLinkWizard, ((std::unordered_map<std::string, Persona>)personas),
                (override));
    MOCK_METHOD(std::vector<std::string>, getExpectedChannels, (const std::string &uuid),
                (override));
    MOCK_METHOD(std::string, getJaegerConfigPath, (), (override));
    MOCK_METHOD(void, insertConnection,
                ((std::vector<std::pair<ConnectionID, LinkProperties>>)&rankedConnections,
                 const ConnectionID &newConn, const LinkProperties &newProps,
                 const PersonaType recipientPersonaType),
                (override));

    MOCK_METHOD(std::vector<uint8_t>, getAesKeyForSelf, (), (override));
    MOCK_METHOD(void, writeConfigs, (), (override));
    MOCK_METHOD(void, addClient, (const std::string &persona, const RawData &key), (override));

    MOCK_METHOD(PluginResponse, prepareToBootstrap,
                (RaceHandle handle, LinkID linkId, std::string configPath, DeviceInfo deviceInfo),
                (override));
    MOCK_METHOD(PluginResponse, onBootstrapPkgReceived, (std::string persona, RawData pkg),
                (override));

    virtual LinkManager *getLinkManager() override {
        return &mockLinkManager;
    }

    MockLinkManager mockLinkManager;
};

#endif
