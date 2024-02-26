
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

#ifndef __MOCK_RACE_SDK_NETWORK_MANAGER_H_
#define __MOCK_RACE_SDK_NETWORK_MANAGER_H_

#include "IRaceSdkNM.h"
#include "gmock/gmock.h"

using ::testing::Matcher;

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
class MockRaceSdkNM : public IRaceSdkNM {
public:
    MOCK_METHOD(RawData, getEntropy, (std::uint32_t), (override));
    MOCK_METHOD(std::string, getActivePersona, (), (override));
    MOCK_METHOD(LinkProperties, getLinkProperties, (LinkID), (override));
    MOCK_METHOD(SdkResponse, sendEncryptedPackage, (EncPkg, ConnectionID, uint64_t, int32_t),
                (override));
    MOCK_METHOD(SdkResponse, presentCleartextMessage, (ClrMsg), (override));
    MOCK_METHOD(std::vector<LinkID>, getLinksForPersonas, (std::vector<std::string>, LinkType),
                (override));
    MOCK_METHOD(std::vector<LinkID>, getLinksForChannel, (std::string), (override));
    MOCK_METHOD(LinkID, getLinkForConnection, (ConnectionID), (override));
    MOCK_METHOD(SdkResponse, openConnection,
                (LinkType, LinkID, std::string, int32_t, int32_t, int32_t), (override));
    MOCK_METHOD(SdkResponse, closeConnection, (ConnectionID, int32_t), (override));
    MOCK_METHOD((std::map<std::string, ChannelProperties>), getSupportedChannels, (), (override));
    MOCK_METHOD(std::vector<std::string>, getPersonasForLink, (LinkID), (override));
    MOCK_METHOD(SdkResponse, setPersonasForLink, (LinkID, std::vector<std::string>), (override));
    MOCK_METHOD(ChannelProperties, getChannelProperties, (std::string), (override));
    MOCK_METHOD(std::vector<ChannelProperties>, getAllChannelProperties, (), (override));
    MOCK_METHOD(SdkResponse, deactivateChannel, (std::string, int32_t), (override));
    MOCK_METHOD(SdkResponse, activateChannel, (std::string, std::string, int32_t), (override));
    MOCK_METHOD(SdkResponse, destroyLink, (std::string, int32_t), (override));
    MOCK_METHOD(SdkResponse, createLink, (std::string, std::vector<std::string>, int32_t),
                (override));
    MOCK_METHOD(SdkResponse, loadLinkAddress,
                (std::string, std::string, std::vector<std::string>, int32_t), (override));
    MOCK_METHOD(SdkResponse, loadLinkAddresses,
                (std::string, std::vector<std::string>, std::vector<std::string>, int32_t),
                (override));
    MOCK_METHOD(SdkResponse, createLinkFromAddress,
                (std::string channelGid, std::string linkAddress, std::vector<std::string> personas,
                 std::int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, bootstrapDevice, (RaceHandle, std::vector<std::string>));
    MOCK_METHOD(SdkResponse, bootstrapFailed, (RaceHandle));
    MOCK_METHOD(SdkResponse, asyncError, (RaceHandle, PluginResponse), (override));
    MOCK_METHOD(std::vector<std::string>, listDir, (const std::string &dirpath), (override));
    MOCK_METHOD(SdkResponse, makeDir, (const std::string &dirpath), (override));
    MOCK_METHOD(SdkResponse, removeDir, (const std::string &dirpath), (override));
    MOCK_METHOD(std::vector<std::uint8_t>, readFile, (const std::string &filename), (override));
    MOCK_METHOD(SdkResponse, appendFile,
                (const std::string &filepath, const std::vector<std::uint8_t> &data), (override));
    MOCK_METHOD(SdkResponse, writeFile,
                (const std::string &filepath, const std::vector<std::uint8_t> &data), (override));
    MOCK_METHOD(SdkResponse, onMessageStatusChanged, (RaceHandle handle, MessageStatus status),
                (override));
    MOCK_METHOD(SdkResponse, onPluginStatusChanged, (PluginStatus pluginStatus), (override));
    MOCK_METHOD(SdkResponse, sendBootstrapPkg,
                (ConnectionID connId, std::string persona, RawData pkg, int32_t timeout),
                (override));
    MOCK_METHOD(SdkResponse, requestPluginUserInput,
                (const std::string &key, const std::string &prompt, bool cache), (override));
    MOCK_METHOD(SdkResponse, requestCommonUserInput, (const std::string &key), (override));
    MOCK_METHOD(SdkResponse, flushChannel,
                (std::string channelGid, uint64_t batchId, int32_t timeout), (override));
    MOCK_METHOD(SdkResponse, displayInfoToUser,
                (const std::string &data, RaceEnums::UserDisplayType displayType), (override));
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

static std::ostream &operator<<(std::ostream &os, const ClrMsg & /*clrMsg*/) {
    os << "<ClrMsg>"
       << "\n";
    return os;
}

static std::ostream &operator<<(std::ostream &os, const EncPkg & /*encPkg*/) {
    os << "<EncPkg>"
       << "\n";
    return os;
}

static std::ostream &operator<<(std::ostream &os, const LinkProperties & /*properties*/) {
    os << "<LinkProperties>"
       << "\n";
    return os;
}

static std::ostream &operator<<(std::ostream &os, const SdkResponse & /*response*/) {
    os << "<SdkResponse>"
       << "\n";
    return os;
}

#endif
