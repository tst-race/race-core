
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

#include <memory>
#include <vector>

#include "../../../include/RaceConfig.h"
#include "../../../source/ArtifactManager.h"
#include "../../../source/ArtifactManagerWrapper.h"
#include "../../common/MockRacePluginArtifactManager.h"
#include "../../common/MockRaceSdk.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

class TestableArtifactManager : public ArtifactManager {
public:
    using ArtifactManager::ArtifactManager;

    MOCK_METHOD(bool, extractZip, (const std::string &zipFile, const std::string &baseDir),
                (const override));
};

class ArtifactManagerTest : public ::testing::Test {
public:
    MockRaceSdk sdk;
    std::shared_ptr<MockRacePluginArtifactManager> plugin0;
    std::shared_ptr<MockRacePluginArtifactManager> plugin1;
    std::unique_ptr<TestableArtifactManager> artifactManager;

    ArtifactManagerTest() :
        plugin0(std::make_shared<MockRacePluginArtifactManager>()),
        plugin1(std::make_shared<MockRacePluginArtifactManager>()) {
        std::vector<std::unique_ptr<ArtifactManagerWrapper>> wrappers;
        wrappers.push_back(std::make_unique<ArtifactManagerWrapper>(
            plugin0, "MockArtifactManager-0", "Mock ArtifactManager 0", sdk));
        wrappers.push_back(std::make_unique<ArtifactManagerWrapper>(
            plugin1, "MockArtifactManager-1", "Mock ArtifactManager 1", sdk));
        artifactManager = std::make_unique<TestableArtifactManager>(std::move(wrappers));
    }
};

////////////////////////////////////////////////////////////////
// init
////////////////////////////////////////////////////////////////

TEST_F(ArtifactManagerTest, init_invokes_all_plugins) {
    const AppConfig &config = sdk.getAppConfig();
    EXPECT_CALL(*plugin0,
                init(::testing::AllOf(
                    ::testing::Field(&PluginConfig::etcDirectory, config.etcDirectory),
                    ::testing::Field(&PluginConfig::loggingDirectory, config.logDirectory))))
        .WillOnce(::testing::Return(PLUGIN_OK));
    EXPECT_CALL(*plugin1,
                init(::testing::AllOf(
                    ::testing::Field(&PluginConfig::etcDirectory, config.etcDirectory),
                    ::testing::Field(&PluginConfig::loggingDirectory, config.logDirectory))))
        .WillOnce(::testing::Return(PLUGIN_OK));

    EXPECT_TRUE(artifactManager->init(config));
    std::vector<std::string> ids{"MockArtifactManager-0", "MockArtifactManager-1"};
    EXPECT_EQ(artifactManager->getIds(), ids);
}

TEST_F(ArtifactManagerTest, init_removes_plugins_that_fail_to_init) {
    const AppConfig &config = sdk.getAppConfig();
    EXPECT_CALL(*plugin0, init(::testing::_)).WillOnce(::testing::Return(PLUGIN_ERROR));
    EXPECT_CALL(*plugin1, init(::testing::_)).WillOnce(::testing::Return(PLUGIN_OK));

    EXPECT_TRUE(artifactManager->init(config));
    std::vector<std::string> ids{"MockArtifactManager-1"};
    EXPECT_EQ(artifactManager->getIds(), ids);
}

TEST_F(ArtifactManagerTest, init_fails_when_all_plugins_fail_to_init) {
    const AppConfig &config = sdk.getAppConfig();
    EXPECT_CALL(*plugin0, init(::testing::_)).WillOnce(::testing::Return(PLUGIN_ERROR));
    EXPECT_CALL(*plugin1, init(::testing::_)).WillOnce(::testing::Return(PLUGIN_ERROR));

    EXPECT_FALSE(artifactManager->init(config));
    std::vector<std::string> ids;
    EXPECT_EQ(artifactManager->getIds(), ids);
}

////////////////////////////////////////////////////////////////
// acquirePlugin
////////////////////////////////////////////////////////////////

TEST_F(ArtifactManagerTest, acquirePlugin_stops_when_first_plugin_succeeds) {
    EXPECT_CALL(*plugin0, acquireArtifact("/tmp/Linux-x86_64-client-thingy.zip",
                                          "Linux-x86_64-client-thingy.zip"))
        .WillOnce(::testing::Return(PLUGIN_OK));
    EXPECT_CALL(*plugin1, acquireArtifact(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*artifactManager, extractZip(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(true));

    EXPECT_TRUE(artifactManager->acquirePlugin("/tmp", "thingy", "Linux", "client", "x86_64"));
}

TEST_F(ArtifactManagerTest, acquirePlugin_attempts_to_try_all_plugins) {
    EXPECT_CALL(*plugin0, acquireArtifact(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(PLUGIN_ERROR));
    EXPECT_CALL(*plugin1, acquireArtifact("/tmp/Android-arm64-v8a-client-thingy.zip",
                                          "Android-arm64-v8a-client-thingy.zip"))
        .WillOnce(::testing::Return(PLUGIN_OK));
    EXPECT_CALL(*artifactManager, extractZip(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(true));

    EXPECT_TRUE(artifactManager->acquirePlugin("/tmp", "thingy", "Android", "client", "arm64-v8a"));
}

TEST_F(ArtifactManagerTest, acquirePlugin_fails_if_no_plugin_succeeds) {
    EXPECT_CALL(*plugin0, acquireArtifact(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(PLUGIN_ERROR));
    EXPECT_CALL(*plugin1, acquireArtifact(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(PLUGIN_ERROR));

    EXPECT_FALSE(artifactManager->acquirePlugin("/tmp", "thingy", "Linux", "server", "x86_64"));
}

TEST_F(ArtifactManagerTest, acquirePlugin_fails_if_zip_extraction_fails) {
    EXPECT_CALL(*plugin0, acquireArtifact("/tmp/Linux-x86_64-server-thingy.zip",
                                          "Linux-x86_64-server-thingy.zip"))
        .WillOnce(::testing::Return(PLUGIN_OK));
    EXPECT_CALL(*plugin1, acquireArtifact(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*artifactManager, extractZip("/tmp/Linux-x86_64-server-thingy.zip", "/tmp"))
        .WillOnce(::testing::Return(false));

    EXPECT_FALSE(artifactManager->acquirePlugin("/tmp", "thingy", "Linux", "server", "x86_64"));
}
