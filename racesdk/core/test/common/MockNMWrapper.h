
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

#ifndef __MOCK_NETWORK_MANAGER_WRAPPER_H__
#define __MOCK_NETWORK_MANAGER_WRAPPER_H__

#include "../../source/NMWrapper.h"
#include "LogExpect.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockNMWrapper : public NMWrapper {
public:
    MockNMWrapper(LogExpect &logger, RaceSdk &sdk) : NMWrapper(sdk, "MockNM"), logger(logger) {
        using nlohmann::json;
        using ::testing::_;
        mId = "MockNMWrapper";
        ON_CALL(*this, shutdown()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "shutdown");
            return std::tuple<bool, double>{true, 0};
        });
        ON_CALL(*this, shutdown(_)).WillByDefault([this](std::int32_t timeoutInSeconds) {
            LOG_EXPECT(this->logger, "shutdown", timeoutInSeconds);
            return std::tuple<bool, double>{true, 0};
        });
        ON_CALL(*this, processClrMsg(_, _, _))
            .WillByDefault([this](RaceHandle handle, const ClrMsg &msg, int32_t timeout) {
                LOG_EXPECT(this->logger, "processClrMsg", handle, msg, timeout);
                return std::tuple<bool, double>{true, 0};
            });
        ON_CALL(*this, processEncPkg(_, _, _, _))
            .WillByDefault([this](RaceHandle handle, const EncPkg &ePkg,
                                  const std::vector<ConnectionID> &connIDs, int32_t timeout) {
                json connIds = connIDs;
                LOG_EXPECT(this->logger, "processEncPkg", handle, ePkg, connIds, timeout);
                return std::tuple<bool, double>{true, 0};
            });
        ON_CALL(*this, prepareToBootstrap(_, _, _, _, _))
            .WillByDefault([this](RaceHandle handle, LinkID linkId, std::string configPath,
                                  DeviceInfo deviceInfo, int32_t timeout) {
                LOG_EXPECT(this->logger, "prepareToBootstrap", handle, linkId, configPath,
                           deviceInfo, timeout);
                return std::tuple<bool, double>{true, 0};
            });
        ON_CALL(*this, onBootstrapPkgReceived(_, _, _))
            .WillByDefault([this](std::string persona, RawData pkg, int32_t timeout) {
                json pkgJson = pkg;
                LOG_EXPECT(this->logger, "onBootstrapPkgReceived", persona, pkgJson, timeout);
                return std::tuple<bool, double>{true, 0};
            });
        ON_CALL(*this, onPackageStatusChanged(_, _, _))
            .WillByDefault([this](RaceHandle handle, PackageStatus status, int32_t timeout) {
                LOG_EXPECT(this->logger, "onPackageStatusChanged", handle, status, timeout);
                return std::tuple<bool, double>{true, 0};
            });
        ON_CALL(*this, onConnectionStatusChanged(_, _, _, _, _, _))
            .WillByDefault([this](RaceHandle handle, const ConnectionID &connId,
                                  ConnectionStatus status, const LinkID &linkId,
                                  const LinkProperties &properties, int32_t timeout) {
                LOG_EXPECT(this->logger, "onConnectionStatusChanged", handle, connId, status,
                           linkId, properties, timeout);
                return std::tuple<bool, double>{true, 0};
            });
        ON_CALL(*this, onLinkStatusChanged(_, _, _, _, _))
            .WillByDefault([this](RaceHandle handle, LinkID linkId, LinkStatus status,
                                  LinkProperties properties, int32_t timeout) {
                LOG_EXPECT(this->logger, "onLinkStatusChanged", handle, linkId, status, properties,
                           timeout);
                return std::tuple<bool, double>{true, 0};
            });
        ON_CALL(*this, onChannelStatusChanged(_, _, _, _, _))
            .WillByDefault([this](RaceHandle handle, const std::string &channelGid,
                                  ChannelStatus status, const ChannelProperties &properties,
                                  int32_t timeout) {
                LOG_EXPECT(this->logger, "onChannelStatusChanged", handle, channelGid, status,
                           properties, timeout);
                return std::tuple<bool, double>{true, 0};
            });
        ON_CALL(*this, onLinkPropertiesChanged(_, _, _))
            .WillByDefault(
                [this](LinkID linkId, const LinkProperties &linkProperties, int32_t timeout) {
                    LOG_EXPECT(this->logger, "onLinkPropertiesChanged", linkId, linkProperties,
                               timeout);
                    return std::tuple<bool, double>{true, 0};
                });
        ON_CALL(*this, onPersonaLinksChanged(_, _, _, _))
            .WillByDefault([this](std::string recipientPersona, LinkType linkType,
                                  const std::vector<LinkID> &links, int32_t timeout) {
                json linksJson = links;
                LOG_EXPECT(this->logger, "onPersonaLinksChanged", recipientPersona, linkType,
                           linksJson, timeout);
                return std::tuple<bool, double>{true, 0};
            });
        ON_CALL(*this, onUserInputReceived(_, _, _, _))
            .WillByDefault([this](RaceHandle handle, bool answered, const std::string &response,
                                  int32_t timeout) {
                LOG_EXPECT(this->logger, "onUserInputReceived", handle, answered, response,
                           timeout);
                return std::tuple<bool, double>{true, 0};
            });
    }
    MOCK_METHOD(void, startHandler, (), (override));
    MOCK_METHOD(void, stopHandler, (), (override));
    MOCK_METHOD(void, waitForCallbacks, (), (override));
    MOCK_METHOD(bool, init, (const PluginConfig &pluginConfig), (override));
    MOCK_METHOD((std::tuple<bool, double>), shutdown, (), (override));
    MOCK_METHOD((std::tuple<bool, double>), shutdown, (std::int32_t timeoutInSeconds), (override));
    MOCK_METHOD((std::tuple<bool, double>), processClrMsg,
                (RaceHandle handle, const ClrMsg &msg, int32_t timeout), (override));
    MOCK_METHOD((std::tuple<bool, double>), processEncPkg,
                (RaceHandle handle, const EncPkg &ePkg, const std::vector<ConnectionID> &connIDs,
                 int32_t timeout),
                (override));
    MOCK_METHOD((std::tuple<bool, double>), prepareToBootstrap,
                (RaceHandle handle, LinkID linkId, std::string configPath, DeviceInfo deviceInfo,
                 int32_t timeout),
                (override));
    MOCK_METHOD(bool, onBootstrapFinished, (RaceHandle bootstrapHandle, BootstrapState state),
                (override));
    MOCK_METHOD((std::tuple<bool, double>), onBootstrapPkgReceived,
                (std::string persona, RawData pkg, int32_t timeout), (override));
    MOCK_METHOD((std::tuple<bool, double>), onPackageStatusChanged,
                (RaceHandle handle, PackageStatus status, int32_t timeout), (override));
    MOCK_METHOD((std::tuple<bool, double>), onConnectionStatusChanged,
                (RaceHandle handle, const ConnectionID &connId, ConnectionStatus status,
                 const LinkID &linkId, const LinkProperties &properties, int32_t timeout),
                (override));
    MOCK_METHOD((std::tuple<bool, double>), onLinkStatusChanged,
                (RaceHandle handle, LinkID linkId, LinkStatus status, LinkProperties properties,
                 int32_t timeout),
                (override));
    MOCK_METHOD((std::tuple<bool, double>), onChannelStatusChanged,
                (RaceHandle handle, const std::string &channelGid, ChannelStatus status,
                 const ChannelProperties &properties, int32_t timeout),
                (override));
    MOCK_METHOD((std::tuple<bool, double>), onLinkPropertiesChanged,
                (LinkID linkId, const LinkProperties &linkProperties, int32_t timeout), (override));
    MOCK_METHOD((std::tuple<bool, double>), onPersonaLinksChanged,
                (std::string recipientPersona, LinkType linkType, const std::vector<LinkID> &links,
                 int32_t timeout),
                (override));
    MOCK_METHOD((std::tuple<bool, double>), onUserInputReceived,
                (RaceHandle handle, bool answered, const std::string &response, int32_t timeout),
                (override));

    // IRaceSdkCommon
    MOCK_METHOD(RawData, getEntropy, (std::uint32_t numBytes), (override));
    MOCK_METHOD(std::string, getActivePersona, (), (override));
    MOCK_METHOD(SdkResponse, asyncError, (RaceHandle handle, PluginResponse status), (override));
    MOCK_METHOD(SdkResponse, makeDir, (const std::string &directoryPath), (override));
    MOCK_METHOD(SdkResponse, removeDir, (const std::string &directoryPath), (override));
    MOCK_METHOD(std::vector<std::string>, listDir, (const std::string &directoryPath), (override));
    MOCK_METHOD(std::vector<std::uint8_t>, readFile, (const std::string &filepath), (override));
    MOCK_METHOD(SdkResponse, appendFile,
                (const std::string &filepath, const std::vector<std::uint8_t> &data), (override));
    MOCK_METHOD(SdkResponse, writeFile,
                (const std::string &filepath, const std::vector<std::uint8_t> &data), (override));

    // IRaceSdkNM
    MOCK_METHOD(SdkResponse, sendEncryptedPackage,
                (EncPkg ePkg, ConnectionID connectionId, uint64_t batchId, int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, presentCleartextMessage, (ClrMsg msg), (override));
    MOCK_METHOD(SdkResponse, onPluginStatusChanged, (PluginStatus status), (override));
    MOCK_METHOD(SdkResponse, openConnection,
                (LinkType linkType, LinkID linkId, std::string linkHints, int32_t priority,
                 int32_t sendTimeout, int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, closeConnection, (ConnectionID connectionId, int32_t timeout),
                (override));
    MOCK_METHOD(std::vector<LinkID>, getLinksForPersonas,
                (std::vector<std::string> recipientPersonas, LinkType linkType), (override));
    MOCK_METHOD(std::vector<LinkID>, getLinksForChannel, (std::string channelGid), (override));
    MOCK_METHOD(LinkID, getLinkForConnection, (ConnectionID connectionId), (override));
    MOCK_METHOD(LinkProperties, getLinkProperties, (LinkID linkId), (override));
    MOCK_METHOD((std::map<std::string, ChannelProperties>), getSupportedChannels, (), (override));
    MOCK_METHOD(ChannelProperties, getChannelProperties, (std::string channelGid), (override));
    MOCK_METHOD(std::vector<ChannelProperties>, getAllChannelProperties, (), (override));
    MOCK_METHOD(SdkResponse, deactivateChannel, (std::string channelGid, std::int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, activateChannel,
                (std::string channelGid, std::string roleName, std::int32_t timeout), (override));
    MOCK_METHOD(SdkResponse, destroyLink, (LinkID linkId, std::int32_t timeout), (override));
    MOCK_METHOD(SdkResponse, createLink,
                (std::string channelGid, std::vector<std::string> personas, std::int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, loadLinkAddress,
                (std::string channelGid, std::string linkAddress, std::vector<std::string> personas,
                 std::int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, loadLinkAddresses,
                (std::string channelGid, std::vector<std::string> linkAddresses,
                 std::vector<std::string> personas, std::int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, createLinkFromAddress,
                (std::string channelGid, std::string linkAddress, std::vector<std::string> personas,
                 std::int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, bootstrapDevice,
                (RaceHandle handle, std::vector<std::string> commsChannels), (override));
    MOCK_METHOD(SdkResponse, bootstrapFailed, (RaceHandle handle), (override));
    MOCK_METHOD(SdkResponse, setPersonasForLink,
                (std::string linkId, std::vector<std::string> personas), (override));
    MOCK_METHOD(std::vector<std::string>, getPersonasForLink, (std::string linkId), (override));
    MOCK_METHOD(SdkResponse, onMessageStatusChanged, (RaceHandle handle, MessageStatus status),
                (override));
    MOCK_METHOD(SdkResponse, sendBootstrapPkg,
                (ConnectionID connectionId, std::string persona, RawData key, int32_t timout),
                (override));
    MOCK_METHOD(SdkResponse, requestPluginUserInput,
                (const std::string &key, const std::string &prompt, bool cache), (override));
    MOCK_METHOD(SdkResponse, requestCommonUserInput, (const std::string &key), (override));
    MOCK_METHOD(SdkResponse, flushChannel,
                (ConnectionID connId, uint64_t batchId, std::int32_t timeout), (override));
    LogExpect &logger;
};

#endif
