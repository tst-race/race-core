
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

#ifndef __MOCK_RACE_BOOTSTRAP_PLUGIN_COMMS_H_
#define __MOCK_RACE_BOOTSTRAP_PLUGIN_COMMS_H_

#include "IRacePluginComms.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockRaceBootstrapPluginComms : public IRacePluginComms {
public:
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
    MOCK_METHOD(PluginResponse, destroyLink, (RaceHandle handle, std::string channelGid),
                (override));
    MOCK_METHOD(PluginResponse, createLink, (RaceHandle handle, std::string channelGid),
                (override));
    MOCK_METHOD(PluginResponse, loadLinkAddress,
                (RaceHandle handle, std::string channelGid, std::string linkAddress), (override));
    MOCK_METHOD(PluginResponse, loadLinkAddresses,
                (RaceHandle handle, std::string channelGid, std::vector<std::string> linkAddresses),
                (override));
    MOCK_METHOD(PluginResponse, createLinkFromAddress,
                (RaceHandle handle, std::string channelGid, std::string linkAddress), (override));
    MOCK_METHOD(PluginResponse, deactivateChannel, (RaceHandle handle, std::string channelGid),
                (override));
    MOCK_METHOD(PluginResponse, activateChannel,
                (RaceHandle handle, std::string channelGid, std::string roleName), (override));
    MOCK_METHOD(PluginResponse, onUserInputReceived,
                (RaceHandle handle, bool answered, const std::string &response), (override));

    MOCK_METHOD(PluginResponse, serveFiles, (LinkID linkId, std::string path), (override));
    MOCK_METHOD(PluginResponse, createBootstrapLink,
                (RaceHandle handle, std::string channelGid, std::string passphrase), (override));
    MOCK_METHOD(PluginResponse, flushChannel,
                (RaceHandle handle, std::string channelGid, uint64_t batchId), (override));
    MOCK_METHOD(PluginResponse, onUserAcknowledgementReceived, (RaceHandle handle), (override));
};

#endif
