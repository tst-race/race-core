
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

// TODO: re-enable test.
TEST_F(RaceSdkTestFixture, DISABLED_receiveEncPkg_handles_threads) {
    const std::int32_t numCallsToreceiveEncPkg = 10;
    EXPECT_CALL(*mockNM, processEncPkg(::testing::_, ::testing::_, ::testing::_))
        .Times(numCallsToreceiveEncPkg);

    sdk.initRaceSystem(&mockApp);

    std::vector<std::thread> receiveEncPkgThreads;

    std::string testPackageContent =
        "000000000000004a0000000000000000ab7e9b1d0a33858bab7e9b1d0a33858b00000000000000000100001111"
        "11111111111111111111111111111111111111111111111111111111111111fhdjlshfjdksahfjdlsajfkldsaj"
        "kfldsajkf;djsakfjdsaklfjdksalfjkdslajfkdlsa;";
    const EncPkg pkg({testPackageContent.begin(), testPackageContent.end()});
    const std::vector<ConnectionID> connIDs = {""};
    for (std::int32_t i = 0; i < numCallsToreceiveEncPkg; ++i) {
        receiveEncPkgThreads.emplace_back(
            [&]() { sdk.getCommsWrapper("MockComms")->receiveEncPkg(pkg, connIDs, 0); });
    }

    for (auto &testThread : receiveEncPkgThreads) {
        testThread.join();
    }
}
