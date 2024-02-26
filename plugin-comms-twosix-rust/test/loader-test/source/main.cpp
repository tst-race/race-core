
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

#include <IRacePluginComms.h>
#include <RaceLog.h>

#include <iostream>

#include "gmock/gmock.h"
#include "race/mocks/MockRaceSdkComms.h"

using ::testing::_;

std::int32_t main(std::int32_t argc, char **argv) {
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

class LoaderTest : public ::testing::Test {
public:
    MockRaceSdkComms sdk;

    LoaderTest() {
        ON_CALL(sdk, getActivePersona()).WillByDefault(::testing::Return("race-client-1"));
        ::testing::DefaultValue<SdkResponse>::Set(SDK_OK);

        ON_CALL(sdk, listDir(::testing::_))
            .WillByDefault(::testing::Return(std::vector<std::string>{"test_dir_1", "test_dir_2"}));
        ON_CALL(sdk, readFile(::testing::_))
            .WillByDefault(::testing::Return(std::vector<std::uint8_t>{'t', 'e', 's', 't'}));

        ON_CALL(sdk, generateConnectionId(::testing::_)).WillByDefault([this](LinkID linkId) {
            return linkId + "/ConnectionID-" + std::to_string(this->connectionCounter++);
        });
        ON_CALL(sdk, generateLinkId(::testing::_)).WillByDefault([this](std::string channelGid) {
            return "LinkID-" + channelGid + "-" + std::to_string(this->linkCounter++);
        });
    }

private:
    int linkCounter{0};
    int connectionCounter{0};
};

// TODO: break this test up into multiple test cases.
// TODO: move this into a separate file.
// TODO: make sure receiveEncPkg call also works
TEST_F(LoaderTest, test_it_all) {
    RaceLog::setLogLevel(RaceLog::LL_DEBUG);

    const auto plugin = createPluginComms(&sdk);

    {
        EXPECT_CALL(sdk, getActivePersona()).Times(1);

        EXPECT_CALL(sdk, writeFile("initialized.txt", _)).Times(1);
        EXPECT_CALL(sdk, readFile("initialized.txt")).Times(1);
        EXPECT_CALL(sdk, makeDir("testdir")).Times(1);
        EXPECT_CALL(sdk, removeDir("testdir")).Times(1);
        EXPECT_CALL(sdk, listDir("/code/")).Times(1);

        PluginConfig pluginConfig;
        auto response = plugin->init(pluginConfig);
        EXPECT_EQ(response, PLUGIN_OK);
        ::testing::Mock::VerifyAndClearExpectations(&sdk);
    }

    {
        SdkResponse hostnameResponse;
        hostnameResponse.handle = 7;
        hostnameResponse.status = SDK_OK;
        EXPECT_CALL(sdk, requestCommonUserInput("hostname"))
            .WillOnce(::testing::Return(hostnameResponse));

        SdkResponse startPortResponse;
        startPortResponse.handle = 8;
        startPortResponse.status = SDK_OK;
        EXPECT_CALL(sdk, requestPluginUserInput("startPort", ::testing::_, true))
            .WillOnce(::testing::Return(startPortResponse));

        EXPECT_CALL(sdk, onChannelStatusChanged(::testing::_, "twoSixDirectRust", CHANNEL_AVAILABLE,
                                                ::testing::_, ::testing::_));

        plugin->activateChannel(2, "twoSixDirectRust", "role");
        plugin->onUserInputReceived(hostnameResponse.handle, true, "race-server-00002");
        plugin->onUserInputReceived(startPortResponse.handle, true, "5000");

        ::testing::Mock::VerifyAndClearExpectations(&sdk);
    }

    // Set up send Link/Connection
    {
        RaceHandle handle = 3;
        // TODO: add expected calls into sdk
        plugin->loadLinkAddress(handle, "twoSixDirectRust",
                                "{\"hostname\":\"race-server-00001\",\"port\":5000}");
    }
    {
        RaceHandle handle = 64;
        EXPECT_CALL(sdk, generateConnectionId("LinkID-twoSixDirectRust-0")).Times(1);
        EXPECT_CALL(sdk,
                    onConnectionStatusChanged(handle, "LinkID-twoSixDirectRust-0/ConnectionID-0",
                                              CONNECTION_OPEN, _, _))
            .Times(1);
        plugin->openConnection(handle, LT_SEND, "LinkID-twoSixDirectRust-0", "{}", RACE_UNLIMITED);
        ::testing::Mock::VerifyAndClearExpectations(&sdk);
    }

    // Set up receive Link/Connection
    {
        RaceHandle handle = 5;
        // TODO: add expected calls into sdk
        // TODO: pass in a real link address
        plugin->createLinkFromAddress(handle, "twoSixDirectRust",
                                      "{\"port\": 5000, \"hostname\": \"hostname\"}");
    }
    // Commented out because this was causing valgrind failures because the receive thread
    // doesn't always shut down before the test completes. This can be re-enabled if we ever
    // implement reliable shutting down (i.e., joining) of the receive threads.
    // {
    //     RaceHandle handle = 64;
    //     EXPECT_CALL(sdk, generateConnectionId("LinkID-twoSixDirectRust-1")).Times(1);
    //     EXPECT_CALL(sdk,
    //                 onConnectionStatusChanged(handle, "LinkID-twoSixDirectRust-1/ConnectionID-1",
    //                                           CONNECTION_OPEN, _, _))
    //         .Times(1);
    //     plugin->openConnection(handle, LT_RECV, "LinkID-twoSixDirectRust-1", "{}",
    //     RACE_UNLIMITED);
    //     ::testing::Mock::VerifyAndClearExpectations(&sdk);
    // }

    // Send on send connection
    {
        RaceHandle handle = 128;
        const std::string connectionId = "LinkID-twoSixDirectRust-0/ConnectionID-0";
        const std::string cipherText = "some package from my C++ test";
        EncPkg pkg(0, 0, {cipherText.begin(), cipherText.end()});
        EXPECT_CALL(sdk, onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, _)).Times(1);
        plugin->sendPackage(handle, connectionId, pkg, 1, 0);
        ::testing::Mock::VerifyAndClearExpectations(&sdk);
    }

    {
        RaceHandle handle = 6;
        plugin->deactivateChannel(handle, "twoSixIndirectRust");
    }

    {
        RaceHandle handle = 7;
        plugin->deactivateChannel(handle, "twoSixDirectRust");
    }

    destroyPluginComms(plugin);

    std::cout << "test main done" << std::endl;
}
