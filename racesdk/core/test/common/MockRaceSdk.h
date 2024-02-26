
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

#ifndef __MOCK_RACE_SDK_H_
#define __MOCK_RACE_SDK_H_

#include "../../source/CommsWrapper.h"
#include "IRaceApp.h"
#include "gmock/gmock.h"
#include "helpers.h"
#include "race_printers.h"

class MockRaceSdk : public RaceSdk {
public:
    MockRaceSdk(const AppConfig &appConfig = createDefaultAppConfig(),
                const RaceConfig &raceConfig = createDefaultRaceConfig(),
                IPluginLoader &pluginLoader = IPluginLoader::factoryDefault("/usr/local/lib/")) :
        RaceSdk(appConfig, raceConfig, pluginLoader) {
        // ON_CALL(*this, getAppConfig()).WillByDefault(::testing::ReturnRef(appConfig));
        // ON_CALL(*this, getRaceConfig()).WillByDefault(::testing::ReturnRef(raceConfig));
    }

    // MOCK_METHOD(const AppConfig &, getAppConfig, (), (const override));
    // MOCK_METHOD(const RaceConfig &, getRaceConfig, (), (const override));
    MOCK_METHOD(const std::shared_ptr<opentracing::Tracer> &, getTracer, (), (const override));
    MOCK_METHOD(const std::unique_ptr<CommsWrapper> &, getCommsWrapper, (const std::string &name),
                (const override));

    MOCK_METHOD(RawData, getEntropy, (std::uint32_t numBytes), (override));
    MOCK_METHOD(std::string, getActivePersona, (), (override));
    MOCK_METHOD(bool, initRaceSystem, (IRaceApp * app), (override));
    MOCK_METHOD(SdkResponse, onUserInputReceived,
                (RaceHandle handle, bool answered, const std::string &response), (override));
    MOCK_METHOD(std::vector<std::uint8_t>, readFile, (const std::string &filepath), (override));
    MOCK_METHOD(SdkResponse, writeFile,
                (const std::string &filepath, const std::vector<std::uint8_t> &data), (override));

    MOCK_METHOD(SdkResponse, asyncError, (RaceHandle handle, PluginResponse status), (override));

    MOCK_METHOD(SdkResponse, sendBootstrapPkg,
                (NMWrapper & plugin, ConnectionID connectionId, const std::string &persona,
                 const RawData &pkg, int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, sendEncryptedPackage,
                (NMWrapper & plugin, EncPkg ePkg, ConnectionID connectionId, uint64_t batchId,
                 int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, presentCleartextMessage, (NMWrapper & plugin, ClrMsg msg), (override));

    MOCK_METHOD(std::vector<LinkID>, getLinksForPersonas,
                (std::vector<std::string> recipientPersonas, LinkType linkType), (override));

    MOCK_METHOD(std::vector<LinkID>, getLinksForChannel, (std::string channelGid), (override));

    MOCK_METHOD(LinkProperties, getLinkProperties, (LinkID linkId), (override));

    MOCK_METHOD((std::map<std::string, ChannelProperties>), getSupportedChannels, (), (override));

    MOCK_METHOD(std::vector<std::string>, getPersonasForLink, (LinkID), (override));

    MOCK_METHOD(SdkResponse, setPersonasForLink,
                (NMWrapper & plugin, LinkID, std::vector<std::string>), (override));

    MOCK_METHOD(ChannelProperties, getChannelProperties, (std::string), (override));

    MOCK_METHOD(SdkResponse, createLink,
                (NMWrapper & plugin, std::string, std::vector<std::string>, int32_t), (override));

    MOCK_METHOD(SdkResponse, loadLinkAddress,
                (NMWrapper & plugin, std::string, std::string, std::vector<std::string>, int32_t),
                (override));

    MOCK_METHOD(SdkResponse, loadLinkAddresses,
                (NMWrapper & plugin, std::string, std::vector<std::string>,
                 std::vector<std::string>, int32_t),
                (override));

    MOCK_METHOD(SdkResponse, bootstrapDevice,
                (NMWrapper & plugin, RaceHandle handle, std::vector<std::string> commsPlugins),
                (override));

    MOCK_METHOD(SdkResponse, openConnection,
                (NMWrapper & plugin, LinkType linkType, LinkID linkId, std::string linkHints,
                 int32_t priority, int32_t timeout, int32_t sendTimeout),
                (override));
    MOCK_METHOD(SdkResponse, closeConnection,
                (NMWrapper & plugin, ConnectionID connectionId, int32_t timeout), (override));
    MOCK_METHOD(LinkID, getLinkForConnection, (ConnectionID connectionId), (override));

    MOCK_METHOD(SdkResponse, onPackageStatusChanged,
                (CommsWrapper & plugin, RaceHandle handle, PackageStatus status, int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, onConnectionStatusChanged,
                (CommsWrapper & plugin, RaceHandle handle, ConnectionID connId,
                 ConnectionStatus status, LinkProperties properties, int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, onLinkStatusChanged,
                (CommsWrapper & plugin, RaceHandle handle, LinkID linkId, LinkStatus status,
                 LinkProperties properties, int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, onChannelStatusChanged,
                (CommsWrapper & plugin, RaceHandle handle, const std::string &channelGid,
                 ChannelStatus status, const ChannelProperties &properties, int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, updateLinkProperties,
                (CommsWrapper & plugin, const LinkID &linkId, const LinkProperties &properties,
                 int32_t timeout),
                (override));
    MOCK_METHOD(ConnectionID, generateConnectionId, (CommsWrapper & plugin, LinkID linkId),
                (override));
    MOCK_METHOD(LinkID, generateLinkId, (CommsWrapper & plugin, const std::string &channelGid));
    MOCK_METHOD(SdkResponse, receiveEncPkg,
                (CommsWrapper & plugin, const EncPkg &pkg, const std::vector<ConnectionID> &connIDs,
                 int32_t timeout),
                (override));

    MOCK_METHOD(RaceHandle, sendClientMessage, (ClrMsg msg), (override));
    MOCK_METHOD(void, sendNMBypassMessage, (ClrMsg msg, const std::string &route), (override));
    MOCK_METHOD(RaceHandle, prepareToBootstrap,
                (DeviceInfo deviceInfo, std::string passphrase, std::string bootstrapChannelId),
                (override));
    MOCK_METHOD(bool, cancelBootstrap, (RaceHandle handle), (override));
    MOCK_METHOD(bool, onBootstrapFinished, (RaceHandle bootstrapHandle, BootstrapState state),
                (override));

    MOCK_METHOD(std::vector<std::string>, getContacts, (), (override));
    MOCK_METHOD(bool, isConnected, (), (override));

    MOCK_METHOD(void, cleanShutdown, (), (override));
    MOCK_METHOD(void, notifyShutdown, (int numSeconds), (override));
    MOCK_METHOD(NMWrapper *, getNM, (), (override));
    MOCK_METHOD(NMWrapper *, getNM, (RaceHandle handle), (const override));
    MOCK_METHOD(bool, createBootstrapLink,
                (RaceHandle handle, const std::string &passphrase,
                 const std::string &bootstrapChannelId),
                (override));
    MOCK_METHOD(SdkResponse, requestPluginUserInput,
                (const std::string &pluginId, bool isTestHarness, const std::string &key,
                 const std::string &prompt, bool cache),
                (override));
    MOCK_METHOD(SdkResponse, requestCommonUserInput,
                (const std::string &pluginId, bool isTestHarness, const std::string &key),
                (override));
    MOCK_METHOD(SdkResponse, displayInfoToUser,
                (const std::string &pluginId, const std::string &data,
                 RaceEnums::UserDisplayType displayType),
                (override));
    MOCK_METHOD(SdkResponse, displayBootstrapInfoToUser,
                (const std::string &pluginId, const std::string &data,
                 RaceEnums::UserDisplayType displayType, RaceEnums::BootstrapActionType actionType),
                (override));
    MOCK_METHOD(SdkResponse, onUserAcknowledgementReceived, (RaceHandle handle), (override));
    MOCK_METHOD(SdkResponse, sendAmpMessage,
                (const std::string &pluginId, const std::string &destination,
                 const std::string &message),
                (override));
    MOCK_METHOD(void, shutdownPluginAsync, (CommsWrapper & plugin), (override));
};

#endif
