
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

#ifndef __MOCK_RACE_PLUGIN_NETWORK_MANAGER_H_
#define __MOCK_RACE_PLUGIN_NETWORK_MANAGER_H_

#include "IRacePluginNM.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockRacePluginNM : public IRacePluginNM {
public:
    MOCK_METHOD(PluginResponse, init, (const PluginConfig &pluginConfig), (override));
    MOCK_METHOD(PluginResponse, shutdown, (), (override));
    MOCK_METHOD(PluginResponse, processClrMsg, (RaceHandle handle, const ClrMsg &msg), (override));
    MOCK_METHOD(PluginResponse, processEncPkg,
                (RaceHandle handle, const EncPkg &ePkg, const std::vector<ConnectionID> &connIDs),
                (override));
    MOCK_METHOD(PluginResponse, prepareToBootstrap,
                (RaceHandle handle, LinkID linkId, std::string configPath, DeviceInfo deviceInfo),
                (override));
    MOCK_METHOD(PluginResponse, onBootstrapFinished,
                (RaceHandle bootstrapHandle, BootstrapState state), (override));
    MOCK_METHOD(PluginResponse, onBootstrapPkgReceived, (std::string persona, RawData pkg),
                (override));
    MOCK_METHOD(PluginResponse, onPackageStatusChanged, (RaceHandle handle, PackageStatus status),
                (override));
    MOCK_METHOD(PluginResponse, onConnectionStatusChanged,
                (RaceHandle handle, ConnectionID connId, ConnectionStatus status, LinkID linkId,
                 LinkProperties linkProperties),
                (override));
    MOCK_METHOD(PluginResponse, onLinkStatusChanged,
                (RaceHandle handle, LinkID linkId, LinkStatus status,
                 LinkProperties linkProperties),
                (override));
    MOCK_METHOD(PluginResponse, onChannelStatusChanged,
                (RaceHandle handle, std::string channelGid, ChannelStatus status,
                 ChannelProperties channelProperties),
                (override));
    MOCK_METHOD(PluginResponse, onLinkPropertiesChanged,
                (LinkID linkId, LinkProperties linkProperties), (override));
    MOCK_METHOD(PluginResponse, onPersonaLinksChanged,
                (std::string recipientPersona, LinkType linkType, std::vector<LinkID> links),
                (override));
    MOCK_METHOD(PluginResponse, onUserInputReceived,
                (RaceHandle handle, bool answered, const std::string &response), (override));
    MOCK_METHOD(PluginResponse, notifyEpoch, (const std::string &data), (override));
    MOCK_METHOD(PluginResponse, onUserAcknowledgementReceived, (RaceHandle handle), (override));
};

#endif
