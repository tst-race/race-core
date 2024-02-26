
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

#include "IRaceSdkTestApp.h"
#include "gmock/gmock.h"
#include "../../../core/source/filesystem.h"

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os, const AppConfig & /*config*/) {
    os << "<AppConfig>"
       << "\n";
    return os;
}

class MockRaceSdkApp : public IRaceSdkTestApp {
public:
    MOCK_METHOD(RawData, getEntropy, (std::uint32_t), (override));
    MOCK_METHOD(std::string, getActivePersona, (), (override));
    MOCK_METHOD(SdkResponse, asyncError, (RaceHandle handle, PluginResponse status), (override));
    MOCK_METHOD(ChannelProperties, getChannelProperties, (std::string), (override));
    MOCK_METHOD(std::vector<ChannelProperties>, getAllChannelProperties, (), (override));
    MOCK_METHOD(SdkResponse, makeDir, (const std::string &directoryPath), (override));
    MOCK_METHOD(SdkResponse, removeDir, (const std::string &directoryPath), (override));
    MOCK_METHOD(std::vector<std::string>, listDir, (const std::string &directoryPath), (override));
    MOCK_METHOD(std::vector<std::uint8_t>, readFile, (const std::string &filepath), (override));
    MOCK_METHOD(SdkResponse, appendFile,
                (const std::string &filepath, const std::vector<std::uint8_t> &data), (override));
    MOCK_METHOD(SdkResponse, writeFile,
                (const std::string &filepath, const std::vector<std::uint8_t> &data), (override));
    MOCK_METHOD(bool, initRaceSystem,
                (IRaceApp *app),
                (override));
    MOCK_METHOD(RaceHandle, sendClientMessage, (ClrMsg msg), (override));
    MOCK_METHOD(void, sendNMBypassMessage, (ClrMsg msg, const std::string &route), (override));
    MOCK_METHOD(bool, addVoaRules, (const nlohmann::json &payload), (override));
    MOCK_METHOD(bool, deleteVoaRules, (const nlohmann::json &payload), (override));
    MOCK_METHOD(void, setVoaActiveState, (bool state), (override));
    MOCK_METHOD(bool, setEnabledChannels, (const std::vector<std::string> &), (override));
    MOCK_METHOD(bool, enableChannel, (const std::string &), (override));
    MOCK_METHOD(bool, disableChannel, (const std::string &), (override));
    MOCK_METHOD(void, openNMBypassReceiveConnection,
                (const std::string &persona, const std::string &route), (override));
    MOCK_METHOD(void, rpcDeactivateChannel, (const std::string &channelGid), (override));
    MOCK_METHOD(void, rpcDestroyLink, (const std::string &linkId), (override));
    MOCK_METHOD(void, rpcCloseConnection, (const std::string &connectionId), (override));
    MOCK_METHOD(void, rpcNotifyEpoch, (const std::string &data), (override));
    MOCK_METHOD(std::vector<std::string>, getInitialEnabledChannels, (), (override));
    MOCK_METHOD(std::vector<std::string>, getContacts, (), (override));
    MOCK_METHOD(bool, isConnected, (), (override));
    MOCK_METHOD(RaceHandle, prepareToBootstrap, (DeviceInfo deviceInfo, std::string passphrase, std::string bootstrapChannelId),
                (override));
    MOCK_METHOD(SdkResponse, onUserInputReceived,
                (RaceHandle handle, bool answered, const std::string &response), (override));
    MOCK_METHOD(const AppConfig &, getAppConfig, (), (const override));
    MOCK_METHOD(void, cleanShutdown, (), (override));
    MOCK_METHOD(void, notifyShutdown, (int numSeconds), (override));
    MOCK_METHOD(SdkResponse, onUserAcknowledgementReceived, (RaceHandle handle), (override));
};

inline void replace_directory(std::string path) {
    fs::remove_all(path);
    fs::create_directories(path);
}

static AppConfig createDefaultAppConfig() {
    AppConfig config;

    // variables
    config.nodeType = RaceEnums::NodeType::NT_CLIENT;
    config.persona = "test persona";
    config.sdkFilePath = "sdk";

    // files
    config.jaegerConfigPath = "";
    config.userResponsesFilePath = "/tmp/test-files/userResponsesFilePath";
    config.logFilePath = "/tmp/test-files/logFilePath";

    // directories
    config.appDir = "/tmp/test-files/appDir";
    config.etcDirectory = "/tmp/test-files/etcDirectory";
    config.bootstrapFilesDirectory = "/tmp/test-files/bootstrapFilesDirectory";
    config.bootstrapCacheDirectory = "/tmp/test-files/bootstrapCacheDirectory";
    config.tmpDirectory = "/tmp/test-files/tmpDirectory";
    config.logDirectory = "/tmp/test-files/logDirectory";
    config.voaConfigPath = "/tmp/test-files/voaConfigPath";

    replace_directory(config.appDir);
    replace_directory(config.etcDirectory);
    replace_directory(config.bootstrapFilesDirectory);
    replace_directory(config.bootstrapCacheDirectory);
    replace_directory(config.tmpDirectory);
    replace_directory(config.logDirectory);
    replace_directory(config.voaConfigPath);
    return config;
}

class RaceTestAppSharedTestFixture : public ::testing::Test {
public:
    RaceTestAppSharedTestFixture() :
        config(createDefaultAppConfig()) {
            ON_CALL(mockSdk, getAppConfig()).WillByDefault(::testing::ReturnRef(config));
            ON_CALL(mockSdk, getActivePersona()).WillByDefault(::testing::Return("my-persona"));
    }
    virtual ~RaceTestAppSharedTestFixture() {}
    virtual void SetUp() override {}
    virtual void TearDown() override {}

public:
    AppConfig config;
    MockRaceSdkApp mockSdk;
};

#endif
