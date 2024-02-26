
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
#include <IRaceSdkComms.h>
#include <RaceLog.h>
#include <libgen.h>        // dirname
#include <linux/limits.h>  // PATH_MAX
#include <race/mocks/MockRaceSdkComms.h>
#include <string.h>  // memset
#include <unistd.h>  // readlink

#include <iostream>

#include "gmock/gmock.h"

std::int32_t main(std::int32_t argc, char **argv) {
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

// TODO: break this test up into multiple test cases.
// TODO: add more assertions.
// TODO: move this into a separate file.
TEST(loader, test_it_all) {
    RaceLog::setLogLevel(RaceLog::LL_DEBUG);

    std::cout << "running main" << std::endl;

    MockRaceSdkComms sdk;
    ON_CALL(sdk, getActivePersona()).WillByDefault(::testing::Return("race-client-1"));
    ::testing::DefaultValue<SdkResponse>::Set(SDK_OK);

    const auto plugin = createPluginComms(&sdk);

    // Attempt the get the path of the executable.
    // TODO: WARNING: Linux specific code.
    // {
    //     char result[PATH_MAX];
    //     memset(&result, 0, sizeof(result));
    //     ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    //     if (count != -1) {
    //         configPath = std::string(dirname(result)) + "/" + configPath;
    //     }
    // }

    PluginConfig pluginConfig;
    ASSERT_EQ(PLUGIN_OK, plugin->init(pluginConfig));
    ASSERT_EQ(PLUGIN_OK, plugin->shutdown());

    delete plugin;

    std::cout << "test main done" << std::endl;
}
