
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

#ifndef PLUGIN_PATH
#pragma GCC error "Need plugin path from cmake"
#endif

// System
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// SDK Core
#include <PluginDef.h>
#include <PluginResponse.h>
#include <PythonLoaderWrapper.h>
#include <CommsWrapper.h>

// Test Helpers
#include <MockRaceSdk.h>

using ::testing::_;

class CommsPluginTest : public ::testing::Test {
public:
    CommsPluginTest() {
        PluginDef pluginDef;
        pluginDef.filePath = "stubs";
        pluginDef.type = RaceEnums::PluginType::PT_COMMS;
        pluginDef.fileType = RaceEnums::PluginFileType::PFT_PYTHON;
        pluginDef.nodeType = RaceEnums::NodeType::NT_ALL;
        pluginDef.pythonModule = "CommsStub.CommsStub";
        pluginDef.pythonClass = "PluginCommsTwoSixPy";

        plugin = std::make_unique<PythonLoaderWrapper<CommsWrapper>>(mockSdk, pluginDef);
    }

public:
    MockRaceSdk mockSdk;
    std::unique_ptr<PythonLoaderWrapper<CommsWrapper>> plugin;
};

TEST_F(CommsPluginTest, sdkFunctions) {
    RawData entropy = {0x01, 0x02};
    EXPECT_CALL(mockSdk, getEntropy(2)).WillOnce(::testing::Return(entropy));
    EXPECT_CALL(mockSdk, getActivePersona()).WillOnce(::testing::Return("expected-persona"));

    SdkResponse response;
    response.status = SDK_OK;
    response.handle = 0x1122334455667788;
    response.queueUtilization = 0.15;

    EXPECT_CALL(mockSdk, requestPluginUserInput(_, false, "expected-user-input-key",
                                                "expected-user-input-prompt", true))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, requestCommonUserInput(_, false, "expected-user-input-key"))
        .WillOnce(::testing::Return(response));

    EXPECT_CALL(mockSdk, displayInfoToUser(_, "expected-message", RaceEnums::UD_TOAST))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, displayBootstrapInfoToUser(_, "expected-message", RaceEnums::UD_QR_CODE,
                                                    RaceEnums::BS_COMPLETE))
        .WillOnce(::testing::Return(response));

    EXPECT_CALL(mockSdk, onPackageStatusChanged(_, 0x8877665544332211, PACKAGE_SENT, 1))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, onConnectionStatusChanged(_, 0x12345678, "expected-conn-id",
                                                   CONNECTION_CLOSED, _, 2))
        .WillOnce(::testing::Return(response));

    EXPECT_CALL(mockSdk,
                onLinkStatusChanged(_, 0x12345678, "expected-link-id", LINK_DESTROYED, _, 2))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, onChannelStatusChanged(_, 0x12345678, "expected-channel-gid",
                                                CHANNEL_AVAILABLE, _, 3))
        .WillOnce(::testing::Invoke(
            [response](auto &, auto, const auto &, auto, const auto &props, auto) {
                EXPECT_EQ("expected-channel-gid", props.channelGid);
                EXPECT_EQ(42, props.maxSendsPerInterval);
                EXPECT_EQ(3600, props.secondsPerInterval);
                EXPECT_EQ(8675309, props.intervalEndTime);
                EXPECT_EQ(7, props.sendsRemainingInInterval);
                return response;
            }));
    // .WillOnce(::testing::Return(response));

    EXPECT_CALL(mockSdk, updateLinkProperties(_, "expected-link-id", _, 4))
        .WillOnce(::testing::Return(response));
    EXPECT_CALL(mockSdk, generateConnectionId(_, "expected-link-id"))
        .WillOnce(::testing::Return("expected-conn-id"));
    EXPECT_CALL(mockSdk, generateLinkId(_, "expected-channel-gid"))
        .WillOnce(::testing::Return("expected-channel-gid/expected-link-id"));

    EncPkg pkg(0x0011223344556677, 0x2211331144115511, {0x08, 0x67, 0x53, 0x09});
    std::vector<std::string> connIds = {"expected-conn-id-1", "expected-conn-id-2"};
    EXPECT_CALL(mockSdk, receiveEncPkg(_, pkg, connIds, 5)).WillOnce(::testing::Return(response));

    // Can't test this one, it's a method on the CommsWrapper itself rather than RaceSdk
    // EXPECT_CALL(*plugin.get(), unblockQueue("expected-conn-id"))
    //     .WillOnce(::testing::Return(response));

    PluginConfig pluginConfig;
    pluginConfig.etcDirectory = "/expected/etc/path";
    pluginConfig.loggingDirectory = "/expected/logging/path";
    pluginConfig.auxDataDirectory = "/expected/auxData/path";
    pluginConfig.tmpDirectory = "/expected/tmp/path";
    ASSERT_EQ(PLUGIN_OK, plugin->init(pluginConfig));
}

// TODO test plugin functions
