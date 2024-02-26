
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

#include <sys/stat.h>

#include <exception>
#include <fstream>
#include <thread>

#include "../../../include/RaceConfig.h"
#include "../../../include/RaceSdk.h"
#include "../../common/MockPluginLoader.h"
#include "../../common/MockRaceApp.h"
#include "../../common/MockRacePluginComms.h"
#include "../../common/MockRacePluginNM.h"
#include "IRacePluginComms.h"
#include "RaceSdkTestFixture.h"
#include "gtest/gtest.h"

using RaceSdkDeathTestFixture = RaceSdkTestFixture;

TEST_F(RaceSdkTestFixture, cleanShutdown_has_no_effect_before_init_is_called) {
    sdk.cleanShutdown();
}

TEST_F(RaceSdkTestFixture, cleanShutdown_calls_shutdown_on_plugins) {
    // networkManager shutdown should be called twice: once explicitly below in cleanShutdown and
    // once implicitly by RaceSdk destructor. comms shutdown checks to see that it hasn't been
    // shutdown before, so the same thing does not happen.
    EXPECT_CALL(*mockNM, shutdown).Times(2);
    EXPECT_CALL(*mockComms, shutdown).Times(1);

    sdk.initRaceSystem(&mockApp);
    sdk.cleanShutdown();
}

TEST_F(RaceSdkTestFixture, cleanShutdown_RaceSdk_dtor_calls_cleanShutdown) {
    EXPECT_CALL(*mockNM, shutdown).Times(1);
    EXPECT_CALL(*mockComms, shutdown).Times(1);

    sdk.initRaceSystem(&mockApp);
}

inline bool isStringInFile(const std::string &searchTerm, const std::string &fileName) {
    std::ifstream inputFile(fileName);
    std::string line;
    while (getline(inputFile, line)) {
        if (line.find(searchTerm) != std::string::npos) {
            return true;
        }
    }
    return false;
}

TEST_F(RaceSdkDeathTestFixture, cleanShutdown_times_out_for_hanging_network_manager_plugin) {
    // NOTE: due to known issues with death tests, EXPECT_CALL does not work as expected, so
    // don't use it because it will silently fail.

    ON_CALL(*mockNM, shutdown).WillByDefault([&]() {
        while (true) {
        }
        return PluginResponse();
    });

    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    EXPECT_DEATH(
        [&]() {
            RaceSdk sdkThatIsGoingToDie(appConfig, raceConfig, pluginLoader);
            MockRaceApp mockApp(&sdkThatIsGoingToDie);
            sdkThatIsGoingToDie.initRaceSystem(&mockApp);
            sdkThatIsGoingToDie.cleanShutdown();
        }(),
        "");

    // Admittedly lo-fi check to see if shutdown was called for both plugins. Unable to design
    // a better approach at the moment.
    EXPECT_TRUE(isStringInFile("Calling IRacePluginNM::shutdown()", appConfig.logFilePath))
        << "network manager plugin shutdown was not called";
    EXPECT_TRUE(isStringInFile("IRacePluginNM::shutdown() timed out, took longer than",
                               appConfig.logFilePath))
        << "error not logged for network manager plugin shutdown timeout";
    EXPECT_TRUE(isStringInFile("Calling IRacePluginComms::shutdown()", appConfig.logFilePath))
        << "comms plugin shutdown was not called";
}

TEST_F(RaceSdkDeathTestFixture, cleanShutdown_times_out_for_hanging_comms_plugin) {
    // NOTE: due to known issues with death tests, EXPECT_CALL does not work as expected, so
    // don't use it because it will silently fail.

    ON_CALL(*mockComms, shutdown).WillByDefault([&]() {
        while (true) {
        }
        return PluginResponse();
    });

    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    EXPECT_DEATH(
        [&]() {
            RaceSdk sdkThatIsGoingToDie(appConfig, raceConfig, pluginLoader);
            MockRaceApp mockApp(&sdkThatIsGoingToDie);
            sdkThatIsGoingToDie.initRaceSystem(&mockApp);
            sdkThatIsGoingToDie.cleanShutdown();
        }(),
        "");

    // Admittedly lo-fi check to see if shutdown was called for both plugins. Unable to design
    // a better approach at the moment.
    EXPECT_TRUE(isStringInFile("Calling IRacePluginNM::shutdown()", appConfig.logFilePath))
        << "network manager plugin shutdown was not called";
    EXPECT_TRUE(isStringInFile("Calling IRacePluginComms::shutdown()", appConfig.logFilePath))
        << "comms plugin shutdown was not called";
    EXPECT_TRUE(isStringInFile("IRacePluginComms::shutdown() timed out, took longer than",
                               appConfig.logFilePath))
        << "error not logged for network manager plugin shutdown timeout";
}

TEST_F(RaceSdkDeathTestFixture, cleanShutdown_times_out_for_hanging_plugins) {
    // NOTE: due to known issues with death tests, EXPECT_CALL does not work as expected (oh the
    // irony), so don't use it because it will silently fail.

    ON_CALL(*mockNM, shutdown).WillByDefault([&]() {
        while (true) {
        }
        return PluginResponse();
    });
    ON_CALL(*mockComms, shutdown).WillByDefault([&]() {
        while (true) {
        }
        return PluginResponse();
    });

    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    EXPECT_DEATH(
        [&]() {
            RaceSdk sdkThatIsGoingToDie(appConfig, raceConfig, pluginLoader);
            MockRaceApp mockApp(&sdkThatIsGoingToDie);
            sdkThatIsGoingToDie.initRaceSystem(&mockApp);
            sdkThatIsGoingToDie.cleanShutdown();
        }(),
        "");

    // Admittedly lo-fi check to see if shutdown was called for both plugins. Unable to design
    // a better approach at the moment.
    EXPECT_TRUE(isStringInFile("Calling IRacePluginNM::shutdown()", appConfig.logFilePath))
        << "network manager plugin shutdown was not called";
    EXPECT_TRUE(isStringInFile("IRacePluginNM::shutdown() timed out, took longer than",
                               appConfig.logFilePath))
        << "error not logged for network manager plugin shutdown timeout";
    EXPECT_TRUE(isStringInFile("Calling IRacePluginComms::shutdown()", appConfig.logFilePath))
        << "comms plugin shutdown was not called";
    EXPECT_TRUE(isStringInFile("IRacePluginComms::shutdown() timed out, took longer than",
                               appConfig.logFilePath))
        << "error not logged for network manager plugin shutdown timeout";
}
