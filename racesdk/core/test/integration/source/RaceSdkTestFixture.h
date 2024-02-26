
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

#ifndef __TEST_INTEGRATION_SOURCE_RACE_SDK_TEST_FIXTURE_H__
#define __TEST_INTEGRATION_SOURCE_RACE_SDK_TEST_FIXTURE_H__

#include <sys/stat.h>

#include <exception>
#include <fstream>
#include <thread>

#include "../../../include/RaceConfig.h"
#include "../../../include/RaceSdk.h"
#include "../../common/MockPluginLoader.h"
#include "../../common/MockRaceApp.h"
#include "../../common/MockRacePluginArtifactManager.h"
#include "../../common/MockRacePluginComms.h"
#include "../../common/MockRacePluginNM.h"
#include "../../common/helpers.h"
#include "IRacePluginComms.h"
#include "gtest/gtest.h"

class RaceSdkTestFixture : public ::testing::Test {
public:
    RaceSdkTestFixture() :
        appConfig(createDefaultAppConfig()),
        raceConfig(createDefaultRaceConfig()),
        mockNM(std::make_shared<MockRacePluginNM>()),
        mockComms(std::make_shared<MockRacePluginComms>()),
        mockArtifactManager(std::make_shared<MockRacePluginArtifactManager>()),
        pluginLoader({mockNM}, {mockComms}, {mockArtifactManager}),
        sdk(appConfig, raceConfig, pluginLoader),
        mockApp(&sdk) {
        ::testing::DefaultValue<PluginResponse>::Set(PLUGIN_OK);
        createAppDirectories(sdk.getAppConfig());
    }
    virtual ~RaceSdkTestFixture() {}
    virtual void SetUp() override {}
    virtual void TearDown() override {}

protected:
public:
    AppConfig appConfig;
    RaceConfig raceConfig;
    std::shared_ptr<MockRacePluginNM> mockNM;
    std::shared_ptr<MockRacePluginComms> mockComms;
    std::shared_ptr<MockRacePluginArtifactManager> mockArtifactManager;
    MockPluginLoader pluginLoader;
    RaceSdk sdk;
    MockRaceApp mockApp;
    std::string sampleTraceID;
};

#endif
