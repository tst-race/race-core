
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

#ifndef __MOCK_RACE_SDK_COMMS_H__
#define __MOCK_RACE_SDK_COMMS_H__

#include "IRaceSdkComms.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockRaceSdkComms : public IRaceSdkComms {
public:
    MockRaceSdkComms(LogExpect &logger) : logger(logger) {
        using ::testing::_;
        ON_CALL(*this, getEntropy(_)).WillByDefault([this](std::uint32_t numBytes) {
            LOG_EXPECT(this->logger, "getEntropy", numBytes);
            return RawData{};
        });
        ON_CALL(*this, getActivePersona()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getActivePersona");
            return std::string{};
        });
        ON_CALL(*this, getChannelProperties(_)).WillByDefault([this](std::string channelGid) {
            LOG_EXPECT(this->logger, "getChannelProperties", channelGid);
            return ChannelProperties{};
        });
        ON_CALL(*this, getAllChannelProperties()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getAllChannelProperties");
            return std::vector<ChannelProperties>{};
        });
        ON_CALL(*this, asyncError(_, _))
            .WillByDefault([this](RaceHandle handle, PluginResponse status) {
                LOG_EXPECT(this->logger, "asyncError", handle, status);
                return SDK_OK;
            });
        ON_CALL(*this, makeDir(_)).WillByDefault([this](const std::string &directoryPath) {
            LOG_EXPECT(this->logger, "makeDir", directoryPath);
            return SDK_OK;
        });
        ON_CALL(*this, removeDir(_)).WillByDefault([this](const std::string &directoryPath) {
            LOG_EXPECT(this->logger, "removeDir", directoryPath);
            return SDK_OK;
        });
        ON_CALL(*this, listDir(_)).WillByDefault([this](const std::string &directoryPath) {
            LOG_EXPECT(this->logger, "listDir", directoryPath);
            return std::vector<std::string>{};
        });
        ON_CALL(*this, readFile(_)).WillByDefault([this](const std::string &filepath) {
            LOG_EXPECT(this->logger, "readFile", filepath);
            return std::vector<std::uint8_t>{};
        });
        ON_CALL(*this, appendFile(_, _))
            .WillByDefault(
                [this](const std::string &filepath, const std::vector<std::uint8_t> &data) {
                    LOG_EXPECT(this->logger, "appendFile", filepath, data.size());
                    return SDK_OK;
                });
        ON_CALL(*this, writeFile(_, _))
            .WillByDefault(
                [this](const std::string &filepath, const std::vector<std::uint8_t> &data) {
                    LOG_EXPECT(this->logger, "writeFile", filepath, data.size());
                    return SDK_OK;
                });
        ON_CALL(*this, onPackageStatusChanged(_, _, _))
            .WillByDefault([this](RaceHandle handle, PackageStatus status, int32_t timeout) {
                LOG_EXPECT(this->logger, "onPackageStatusChanged", handle, status, timeout);
                return SDK_OK;
            });
        ON_CALL(*this, onConnectionStatusChanged(_, _, _, _, _))
            .WillByDefault([this](RaceHandle handle, ConnectionID connId, ConnectionStatus status,
                                  LinkProperties properties, int32_t timeout) {
                LOG_EXPECT(this->logger, "onConnectionStatusChanged", handle, connId, status,
                           properties, timeout);
                return SDK_OK;
            });
        ON_CALL(*this, onChannelStatusChanged(_, _, _, _, _))
            .WillByDefault([this](RaceHandle handle, std::string channelGid, ChannelStatus status,
                                  ChannelProperties properties, int32_t timeout) {
                LOG_EXPECT(this->logger, "onChannelStatusChanged", handle, channelGid, status,
                           properties, timeout);
                return SDK_OK;
            });
        ON_CALL(*this, onLinkStatusChanged(_, _, _, _, _))
            .WillByDefault([this](RaceHandle handle, LinkID linkId, LinkStatus status,
                                  LinkProperties properties, int32_t timeout) {
                LOG_EXPECT(this->logger, "onLinkStatusChanged", handle, linkId, status, properties,
                           timeout);
                return SDK_OK;
            });
        ON_CALL(*this, updateLinkProperties(_, _, _))
            .WillByDefault([this](LinkID linkId, LinkProperties properties, int32_t timeout) {
                LOG_EXPECT(this->logger, "updateLinkProperties", linkId, properties, timeout);
                return SDK_OK;
            });
        ON_CALL(*this, generateConnectionId(_)).WillByDefault([this](LinkID linkId) {
            LOG_EXPECT(this->logger, "generateConnectionId", linkId);
            return "default connection id";
        });
        ON_CALL(*this, generateLinkId(_)).WillByDefault([this](std::string channelGid) {
            LOG_EXPECT(this->logger, "generateLinkId", channelGid);
            return "default link id";
        });
        ON_CALL(*this, receiveEncPkg(_, _, _))
            .WillByDefault([this](const EncPkg &pkg, const std::vector<ConnectionID> &connIDs,
                                  int32_t timeout) {
                json connIdsJson = connIDs;
                LOG_EXPECT(this->logger, "receiveEncPkg", pkg.getSize(), connIdsJson, timeout);
                return SDK_OK;
            });
        ON_CALL(*this, requestPluginUserInput(_, _, _))
            .WillByDefault([this](const std::string &key, const std::string &prompt, bool cache) {
                LOG_EXPECT(this->logger, "requestPluginUserInput", key, prompt, cache);
                return SDK_OK;
            });
        ON_CALL(*this, requestCommonUserInput(_)).WillByDefault([this](const std::string &key) {
            LOG_EXPECT(this->logger, "requestCommonUserInput", key);
            return SDK_OK;
        });
        ON_CALL(*this, displayInfoToUser(_, _))
            .WillByDefault([this](const std::string &data, RaceEnums::UserDisplayType displayType) {
                LOG_EXPECT(this->logger, "displayInfoToUser", data, displayType);
                return SDK_OK;
            });
        ON_CALL(*this, displayBootstrapInfoToUser(_, _, _))
            .WillByDefault([this](const std::string &data, RaceEnums::UserDisplayType displayType,
                                  RaceEnums::BootstrapActionType actionType) {
                LOG_EXPECT(this->logger, "displayBootstrapInfoToUser", data, displayType,
                           actionType);
                return SDK_OK;
            });
        ON_CALL(*this, unblockQueue(_)).WillByDefault([this](const ConnectionID &connId) {
            LOG_EXPECT(this->logger, "unblockQueue", connId);
            return SDK_OK;
        });
    }
    MOCK_METHOD(RawData, getEntropy, (std::uint32_t numBytes), (override));
    MOCK_METHOD(std::string, getActivePersona, (), (override));
    MOCK_METHOD(ChannelProperties, getChannelProperties, (std::string channelGid), (override));
    MOCK_METHOD(std::vector<ChannelProperties>, getAllChannelProperties, (), (override));
    MOCK_METHOD(SdkResponse, asyncError, (RaceHandle handle, PluginResponse status), (override));
    MOCK_METHOD(SdkResponse, makeDir, (const std::string &directoryPath), (override));
    MOCK_METHOD(SdkResponse, removeDir, (const std::string &directoryPath), (override));
    MOCK_METHOD(std::vector<std::string>, listDir, (const std::string &directoryPath), (override));
    MOCK_METHOD(std::vector<std::uint8_t>, readFile, (const std::string &filepath), (override));
    MOCK_METHOD(SdkResponse, appendFile,
                (const std::string &filepath, const std::vector<std::uint8_t> &data), (override));
    MOCK_METHOD(SdkResponse, writeFile,
                (const std::string &filepath, const std::vector<std::uint8_t> &data), (override));

    MOCK_METHOD(SdkResponse, onPackageStatusChanged,
                (RaceHandle handle, PackageStatus status, int32_t timeout), (override));
    MOCK_METHOD(SdkResponse, onConnectionStatusChanged,
                (RaceHandle handle, ConnectionID connId, ConnectionStatus status,
                 LinkProperties properties, int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, onChannelStatusChanged,
                (RaceHandle handle, std::string channelGid, ChannelStatus status,
                 ChannelProperties properties, int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, onLinkStatusChanged,
                (RaceHandle handle, LinkID linkId, LinkStatus status, LinkProperties properties,
                 int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, updateLinkProperties,
                (LinkID linkId, LinkProperties properties, int32_t timeout), (override));
    MOCK_METHOD(ConnectionID, generateConnectionId, (LinkID linkId), (override));
    MOCK_METHOD(LinkID, generateLinkId, (std::string channelGid), (override));
    MOCK_METHOD(SdkResponse, receiveEncPkg,
                (const EncPkg &pkg, const std::vector<ConnectionID> &connIDs, int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, requestPluginUserInput,
                (const std::string &key, const std::string &prompt, bool cache), (override));
    MOCK_METHOD(SdkResponse, requestCommonUserInput, (const std::string &key), (override));
    MOCK_METHOD(SdkResponse, displayInfoToUser,
                (const std::string &data, RaceEnums::UserDisplayType displayType), (override));
    MOCK_METHOD(SdkResponse, displayBootstrapInfoToUser,
                (const std::string &data, RaceEnums::UserDisplayType displayType,
                 RaceEnums::BootstrapActionType actionType),
                (override));
    MOCK_METHOD(SdkResponse, unblockQueue, (ConnectionID connId), (override));

public:
    LogExpect &logger;
};

#endif