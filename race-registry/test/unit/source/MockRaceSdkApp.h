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


#pragma once

#include "IRaceSdkApp.h"
#include "gmock/gmock.h"

class MockRaceSdkApp : public IRaceSdkApp {
public:
    MockRaceSdkApp() {
        AppConfig config;
        config.persona = "race-client-00001";
        config.etcDirectory = "/etc/race";
        config.configTarPath = "/tmp/configs.tar.gz";
        config.baseConfigPath = "/data/configs";
        config.jaegerConfigPath = config.etcDirectory + "/jaeger-config.yml";
        config.userResponsesFilePath = config.etcDirectory + "/user-responses.json";
        config.encryptionType = RaceEnums::ENC_NONE;
        ON_CALL(*this, getAppConfig()).WillByDefault(testing::ReturnRef(appConfig));
    }
    MOCK_METHOD(const AppConfig &, getAppConfig, (), (const, override));
    MOCK_METHOD(bool, initRaceSystem, (IRaceApp *app), (override));
    MOCK_METHOD(RaceHandle, prepareToBootstrap, (DeviceInfo deviceInfo, std::string passphrase, std::string bootstrapChannelId), (override));
    MOCK_METHOD(SdkResponse, onUserInputReceived, (RaceHandle handle, bool answered, const std::string &response), (override));
    MOCK_METHOD(SdkResponse, onUserAcknowledgementReceived, (RaceHandle handle), (override));
    MOCK_METHOD(RaceHandle, sendClientMessage, (ClrMsg msg), (override));
    MOCK_METHOD(bool, addVoaRules, (const nlohmann::json &payload), (override));
    MOCK_METHOD(bool, deleteVoaRules, (const nlohmann::json &payload), (override));
    MOCK_METHOD(void, setVoaActiveState, (bool state), (override));
    MOCK_METHOD(bool, setEnabledChannels, (const std::vector<std::string> &), (override));
    MOCK_METHOD(bool, enableChannel, (const std::string &), (override));
    MOCK_METHOD(bool, disableChannel, (const std::string &), (override));
    MOCK_METHOD(std::vector<std::string>, getContacts, (), (override));
    MOCK_METHOD(bool, isConnected, (), (override));
    MOCK_METHOD(void, cleanShutdown, (), (override));
    MOCK_METHOD(void, notifyShutdown, (std::int32_t numSeconds), (override));

    MOCK_METHOD(RawData, getEntropy, (std::uint32_t), (override));
    MOCK_METHOD(std::string, getActivePersona, (), (override));
    MOCK_METHOD(ChannelProperties, getChannelProperties, (std::string), (override));
    MOCK_METHOD(std::vector<ChannelProperties>, getAllChannelProperties, (), (override));
    MOCK_METHOD(SdkResponse, asyncError, (RaceHandle, PluginResponse), (override));
    MOCK_METHOD(std::vector<std::string>, listDir, (const std::string &dirpath), (override));
    MOCK_METHOD(SdkResponse, makeDir, (const std::string &dirpath), (override));
    MOCK_METHOD(SdkResponse, removeDir, (const std::string &dirpath), (override));
    MOCK_METHOD(std::vector<std::uint8_t>, readFile, (const std::string &filename), (override));
    MOCK_METHOD(SdkResponse, appendFile,
                (const std::string &filepath, const std::vector<std::uint8_t> &data), (override));
    MOCK_METHOD(SdkResponse, writeFile,
                (const std::string &filepath, const std::vector<std::uint8_t> &data), (override));
public:
    AppConfig appConfig;
};
