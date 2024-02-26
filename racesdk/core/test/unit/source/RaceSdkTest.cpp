
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

#include <exception>
#include <memory>

#include "../../../include/PluginLoader.h"
#include "../../../include/RaceConfig.h"
#include "../../../include/RaceSdk.h"
#include "../../../source/AppWrapper.h"
#include "../../../source/BootstrapThread.h"
#include "../../../source/CommsWrapper.h"
#include "../../../source/NMWrapper.h"
#include "../../../source/filesystem.h"
#include "../../common/MockArtifactManager.h"
#include "../../common/MockBootstrapManager.h"
#include "../../common/MockPluginLoader.h"
#include "../../common/MockRaceApp.h"
#include "../../common/MockRaceBootstrapPluginComms.h"
#include "../../common/MockRacePluginArtifactManager.h"
#include "../../common/MockRacePluginComms.h"
#include "../../common/MockRacePluginNM.h"
#include "../../common/MockRaceSdk.h"
#include "../../common/helpers.h"
#include "../../common/race_printers.h"
#include "IRacePluginComms.h"
#include "gtest/gtest.h"

using namespace std::chrono_literals;
using ::testing::_;

class TestableRaceChannels : public RaceChannels {
public:
    using RaceChannels::RaceChannels;

    bool isUserEnabled(const std::string &) override {
        return true;
    }
};

class TestableRaceSdk : public RaceSdk {
public:
    TestableRaceSdk(const AppConfig &_appConfig, const RaceConfig &_raceConfig,
                    IPluginLoader &_pluginLoader,
                    std::shared_ptr<FileSystemHelper> _fileSystemHelper) :
        RaceSdk(_appConfig, _raceConfig, _pluginLoader, _fileSystemHelper) {
        // Replace the RaceChannels object
        channels = std::make_unique<TestableRaceChannels>(_raceConfig.channels, this);
        initializeRaceChannels();
    }

    void setApp(IRaceApp *_app) {
        appWrapper = std::make_unique<AppWrapper>(_app, *this);
        appWrapper->startHandler();
    }

    void setArtifactManager(ArtifactManager *_artifactManager) {
        artifactManager.reset(_artifactManager);
    }

    BootstrapThread *getBootstrapThread() {
        return bootstrapManager.getBootstrapThread();
    }

    void waitForCallbacks() {
        try {
            CommsWrapper *comms = getCommsWrapper("MockComms-0");
            if (comms) {
                comms->waitForCallbacks();
            }
        } catch (...) {
        }
        try {
            NMWrapper *networkManager = getNM();
            if (networkManager) {
                networkManager->waitForCallbacks();
            }

        } catch (...) {
        }
        if (appWrapper) {
            appWrapper->waitForCallbacks();
        }
    }

    using RaceSdk::setChannelEnabled;  // allow public access for test only
};

class MockBsManTestableSdk : public TestableRaceSdk {
public:
    MockBsManTestableSdk(const AppConfig &_appConfig, const RaceConfig &_raceConfig,
                         IPluginLoader &_pluginLoader,
                         std::shared_ptr<FileSystemHelper> _fileSystemHelper) :
        TestableRaceSdk(_appConfig, _raceConfig, _pluginLoader, _fileSystemHelper),
        test_info(testing::UnitTest::GetInstance()->current_test_info()),
        logger(test_info->test_suite_name(), test_info->name()),
        mockBootstrapManager(logger, *this, _fileSystemHelper) {}

    virtual BootstrapManager &getBootstrapManager() override {
        return mockBootstrapManager;
    }

    const testing::TestInfo *test_info;
    LogExpect logger;
    MockBootstrapManager mockBootstrapManager;
};

RaceConfig createTestFixtureRaceConfig() {
    RaceConfig config;
    config.androidPythonPath = "";
    // config.plugins;
    // config.enabledChannels;
    config.isPluginFetchOnStartEnabled = true;
    config.isVoaEnabled = true;
    config.wrapperQueueMaxSize = 1000000;
    config.wrapperTotalMaxSize = 1000000000;
    config.logLevel = RaceLog::LL_DEBUG;
    // config.logLevelStdout = RaceLog::LL_DEBUG;
    config.logRaceConfig = false;
    config.logNMConfig = false;
    config.logCommsConfig = false;
    config.msgLogLength = 256;

    ChannelProperties channelProperties;
    channelProperties.channelStatus = CHANNEL_ENABLED;
    channelProperties.channelGid = "MockComms-0/channel1";

    ChannelRole role;
    role.roleName = "role";
    role.linkSide = LS_BOTH;
    channelProperties.roles = {role};

    ChannelProperties bootstrapChannelProperties;
    bootstrapChannelProperties.channelStatus = CHANNEL_ENABLED;
    bootstrapChannelProperties.channelGid = "MockComms-0/channel2";
    bootstrapChannelProperties.connectionType = CT_LOCAL;
    bootstrapChannelProperties.bootstrap = true;

    ChannelRole bootstrapRole;
    bootstrapRole.roleName = "bootstrap-role";
    bootstrapRole.linkSide = LS_BOTH;
    bootstrapChannelProperties.roles = {bootstrapRole};

    config.channels = {channelProperties, bootstrapChannelProperties};

    PluginDef networkManagerPluginDef;
    PluginDef commsPluginDef1;
    PluginDef commsPluginDef2;
    PluginDef ampPluginDef;

    commsPluginDef1.filePath = "MockComms-0";
    commsPluginDef1.channels = {channelProperties.channelGid};

    commsPluginDef2.filePath = "MockComms-0";
    commsPluginDef2.channels = {bootstrapChannelProperties.channelGid};

    config.plugins[RaceEnums::PluginType::PT_NM] = {networkManagerPluginDef};
    config.plugins[RaceEnums::PluginType::PT_COMMS] = {commsPluginDef1, commsPluginDef2};

    config.plugins[RaceEnums::PluginType::PT_ARTIFACT_MANAGER] = {ampPluginDef};

    config.environmentTags = {{"", {}}, {"phone", {}}};

    return config;
}

class RaceSdkTestFixture : public ::testing::Test {
public:
    RaceSdkTestFixture() :
        appConfig(createDefaultAppConfig()),
        raceConfig(createTestFixtureRaceConfig()),
        mockNM(std::make_shared<MockRacePluginNM>()),
        mockComms(std::make_shared<MockRacePluginComms>()),
        mockArtifactManagerPlugin(std::make_shared<MockRacePluginArtifactManager>()),
        pluginLoader({mockNM}, {mockComms}, {mockArtifactManagerPlugin}),
        sdk(appConfig, raceConfig, pluginLoader, std::make_shared<MockFileSystemHelper>()),
        mockApp(&sdk) {
        sdk.setApp(&mockApp);
        ::testing::DefaultValue<PluginResponse>::Set(PLUGIN_OK);
        createAppDirectories(appConfig);
    }
    virtual ~RaceSdkTestFixture() {}
    virtual void SetUp() override {
        initialize();
    }
    virtual void TearDown() override {
        sdk.waitForCallbacks();
        sdk.cleanShutdown();
    }

    void initialize() {
        sdk.initRaceSystem(&mockApp);

        channelProperties = raceConfig.channels[0];
        channelGid = channelProperties.channelGid;
        bootstrapChannelProperties = raceConfig.channels[1];
        bootstrapChannelGid = bootstrapChannelProperties.channelGid;

        sdk.getNM()->activateChannel(channelGid, "role", RACE_BLOCKING);
        sdk.getNM()->activateChannel(bootstrapChannelGid, "bootstrap-role", RACE_BLOCKING);

        sdk.getCommsWrapper("MockComms-0")
            ->onChannelStatusChanged(0, channelGid, CHANNEL_AVAILABLE, channelProperties, 0);

        sdk.getCommsWrapper("MockComms-0")
            ->onChannelStatusChanged(0, bootstrapChannelGid, CHANNEL_AVAILABLE,
                                     bootstrapChannelProperties, 0);
    }

public:
    AppConfig appConfig;
    RaceConfig raceConfig;

    std::string channelGid;
    std::string bootstrapChannelGid;
    ChannelProperties channelProperties;
    ChannelProperties bootstrapChannelProperties;

    std::shared_ptr<MockRacePluginNM> mockNM;
    std::shared_ptr<MockRacePluginComms> mockComms;
    std::shared_ptr<MockRacePluginArtifactManager> mockArtifactManagerPlugin;
    MockPluginLoader pluginLoader;
    TestableRaceSdk sdk;
    MockRaceApp mockApp;
};

//
// Helpers
//

inline LinkID createLinkForTesting(MockRacePluginComms *mockComms, TestableRaceSdk *sdk,
                                   std::string commsPluginId, std::string channel,
                                   std::vector<std::string> personas) {
    CommsWrapper *commsSdk = sdk->getCommsWrapper(commsPluginId);
    LinkID linkIdToReturn;
    ON_CALL(*mockComms, createLink(_, _))
        .WillByDefault(testing::Invoke([commsSdk, &linkIdToReturn](RaceHandle handle,
                                                                   std::string channelGid) {
            LinkID linkId = commsSdk->generateLinkId(channelGid);
            linkIdToReturn = linkId;
            LinkProperties linkProps;
            linkProps.linkType = LT_BIDI;
            linkProps.transmissionType = TT_MULTICAST;
            linkProps.connectionType = CT_INDIRECT;
            linkProps.sendType = ST_STORED_ASYNC;
            linkProps.channelGid = channelGid;
            commsSdk->onLinkStatusChanged(handle, linkId, LINK_CREATED, linkProps, RACE_BLOCKING);
            return PLUGIN_OK;
        }));

    sdk->getNM()->createLink(channel, personas, 0);
    sdk->getNM()->waitForCallbacks();
    sdk->getCommsWrapper(commsPluginId)->waitForCallbacks();

    return linkIdToReturn;
}

// getEntropy

TEST_F(RaceSdkTestFixture, getEntropy_returns_correct_size) {
    const std::uint32_t entropySize = 64u;
    RawData entropy = sdk.getEntropy(entropySize);
    ASSERT_EQ(entropy.size(), static_cast<size_t>(entropySize));
}

////////////////////////////////////////////////////////////////
// initRaceSystem
////////////////////////////////////////////////////////////////

class RaceSdkInitTestFixture : public RaceSdkTestFixture {
public:
    virtual void SetUp() override {}
};

/**
 * @brief initRaceSystem should call init and start (in that order) exactly once for network manager
 * and comms plugin.
 *
 */
TEST_F(RaceSdkInitTestFixture, initRaceSystem_calls_plugin_init) {
    PluginConfig pluginConfigNM;
    pluginConfigNM.etcDirectory = appConfig.etcDirectory;
    pluginConfigNM.loggingDirectory = appConfig.logDirectory;
    pluginConfigNM.auxDataDirectory =
        appConfig.pluginArtifactsBaseDir + "/network-manager/MockNM-0/aux-data";
    pluginConfigNM.tmpDirectory = appConfig.tmpDirectory + "/MockNM-0";
    pluginConfigNM.pluginDirectory = appConfig.pluginArtifactsBaseDir + "/network-manager/MockNM-0";

    PluginConfig pluginConfigComms;
    pluginConfigComms.etcDirectory = appConfig.etcDirectory;
    pluginConfigComms.loggingDirectory = appConfig.logDirectory;
    pluginConfigComms.auxDataDirectory =
        appConfig.pluginArtifactsBaseDir + "/comms/MockComms-0/aux-data";
    pluginConfigComms.tmpDirectory = appConfig.tmpDirectory + "/MockComms-0";
    pluginConfigComms.pluginDirectory = appConfig.pluginArtifactsBaseDir + "/comms/MockComms-0";

    EXPECT_CALL(*mockNM, init(pluginConfigNM)).Times(1).WillOnce(::testing::Return(PLUGIN_OK));

    EXPECT_CALL(*mockComms, init(pluginConfigComms))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_OK));

    EXPECT_TRUE(sdk.initRaceSystem(&mockApp));
}

TEST_F(RaceSdkInitTestFixture, initRaceSystem_network_manager_init_fails) {
    PluginConfig pluginConfigNM;
    pluginConfigNM.etcDirectory = appConfig.etcDirectory;
    pluginConfigNM.loggingDirectory = appConfig.logDirectory;
    pluginConfigNM.auxDataDirectory =
        appConfig.pluginArtifactsBaseDir + "/network-manager/MockNM-0/aux-data";
    pluginConfigNM.tmpDirectory = appConfig.tmpDirectory + "/MockNM-0";
    pluginConfigNM.pluginDirectory = appConfig.pluginArtifactsBaseDir + "/network-manager/MockNM-0";

    PluginConfig pluginConfigComms;
    pluginConfigComms.etcDirectory = appConfig.etcDirectory;
    pluginConfigComms.loggingDirectory = appConfig.logDirectory;
    pluginConfigComms.auxDataDirectory =
        appConfig.pluginArtifactsBaseDir + "/comms/MockComms-0/aux-data";
    pluginConfigComms.tmpDirectory = appConfig.tmpDirectory + "/MockComms-0";
    pluginConfigComms.pluginDirectory = appConfig.pluginArtifactsBaseDir + "/comms/MockComms-0";

    EXPECT_CALL(*mockNM, init(pluginConfigNM)).Times(1).WillOnce(::testing::Return(PLUGIN_FATAL));

    EXPECT_CALL(*mockComms, init(pluginConfigComms)).Times(0);

    ASSERT_THROW(sdk.initRaceSystem(&mockApp), std::runtime_error);
}

TEST_F(RaceSdkInitTestFixture, initRaceSystem_all_comms_init_fail_fails) {
    PluginConfig pluginConfigNM;
    pluginConfigNM.etcDirectory = appConfig.etcDirectory;
    pluginConfigNM.loggingDirectory = appConfig.logDirectory;
    pluginConfigNM.auxDataDirectory =
        appConfig.pluginArtifactsBaseDir + "/network-manager/MockNM-0/aux-data";
    pluginConfigNM.tmpDirectory = appConfig.tmpDirectory + "/MockNM-0";
    pluginConfigNM.pluginDirectory = appConfig.pluginArtifactsBaseDir + "/network-manager/MockNM-0";

    PluginConfig pluginConfigComms;
    pluginConfigComms.etcDirectory = appConfig.etcDirectory;
    pluginConfigComms.loggingDirectory = appConfig.logDirectory;
    pluginConfigComms.auxDataDirectory =
        appConfig.pluginArtifactsBaseDir + "/comms/MockComms-0/aux-data";
    pluginConfigComms.tmpDirectory = appConfig.tmpDirectory + "/MockComms-0";
    pluginConfigComms.pluginDirectory = appConfig.pluginArtifactsBaseDir + "/comms/MockComms-0";

    EXPECT_CALL(*mockNM, init(pluginConfigNM)).Times(1).WillOnce(::testing::Return(PLUGIN_OK));

    EXPECT_CALL(*mockComms, init(pluginConfigComms))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));

    ASSERT_THROW(sdk.initRaceSystem(&mockApp), std::runtime_error);
}

TEST_F(RaceSdkInitTestFixture, init_fail_stops_plugin) {
    EXPECT_CALL(*mockComms, init(::testing::_)).Times(1).WillOnce(::testing::Return(PLUGIN_FATAL));

    ASSERT_THROW(sdk.initRaceSystem(&mockApp), std::runtime_error);
}

TEST_F(RaceSdkInitTestFixture, init_defaults_to_empty_if_user_input_not_received) {
    sdk.initRaceSystem(&mockApp);
    EXPECT_EQ(sdk.getRaceConfig().env, "");
}

TEST_F(RaceSdkInitTestFixture, init_receives_env_value_from_user_input) {
    EXPECT_CALL(mockApp, requestUserInput(_, _, "env", _, _))
        .WillOnce([this](RaceHandle handle, const std::string & /*pluginId*/,
                         const std::string & /*key*/, const std::string & /*prompt*/,
                         bool /*cache*/) {
            std::thread([handle, this]() {
                sdk.onUserInputReceived(handle, true, "Phone");
            }).detach();
            return SdkResponse{SDK_OK};
        });
    sdk.initRaceSystem(&mockApp);

    // should be lower case
    EXPECT_EQ(sdk.getRaceConfig().env, "phone");
}

TEST_F(RaceSdkInitTestFixture, init_invalid_env_return_false) {
    EXPECT_CALL(mockApp, requestUserInput(_, _, "env", _, _))
        .WillOnce([this](RaceHandle handle, const std::string & /*pluginId*/,
                         const std::string & /*key*/, const std::string & /*prompt*/,
                         bool /*cache*/) {
            std::thread([handle, this]() {
                sdk.onUserInputReceived(handle, true, "Invalid");
            }).detach();
            return SdkResponse{SDK_OK};
        });

    EXPECT_EQ(sdk.initRaceSystem(&mockApp), false);
}

////////////////////////////////////////////////////////////////
// getLinkProperties
////////////////////////////////////////////////////////////////

static void compareLinkPropertySet(const LinkPropertySet &a, const LinkPropertySet &b) {
    EXPECT_EQ(a.bandwidth_bps, b.bandwidth_bps);
    EXPECT_EQ(a.latency_ms, b.latency_ms);
    EXPECT_EQ(a.loss, b.loss);
}

static void compareLinkPropertyPair(const LinkPropertyPair &a, const LinkPropertyPair &b) {
    compareLinkPropertySet(a.send, b.send);
    compareLinkPropertySet(a.receive, b.receive);
}

static void compareLinkProperties(const LinkProperties &a, const LinkProperties &b) {
    EXPECT_EQ(a.linkType, b.linkType);
    compareLinkPropertyPair(a.worst, b.worst);
    compareLinkPropertyPair(a.expected, b.expected);
    compareLinkPropertyPair(a.best, b.best);
    EXPECT_EQ(a.duration_s, b.duration_s);
    EXPECT_EQ(a.period_s, b.period_s);
    EXPECT_EQ(a.reliable, b.reliable);
    EXPECT_EQ(a.mtu, b.mtu);
}

/**
 * @brief If the LinkID provided is not recognized then an exception is thrown.
 *
 */
TEST_F(RaceSdkTestFixture, getLinkProperties_does_not_throw_an_error_if_LinkID_is_invalid) {
    ASSERT_NO_THROW(sdk.getLinkProperties("some LinkID that is not cached"));
}

/**
 * @brief getLinkProperties should return any LinkProperties that match the provided LinkType and
 * LinkID. The test pre-populates RaceSdk with a link profile and link properties which
 * getLinkProperties can then query.
 *
 */
TEST_F(RaceSdkTestFixture, getLinkProperties_returns_the_added_link_properties) {
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {});

    LinkProperties propertiesToAdd = getDefaultLinkProperties();
    propertiesToAdd.linkType = LT_SEND;
    sdk.getCommsWrapper("MockComms-0")->updateLinkProperties(linkId, propertiesToAdd, 0);

    const LinkProperties linkProps = sdk.getLinkProperties(linkId);

    compareLinkProperties(linkProps, propertiesToAdd);
}

////////////////////////////////////////////////////////////////
// getContacts
////////////////////////////////////////////////////////////////

/**
 * @brief getContacts should return all the contacts added by network manager via calls to
 * createLink.
 *
 */
TEST_F(RaceSdkTestFixture, getContacts_returns_all_contacts) {
    std::vector<std::string> personas = {"persona 1", "persona 2", "persona 3", "persona 4",
                                         "persona 5"};

    createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, personas);

    std::vector<std::string> contacts = sdk.getContacts();

    ASSERT_EQ(personas.size(), contacts.size());
    for (const auto &persona : personas) {
        EXPECT_TRUE(std::find(contacts.begin(), contacts.end(), persona) != contacts.end())
            << "Failed to find persona: " << persona;
    }
}

/**
 * @brief getContacts should only return unique contacts. A persona may be available on multiple
 * links, but should only be reported to the client once.
 *
 */
TEST_F(RaceSdkTestFixture, getContacts_only_returns_unique_contacts) {
    {
        std::vector<std::string> personas = {"persona 1", "persona 2", "persona 3", "persona 4",
                                             "persona 5"};
        const LinkID linkId =
            createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, personas);
    }

    {
        std::vector<std::string> personas = {"persona 1", "persona 3", "persona 5", "persona 10",
                                             "persona 12"};
        const LinkID linkId =
            createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, personas);
    }

    const std::vector<std::string> contacts = sdk.getContacts();

    ASSERT_EQ(contacts.size(), 7);
    auto contactsHasPersona = [&contacts](const std::string &persona) {
        EXPECT_TRUE(std::find(contacts.begin(), contacts.end(), persona) != contacts.end());
    };
    contactsHasPersona("persona 1");
    contactsHasPersona("persona 2");
    contactsHasPersona("persona 3");
    contactsHasPersona("persona 4");
    contactsHasPersona("persona 5");
    contactsHasPersona("persona 10");
    contactsHasPersona("persona 12");
}

////////////////////////////////////////////////////////////////
// sendEncryptedPackage
////////////////////////////////////////////////////////////////

/**
 * @brief sendEncryptedPackage should throw an exception if comms plugin is not set.
 *
 */
TEST_F(RaceSdkTestFixture, sendEncryptedPackage_returns_error_comms_not_set) {
    const uint64_t batchId = 0;
    ASSERT_EQ(
        sdk.sendEncryptedPackage(*sdk.getNM(), EncPkg({}), "MockComms/Invalid", batchId, 0).status,
        SDK_INVALID_ARGUMENT);
    sdk.cleanShutdown();
}

/**
 * @brief sendEncryptedPackage should simply forward the package and connection ID to comms plugin.
 *
 */
TEST_F(RaceSdkTestFixture, sendEncryptedPackage_should_call_comms) {
    const std::string cipherText = "my cipher text";
    EncPkg packageToSend(0, 0, {cipherText.begin(), cipherText.end()});
    RaceHandle handle = 0;

    auto packageValidator = [=](EncPkg pkg) -> int {
        return pkg.getCipherText() == packageToSend.getCipherText() ? 1 : 0;
    };
    auto handleValidator = [&handle](RaceHandle commsHandle) -> int {
        handle = commsHandle;
        return 1;
    };

    // create a dummy connection
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {""});
    createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"my persona"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0).handle;
    const ConnectionID connectionId =
        sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    EXPECT_CALL(*mockComms,
                sendPackage(::testing::Truly(handleValidator), connectionId,
                            ::testing::Truly(packageValidator), ::testing::_, ::testing::_))
        .Times(1);
    const uint64_t batchId = 0;
    auto sdkResponse =
        sdk.sendEncryptedPackage(*sdk.getNM(), packageToSend, connectionId, batchId, 0);

    // make sure the plugins get the call before the expect
    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
    sdk.cleanShutdown();

    EXPECT_EQ(sdkResponse.status, SDK_OK);
    EXPECT_EQ(handle, sdkResponse.handle);
}

/**
 * @brief sendEncryptedPackage should return queue full if the package is small enough to fit in the
 * queue, but too large to fit in the remaining space.
 *
 */
TEST_F(RaceSdkTestFixture, sendEncryptedPackage_returns_queue_full_on_too_large_package) {
    const std::string cipherText(raceConfig.wrapperQueueMaxSize / 2 + 1, 'a');
    EncPkg packageToSend(0, 0, {cipherText.begin(), cipherText.end()});

    // create a dummy connection
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"my persona"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0).handle;
    const LinkID connectionId = sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    std::promise<void> promise;
    auto future = promise.get_future();
    EXPECT_CALL(*mockComms,
                sendPackage(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce([&](RaceHandle, ConnectionID, EncPkg, double, uint64_t) {
            future.wait();
            return PLUGIN_OK;
        });

    const uint64_t batchId = 0;
    auto sdkResponse1 =
        sdk.sendEncryptedPackage(*sdk.getNM(), packageToSend, connectionId, batchId, 0);
    auto sdkResponse2 =
        sdk.sendEncryptedPackage(*sdk.getNM(), packageToSend, connectionId, batchId, 0);
    promise.set_value();

    // make sure the plugins get the call before the expect
    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
    sdk.cleanShutdown();

    EXPECT_EQ(sdkResponse1.status, SDK_OK);
    EXPECT_EQ(sdkResponse2.status, SDK_QUEUE_FULL);
}

/**
 * @brief sendEncryptedPackage timeout should cause sendPackage to wait for space to become
 * available
 *
 */
TEST_F(RaceSdkTestFixture, sendEncryptedPackage_timeout_waits) {
    using namespace std::chrono_literals;

    const std::string cipherText(raceConfig.wrapperQueueMaxSize / 2 + 1, 'a');
    EncPkg packageToSend(0, 0, {cipherText.begin(), cipherText.end()});
    // create a dummy connection
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"my persona"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0).handle;
    const LinkID connectionId = sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    EXPECT_CALL(*mockComms,
                sendPackage(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(2)
        .WillOnce([&](RaceHandle, ConnectionID, EncPkg, double, uint64_t) {
            std::this_thread::sleep_for(10ms);
            return PLUGIN_OK;
        });
    const uint64_t batchId = 0;
    auto sdkResponse1 =
        sdk.sendEncryptedPackage(*sdk.getNM(), packageToSend, connectionId, batchId, 0);
    auto sdkResponse2 =
        sdk.sendEncryptedPackage(*sdk.getNM(), packageToSend, connectionId, batchId, 10000);

    // make sure the plugins get the call before the expect
    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
    sdk.cleanShutdown();

    EXPECT_EQ(sdkResponse1.status, SDK_OK);
    EXPECT_EQ(sdkResponse2.status, SDK_OK);
}

/**
 * @brief sendEncryptedPackage should return invalid argument if the package does not fit into an
 * empty queue
 *
 */
TEST_F(RaceSdkTestFixture,
       sendEncryptedPackage_returns_invalid_argument_on_single_too_large_package) {
    const std::string cipherText(raceConfig.wrapperQueueMaxSize + 1, 'a');
    EncPkg packageToSend(0, 0, {cipherText.begin(), cipherText.end()});

    // create a dummy connection
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"my persona"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0).handle;
    const LinkID connectionId = sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    EXPECT_CALL(*mockComms,
                sendPackage(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(0);
    const uint64_t batchId = 0;
    auto sdkResponse =
        sdk.sendEncryptedPackage(*sdk.getNM(), packageToSend, connectionId, batchId, 0);

    // make sure the plugins get the call before the expect
    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
    sdk.cleanShutdown();

    EXPECT_EQ(sdkResponse.status, SDK_INVALID_ARGUMENT);
}

/**
 * @brief sendEncryptedPackage queue utilization is the correct value
 *
 */
TEST_F(RaceSdkTestFixture, sendEncryptedPackage_returns_correct_utilization) {
    const std::string cipherText(raceConfig.wrapperQueueMaxSize / 100,
                                 'a');  // should cause 0.01 queue utilization
    EncPkg packageToSend(0, 0, {cipherText.begin(), cipherText.end()});

    // create a dummy connection
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"my persona"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0).handle;
    const LinkID connectionId = sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    EXPECT_CALL(*mockComms,
                sendPackage(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(1);
    const uint64_t batchId = 0;
    auto sdkResponse =
        sdk.sendEncryptedPackage(*sdk.getNM(), packageToSend, connectionId, batchId, 0);

    // make sure the mock comms gets the call before the expect
    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();

    EXPECT_EQ(sdkResponse.status, SDK_OK);
    EXPECT_NEAR(sdkResponse.queueUtilization, 0.01, 0.0001);
}

/**
 * @brief sendEncryptedPackage timeoutTimestamp is the correct value
 *
 */
TEST_F(RaceSdkTestFixture, sendEncryptedPackage_timeout_timestamp_correct) {
    const std::string cipherText = "super secret text";
    EncPkg packageToSend(0, 0, {cipherText.begin(), cipherText.end()});

    // create a dummy connection
    int32_t sendTimeout = 12345;
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"my persona"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, sendTimeout, 0).handle;

    const LinkID connectionId = sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
    double approxTimestamp = now.count() + sendTimeout;

    EXPECT_CALL(*mockComms,
                sendPackage(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce([&](RaceHandle, ConnectionID, EncPkg, double timeoutTimestamp, uint64_t) {
            EXPECT_NEAR(timeoutTimestamp, approxTimestamp, 1);
            return PLUGIN_OK;
        });
    const uint64_t batchId = 0;
    sdk.sendEncryptedPackage(*sdk.getNM(), packageToSend, connectionId, batchId, 0);

    // make sure the mock comms gets the call before the expect
    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
}

/**
 * @brief sendEncryptedPackage package timeout causes package failed callback
 *
 */
TEST_F(RaceSdkTestFixture, sendEncryptedPackage_timeout_package_failed) {
    const std::string cipherText = "super secret text";
    EncPkg packageToSend(0, 0, {cipherText.begin(), cipherText.end()});

    // create a dummy connection
    int32_t sendTimeout1 = 12345;
    int32_t sendTimeout2 = 0;
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"my persona"});

    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, sendTimeout1, 0).handle;
    const LinkID connectionId = sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    RaceHandle connHandle2 =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, sendTimeout2, 0).handle;
    const LinkID connectionId2 = sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle2, connectionId2, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    std::promise<void> promise;

    EXPECT_CALL(*mockComms,
                sendPackage(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce([&](RaceHandle, ConnectionID, EncPkg, double, uint64_t) {
            promise.get_future().wait();
            return PLUGIN_OK;
        });
    EXPECT_CALL(*mockNM, onPackageStatusChanged(::testing::_, ::testing::_)).Times(1);
    const uint64_t batchId = 0;
    sdk.sendEncryptedPackage(*sdk.getNM(), packageToSend, connectionId, batchId, 0);
    sdk.sendEncryptedPackage(*sdk.getNM(), packageToSend, connectionId2, batchId, 0);

    // have to wait for package to time out
    std::this_thread::sleep_for(10ms);
    promise.set_value();

    // make sure the mock comms gets the call before the expect
    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
}

////////////////////////////////////////////////////////////////
// presentCleartextMessage
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, presentCleartextMessage_does_not_crash_if_client_not_set) {
    const ClrMsg theMessageToSend("my message", "from sender", "to recipient", 1, 0, 0, 2, 3);
    sdk.presentCleartextMessage(*sdk.getNM(), theMessageToSend);
}

TEST_F(RaceSdkTestFixture, presentCleartextMessage_calls_the_client) {
    const ClrMsg theMessageToSend("my message", "from sender", "to recipient", 1, 0, 0, 2, 3);
    auto clrMsgValidator = [&theMessageToSend](ClrMsg msg) -> int {
        return (msg.getMsg() == theMessageToSend.getMsg() &&
                msg.getFrom() == theMessageToSend.getFrom() &&
                msg.getTo() == theMessageToSend.getTo()) ?
                   1 :
                   0;
    };

    EXPECT_CALL(mockApp, handleReceivedMessage(::testing::Truly(clrMsgValidator))).Times(1);

    sdk.presentCleartextMessage(*sdk.getNM(), theMessageToSend);

    sdk.cleanShutdown();
}

TEST_F(RaceSdkTestFixture, presentCleartextMessage_does_not_crash_on_empty_message) {
    const ClrMsg theMessageToSend("", "from sender", "to recipient", 1, 0, 0, 2, 3);

    EXPECT_CALL(mockApp, handleReceivedMessage(::testing::Truly([](ClrMsg) { return 1; })))
        .Times(1);
    sdk.presentCleartextMessage(*sdk.getNM(), theMessageToSend);

    sdk.cleanShutdown();
}

TEST_F(RaceSdkTestFixture, presentCleartextMessage_does_not_crash_on_missing_trace_id) {
    // trace and span id default to 0
    const ClrMsg theMessageToSend("my message", "from sender", "to recipient", 1, 0, 0);

    EXPECT_CALL(mockApp, handleReceivedMessage(::testing::Truly([](ClrMsg) { return 1; })))
        .Times(1);

    sdk.presentCleartextMessage(*sdk.getNM(), theMessageToSend);

    sdk.cleanShutdown();
}

TEST_F(RaceSdkTestFixture, presentCleartextMessage_amp_index_set_calls_amp) {
    const ClrMsg msg("my message", "from sender", "to recipient", 1, 0, 1);

    EXPECT_CALL(*mockArtifactManagerPlugin, receiveAmpMessage(msg.getMsg())).Times(1);
    EXPECT_CALL(mockApp, handleReceivedMessage(_)).Times(0);

    sdk.presentCleartextMessage(*sdk.getNM(), msg);

    sdk.cleanShutdown();
}

TEST_F(RaceSdkTestFixture, presentCleartextMessage_invalid_amp_index_doesnt_crash) {
    const ClrMsg msg("my message", "from sender", "to recipient", 1, 0, 99);

    EXPECT_CALL(*mockArtifactManagerPlugin, receiveAmpMessage(msg.getMsg())).Times(0);
    EXPECT_CALL(mockApp, handleReceivedMessage(_)).Times(0);

    sdk.presentCleartextMessage(*sdk.getNM(), msg);

    sdk.cleanShutdown();
}

////////////////////////////////////////////////////////////////
// onPluginStatusChanged
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, onPluginStatusChanged_does_not_crash_if_client_not_set) {
    PluginStatus pluginStatus = PLUGIN_NOT_READY;
    sdk.onPluginStatusChanged(*sdk.getNM(), pluginStatus);
}

TEST_F(RaceSdkTestFixture, onPluginStatusChanged_calls_the_client) {
    PluginStatus pluginStatus = PLUGIN_NOT_READY;
    nlohmann::json statusJson = nlohmann::json({});
    statusJson["network-manager-status"] = "PLUGIN_NOT_READY";
    EXPECT_CALL(mockApp, onSdkStatusChanged(statusJson)).Times(1);

    sdk.onPluginStatusChanged(*sdk.getNM(), pluginStatus);

    sdk.cleanShutdown();
}

////////////////////////////////////////////////////////////////
// getLinks
////////////////////////////////////////////////////////////////

/**
 * @brief getLinksForPersonas should return an empty vector if no links have been established by
 * the comms plugin.
 *
 */
TEST_F(RaceSdkTestFixture, getLinksForPersonas_returns_empty_vector) {
    const std::vector<LinkID> result = sdk.getLinksForPersonas({""}, LT_SEND);

    EXPECT_EQ(result.size(), 0);
}

////////////////////////////////////////////////////////////////
// Fatal Plugin Failures
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, send_fail_stops_plugin) {
    EncPkg packageToSend(0, 0, {});
    // create a dummy connection
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"my persona"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0).handle;
    const ConnectionID connectionId =
        sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    EXPECT_EQ(sdk.links->getLinkConnections(linkId).size(), 1);
    EXPECT_EQ(*sdk.links->getLinkConnections(linkId).begin(), connectionId);
    EXPECT_EQ(sdk.channels->getLinksForChannel(channelGid).size(), 1);
    EXPECT_EQ(*sdk.channels->getLinksForChannel(channelGid).begin(), linkId);

    EXPECT_CALL(*mockComms,
                sendPackage(::testing::_, connectionId, ::testing::_, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    const uint64_t batchId = 0;
    sdk.sendEncryptedPackage(*sdk.getNM(), packageToSend, connectionId, batchId, 0);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(10ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.links->getLinkConnections(linkId).size(), 0);
    EXPECT_EQ(sdk.channels->getLinksForChannel(channelGid).size(), 0);
}

TEST_F(RaceSdkTestFixture, open_fail_stops_plugin) {
    // create a dummy connection
    const LinkID linkId = sdk.getCommsWrapper("MockComms-0")->generateLinkId(channelGid);
    sdk.getNM()->createLink(channelGid, {"my persona"}, 0);

    EXPECT_CALL(*mockComms, openConnection(::testing::_, ::testing::_, ::testing::_, ::testing::_,
                                           ::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(10ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.links->getLinkConnections(linkId).size(), 0);
    EXPECT_EQ(sdk.channels->getLinksForChannel(channelGid).size(), 0);
}

TEST_F(RaceSdkTestFixture, close_fail_stops_plugin) {
    // create a dummy connection
    const LinkID linkId = sdk.getCommsWrapper("MockComms-0")->generateLinkId(channelGid);
    sdk.getNM()->createLink(channelGid, {"my persona"}, 0);
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0).handle;
    const ConnectionID connectionId =
        sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    EXPECT_CALL(*mockComms, closeConnection(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    sdk.closeConnection(*sdk.getNM(), connectionId, 0);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(10ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.links->getLinkConnections(linkId).size(), 0);
    EXPECT_EQ(sdk.channels->getLinksForChannel(channelGid).size(), 0);
}

TEST_F(RaceSdkTestFixture, plugin_fatal_deactivateChannel) {
    // create a dummy connection
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"my persona"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0).handle;
    const ConnectionID connectionId =
        sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    EXPECT_CALL(*mockComms, deactivateChannel(_, channelGid))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    sdk.deactivateChannel(*sdk.getNM(), channelGid, 0);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(10ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.links->getLinkConnections(linkId).size(), 0);
    EXPECT_EQ(sdk.channels->getLinksForChannel(channelGid).size(), 0);
}

TEST_F(RaceSdkTestFixture, plugin_fatal_activateChannel) {
    // create a dummy connection
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"role"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0).handle;

    const ConnectionID connectionId =
        sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    sdk.setChannelEnabled(channelGid, true);
    EXPECT_CALL(*mockComms, activateChannel(_, channelGid, {"role"}))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    sdk.activateChannel(*sdk.getNM(), channelGid, {"role"}, 0);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(10ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.links->getLinkConnections(linkId).size(), 0);
    EXPECT_EQ(sdk.channels->getLinksForChannel(channelGid).size(), 0);
}

TEST_F(RaceSdkTestFixture, plugin_fatal_destroyLink) {
    const LinkID testLinkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"role"});
    EXPECT_EQ(sdk.channels->getLinksForChannel(channelGid).size(), 1);
    EXPECT_EQ(*sdk.channels->getLinksForChannel(channelGid).begin(), testLinkId);

    EXPECT_CALL(*mockComms, destroyLink(_, testLinkId))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    sdk.destroyLink(*sdk.getNM(), testLinkId, 0);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(10ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.channels->getLinksForChannel(channelGid).size(), 0);
}

TEST_F(RaceSdkTestFixture, plugin_fatal_createLink) {
    const LinkID testLinkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"role"});

    EXPECT_CALL(*mockComms, createLink(_, channelGid))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    sdk.createLink(*sdk.getNM(), channelGid, {"role"}, 0);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(10ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.channels->getLinksForChannel(channelGid).size(), 0);
}

TEST_F(RaceSdkTestFixture, plugin_fatal_createBootstrapLink) {
    DeviceInfo deviceInfo;
    deviceInfo.architecture = "x86_64";
    deviceInfo.platform = "linux";
    deviceInfo.nodeType = "client";
    std::string passphrase = "password1";

    const LinkID testLinkId = createLinkForTesting(mockComms.get(), &sdk, "MockComms-0",
                                                   bootstrapChannelGid, {"bootstrap-role"});
    RaceHandle linkHandle = sdk.generateHandle(false);

    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    sdk.waitForCallbacks();

    EXPECT_CALL(*mockComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    sdk.createBootstrapLink(linkHandle, passphrase, bootstrapChannelGid);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(10ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.channels->getLinksForChannel(channelGid).size(), 0);
}

TEST_F(RaceSdkTestFixture, cancelBootstrap) {
    RaceHandle handle = 12345;
    MockBsManTestableSdk bsManTestSdk(appConfig, raceConfig, pluginLoader,
                                      std::make_shared<MockFileSystemHelper>());
    EXPECT_CALL(bsManTestSdk.mockBootstrapManager, cancelBootstrap(handle))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_ERROR));
    bsManTestSdk.cancelBootstrap(handle);
    bsManTestSdk.waitForCallbacks();
}

TEST_F(RaceSdkTestFixture, plugin_fatal_loadLinkAddress) {
    const LinkID linkId = sdk.getCommsWrapper("MockComms-0")->generateLinkId(channelGid);

    EXPECT_CALL(*mockComms, loadLinkAddress(_, channelGid, linkId))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    sdk.loadLinkAddress(*sdk.getNM(), channelGid, linkId, {"role"}, 0);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(10ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.links->getLinkConnections(linkId).size(), 0);
}

TEST_F(RaceSdkTestFixture, plugin_fatal_loadLinkAddresses) {
    const LinkID linkId = sdk.getCommsWrapper("MockComms-0")->generateLinkId(channelGid);

    EXPECT_CALL(*mockComms, loadLinkAddresses(_, channelGid, _))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    sdk.loadLinkAddresses(*sdk.getNM(), channelGid, {linkId}, {"role"}, 0);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(10ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.links->getLinkConnections(linkId).size(), 0);
}

TEST_F(RaceSdkTestFixture, plugin_fatal_createLinkFromAddress) {
    const LinkID linkId = sdk.getCommsWrapper("MockComms-0")->generateLinkId(channelGid);
    EXPECT_CALL(*mockComms, createLinkFromAddress(_, channelGid, linkId))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    sdk.createLinkFromAddress(*sdk.getNM(), channelGid, linkId, {"role"}, 0);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(10ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.links->getLinkConnections(linkId).size(), 0);
}

TEST_F(RaceSdkTestFixture, plugin_fatal_serveFiles) {
    // create a dummy connection
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"role"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0).handle;
    const ConnectionID connectionId =
        sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    EXPECT_CALL(*mockComms, serveFiles(linkId, "dummy/path"))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    sdk.serveFiles(linkId, "dummy/path", 0);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(10ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.links->getLinkConnections(linkId).size(), 0);
    EXPECT_EQ(sdk.channels->getLinksForChannel(channelGid).size(), 0);
}

TEST_F(RaceSdkTestFixture, plugin_fatal_flushChannel) {
    // create a dummy connection
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"role"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0).handle;
    const ConnectionID connectionId =
        sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    EXPECT_CALL(*mockComms, flushChannel(_, channelGid, 1))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    sdk.flushChannel(*sdk.getNM(), channelGid, 1, 0);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(10ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.links->getLinkConnections(linkId).size(), 0);
    EXPECT_EQ(sdk.channels->getLinksForChannel(channelGid).size(), 0);
}

TEST_F(RaceSdkTestFixture, plugin_fatal_onUserInputReceived) {
    // create a dummy connection
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"role"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0).handle;

    const ConnectionID connectionId =
        sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);
    sdk.waitForCallbacks();  // ensure connection is open
    EXPECT_CALL(*mockComms, onUserInputReceived(connHandle + 1, false, _))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_FATAL));
    sdk.requestPluginUserInput("MockComms-0", false, "key", "prompt?", true);

    int i = 0;
    for (; i < 100; i++) {
        try {
            sdk.getCommsWrapper("MockComms-0");
        } catch (std::out_of_range &) {
            break;
        } catch (std::exception &e) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: " +
                          std::string(e.what());
        } catch (...) {
            FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' threw unexpected exception: "
                      "<unknown>";
        }
        std::this_thread::sleep_for(20ms);
    }

    if (i == 100) {
        FAIL() << "Call to 'sdk.getCommsWrapper(\"MockComms\")' did not throw expected exception";
    }

    // ensure sdk cleaned up after comms
    EXPECT_EQ(sdk.links->getLinkConnections(linkId).size(), 0);
    EXPECT_EQ(sdk.channels->getLinksForChannel(channelGid).size(), 0);
}

/**
 * @brief Fixture class that pre-populates RaceSdk with a link profile and associated properties.
 *
 */
class RaceSdkTestFixtureGetLinksPrePopulateOnePersona : public RaceSdkTestFixture {
public:
    virtual void SetUp() override {
        initialize();

        std::vector<std::string> personas = {"My persona"};
        linkId = createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, personas);

        LinkProperties linkProps = getDefaultLinkProperties();
        linkProps.linkType = LT_SEND;
        sdk.getCommsWrapper("MockComms-0")->updateLinkProperties(linkId, linkProps, 0);
    }

public:
    LinkID linkId;
};

/**
 * @brief getLinks should return empty if the LinkType is undefined.
 *
 */
TEST_F(RaceSdkTestFixtureGetLinksPrePopulateOnePersona,
       getLinks_should_return_empty_for_invalid_link_type) {
    const std::vector<LinkID> result = sdk.getLinksForPersonas({}, LT_UNDEF);

    EXPECT_EQ(result.size(), 0);
}

/**
 * @brief getLinks should return all links that can reach the given persona of the given link type.
 *
 */
TEST_F(RaceSdkTestFixtureGetLinksPrePopulateOnePersona,
       getLinks_returns_available_link_to_persona) {
    const std::vector<LinkID> result = sdk.getLinksForPersonas({"My persona"}, LT_SEND);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], linkId);
}

/**
 * @brief getLinks should return nothing if the LinkType does not match.
 *
 */
TEST_F(RaceSdkTestFixtureGetLinksPrePopulateOnePersona,
       getLinks_should_return_nothing_if_link_types_do_not_match) {
    const std::vector<std::string> result = sdk.getLinksForPersonas({"My persona"}, LT_BIDI);

    ASSERT_EQ(result.size(), 0);
}

/**
 * @brief getLinks should return nothing if the Persona does not match.
 *
 */
TEST_F(RaceSdkTestFixtureGetLinksPrePopulateOnePersona,
       getLinks_should_return_nothing_for_a_different_persona) {
    const std::vector<LinkID> result = sdk.getLinksForPersonas({"Some other persona"}, LT_SEND);

    ASSERT_EQ(result.size(), 0);
}

/**
 * @brief getLinks should return nothing if each Persona does not match.
 *
 */
TEST_F(RaceSdkTestFixtureGetLinksPrePopulateOnePersona,
       getLinks_should_return_nothing_if_each_persona_does_not_match) {
    std::vector<std::string> personas = {"My persona", "Some other persona"};
    const std::vector<LinkID> result = sdk.getLinksForPersonas(personas, LT_SEND);

    ASSERT_EQ(result.size(), 0);
}

/**
 * @brief Fixture class that pre-populates RaceSdk with multiple link profiles and associated
 * properties.
 *
 */
class RaceSdkTestFixtureGetLinksPrePopulateManyPersonas : public RaceSdkTestFixture {
public:
    virtual void SetUp() override {
        initialize();

        {
            std::vector<std::string> personas = {"persona 1", "persona 2", "persona 3"};
            linkIdSend =
                createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, personas);

            LinkProperties linkProps = getDefaultLinkProperties();
            linkProps.linkType = LT_SEND;
            sdk.getCommsWrapper("MockComms-0")->updateLinkProperties(linkIdSend, linkProps, 0);
        }

        {
            std::vector<std::string> personas = {"persona 2", "persona 3", "persona 5"};
            linkIdBidi =
                createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, personas);

            LinkProperties linkProps = getDefaultLinkProperties();
            linkProps.linkType = LT_BIDI;
            sdk.getCommsWrapper("MockComms-0")->updateLinkProperties(linkIdBidi, linkProps, 0);
        }

        {
            std::vector<std::string> personas = {"persona 2", "persona 5", "persona 7"};
            linkIdRecv =
                createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, personas);

            LinkProperties linkProps = getDefaultLinkProperties();
            linkProps.linkType = LT_RECV;
            sdk.getCommsWrapper("MockComms-0")->updateLinkProperties(linkIdRecv, linkProps, 0);
        }
    }

public:
    LinkID linkIdSend;
    LinkID linkIdBidi;
    LinkID linkIdRecv;
};

#include <sstream>
template <typename T>
static std::string vectorToString(const std::vector<T> &someVector) {
    if (someVector.size() == 0) {
        return "";
    }

    std::stringstream result;
    result << "{ " << someVector[0];
    for (size_t index = 1; index < someVector.size(); ++index) {
        result << ", " << someVector[index];
    }
    result << " }";

    return result.str();
}

/**
 * @brief
 *
 */
TEST_F(RaceSdkTestFixtureGetLinksPrePopulateManyPersonas, getLinks_for_persona_1_2_send) {
    std::vector<std::string> personas = {"persona 1", "persona 2"};
    const std::vector<LinkID> result = sdk.getLinksForPersonas(personas, LT_SEND);

    EXPECT_EQ(result.size(), 1) << vectorToString<LinkID>(result);
}

TEST_F(RaceSdkTestFixtureGetLinksPrePopulateManyPersonas, getLinks_for_persona_5_send) {
    std::vector<std::string> personas = {"persona 5"};
    const std::vector<LinkID> result = sdk.getLinksForPersonas(personas, LT_SEND);

    EXPECT_EQ(result.size(), 1) << vectorToString<LinkID>(result);
    EXPECT_TRUE(std::find(result.begin(), result.end(), linkIdBidi) != result.end());
}

TEST_F(RaceSdkTestFixtureGetLinksPrePopulateManyPersonas, getLinks_for_persona_7_send) {
    std::vector<std::string> personas = {"persona 7"};
    const std::vector<LinkID> result = sdk.getLinksForPersonas(personas, LT_SEND);

    EXPECT_EQ(result.size(), 0) << vectorToString<LinkID>(result);
}

TEST_F(RaceSdkTestFixtureGetLinksPrePopulateManyPersonas, getLinks_for_persona_2_send) {
    std::vector<std::string> personas = {"persona 2"};
    const std::vector<LinkID> result = sdk.getLinksForPersonas(personas, LT_SEND);

    EXPECT_EQ(result.size(), 2) << vectorToString<LinkID>(result);
}

// ////////////////////////////////////////////////////////////////
// // updateLinkProperties
// ///////////////////////////////////////////////////////////////

/**
 * @brief updateLinkProperties should return an error if an invalid LinkID is provided.
 */

TEST_F(RaceSdkTestFixture, updateLinkProperties_should_return_error_for_invalid_link_id) {
    EXPECT_CALL(*mockNM, onLinkPropertiesChanged(::testing::_, ::testing::_)).Times(0);
    LinkProperties properties = getDefaultLinkProperties();
    properties.linkType = LT_SEND;
    EXPECT_EQ(sdk.getCommsWrapper("MockComms-0")->updateLinkProperties("", properties, 0).status,
              SDK_INVALID_ARGUMENT);
    EXPECT_EQ(
        sdk.getCommsWrapper("MockComms-0")->updateLinkProperties("LinkID_0", properties, 0).status,
        SDK_INVALID_ARGUMENT);
}

/**
 * @brief updateLinkProperties should return an error if the LinkProperties are invalid
 *
 */
TEST_F(RaceSdkTestFixture, updateLinkProperties_should_return_error_for_invalid_link_properties) {
    EXPECT_CALL(*mockNM, onLinkPropertiesChanged(::testing::_, ::testing::_)).Times(0);
    const LinkID linkId = sdk.getCommsWrapper("MockComms-0")->generateLinkId(channelGid);
    sdk.getNM()->createLink(channelGid, {""}, 0);
    LinkProperties properties;
    ASSERT_EQ(
        sdk.getCommsWrapper("MockComms-0")->updateLinkProperties(linkId, properties, 0).status,
        SDK_INVALID_ARGUMENT);
}

/**
 * @brief updateLinkProperties should return success if the LinkID and LinkProperties are valid.
 *
 */
TEST_F(RaceSdkTestFixture, updateLinkProperties_should_return_ok_for_valid_link_id) {
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {""});
    LinkProperties properties = getDefaultLinkProperties();
    properties.linkType = LT_SEND;
    EXPECT_CALL(*mockNM, onLinkPropertiesChanged(linkId, properties)).Times(1);
    ASSERT_EQ(
        sdk.getCommsWrapper("MockComms-0")->updateLinkProperties(linkId, properties, 0).status,
        SDK_OK);
}

////////////////////////////////////////////////////////////////
// receiveEncPkg
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, receiveEncPkg_should_handle_empty_package) {
    auto packageValidator = [](EncPkg package) -> int {
        return package.getCipherText().size() == 0 ? 1 : 0;
    };
    RaceHandle handle = 0;
    auto handleValidator = [&handle](RaceHandle commsHandle) -> int {
        handle = commsHandle;
        return 1;
    };

    // Set up a dummy connection.
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"my persona"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, RACE_UNLIMITED, 0).handle;
    const LinkID connectionId = sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    const std::vector<ConnectionID> connIds = {connectionId};
    EXPECT_CALL(*mockNM, processEncPkg(::testing::Truly(handleValidator),
                                       ::testing::Truly(packageValidator), connIds))
        .Times(1);

    EncPkg pkg(0, 0, {});

    auto sdkResponse = sdk.getCommsWrapper("MockComms-0")->receiveEncPkg(pkg, connIds, 0);

    // make sure the plugins get the call before the expect
    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
    sdk.cleanShutdown();

    EXPECT_EQ(handle, sdkResponse.handle);
}

TEST_F(RaceSdkTestFixture,
       receiveEncPkg_should_not_call_network_manager_if_the_connection_does_not_exist) {
    EXPECT_CALL(*mockNM, processEncPkg(::testing::_, ::testing::_, ::testing::_)).Times(0);

    EncPkg pkg(0, 0, {});
    const std::vector<ConnectionID> connIDs = {""};

    sdk.getCommsWrapper("MockComms-0")->receiveEncPkg(pkg, connIDs, 0);

    // make sure the plugins get the call before the expect
    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
    sdk.cleanShutdown();
}

TEST_F(RaceSdkTestFixture,
       receiveEncPkg_should_call_network_manager_if_some_of_the_connections_exist) {
    auto packageValidator = [](EncPkg package) -> int {
        return package.getCipherText().size() == 0 ? 1 : 0;
    };
    RaceHandle handle = 0;
    auto handleValidator = [&handle](RaceHandle commsHandle) -> int {
        handle = commsHandle;
        return 1;
    };

    // Set up a dummy connection.
    const LinkID linkId =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"my persona"});
    RaceHandle connHandle =
        sdk.openConnection(*sdk.getNM(), LT_RECV, linkId, "", 0, 0, RACE_UNLIMITED).handle;
    const LinkID connectionId = sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-0")
        ->onConnectionStatusChanged(connHandle, connectionId, CONNECTION_OPEN,
                                    getDefaultLinkProperties(), 0);

    // Add a few connection IDs that don't exist.
    const std::vector<ConnectionID> connIds = {connectionId, "fake1", "fake2"};
    const std::vector<ConnectionID> filteredConnIds = {connectionId};
    EXPECT_CALL(*mockNM, processEncPkg(::testing::Truly(handleValidator),
                                       ::testing::Truly(packageValidator), filteredConnIds))
        .Times(1);

    EncPkg pkg(0, 0, {});

    auto sdkResponse = sdk.getCommsWrapper("MockComms-0")->receiveEncPkg(pkg, connIds, 0);

    // make sure the plugins get the call before the expect
    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
    sdk.cleanShutdown();

    EXPECT_EQ(handle, sdkResponse.handle);
}

////////////////////////////////////////////////////////////////
// getSupportedChannels
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, getSupportedChannels_works) {
    EXPECT_EQ(sdk.getSupportedChannels().size(), 2);
}

////////////////////////////////////////////////////////////////
// getChannelProperties
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, getChannelProperties_works) {
    EXPECT_EQ(sdk.getChannelProperties(channelGid), channelProperties);
}

TEST_F(RaceSdkTestFixture, getChannelProperties_returns_default) {
    std::string channelGid = "channel1";
    EXPECT_EQ(sdk.getChannelProperties(channelGid), ChannelProperties());
}

////////////////////////////////////////////////////////////////
// loadLinkAddress
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, loadLinkAddress_nonexistent_channel) {
    std::string channelGid = "channel1";
    EXPECT_EQ(
        sdk.loadLinkAddress(*sdk.getNM(), channelGid, "", std::vector<std::string>({""}), 0).status,
        SDK_INVALID_ARGUMENT);
}

TEST_F(RaceSdkTestFixture, loadLinkAddress_unavailable_channel) {
    std::string channelGid = "channel1";
    sdk.getNM()->activateChannel(channelGid, "role", RACE_BLOCKING);
    EXPECT_EQ(
        sdk.loadLinkAddress(*sdk.getNM(), channelGid, "", std::vector<std::string>({""}), 0).status,
        SDK_INVALID_ARGUMENT);
}

TEST_F(RaceSdkTestFixture, loadLinkAddress_available_channel) {
    EXPECT_EQ(
        sdk.loadLinkAddress(*sdk.getNM(), channelGid, "", std::vector<std::string>({""}), 0).status,
        SDK_OK);
}

////////////////////////////////////////////////////////////////
// loadLinkAddresses
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, loadLinkAddresses_nonexistent_channel) {
    std::string channelGid = "channel1";
    EXPECT_EQ(sdk.loadLinkAddresses(*sdk.getNM(), channelGid, std::vector<std::string>({""}),
                                    std::vector<std::string>({""}), 0)
                  .status,
              SDK_INVALID_ARGUMENT);
}

TEST_F(RaceSdkTestFixture, loadLinkAddresses_unavailable_channel) {
    sdk.getCommsWrapper("MockComms-0")
        ->onChannelStatusChanged(0, channelGid, CHANNEL_UNAVAILABLE, ChannelProperties(), 0);
    EXPECT_EQ(sdk.loadLinkAddresses(*sdk.getNM(), channelGid, std::vector<std::string>({""}),
                                    std::vector<std::string>({""}), 0)
                  .status,
              SDK_INVALID_ARGUMENT);
}

TEST_F(RaceSdkTestFixture, loadLinkAddresses_available_channel) {
    EXPECT_EQ(sdk.loadLinkAddresses(*sdk.getNM(), channelGid, std::vector<std::string>({""}),
                                    std::vector<std::string>({""}), 0)
                  .status,
              SDK_OK);
}

////////////////////////////////////////////////////////////////
// createLink
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, createLink_nonexistent_channel) {
    std::string channelGid = "channel1";
    EXPECT_EQ(sdk.createLink(*sdk.getNM(), channelGid, std::vector<std::string>({""}), 0).status,
              SDK_INVALID_ARGUMENT);
}

TEST_F(RaceSdkTestFixture, createLink_unavailable_channel) {
    sdk.getCommsWrapper("MockComms-0")
        ->onChannelStatusChanged(0, channelGid, CHANNEL_UNAVAILABLE, ChannelProperties(), 0);
    EXPECT_EQ(sdk.createLink(*sdk.getNM(), channelGid, std::vector<std::string>({""}), 0).status,
              SDK_INVALID_ARGUMENT);
}

TEST_F(RaceSdkTestFixture, createLink_available_channel) {
    EXPECT_EQ(sdk.createLink(*sdk.getNM(), channelGid, std::vector<std::string>({""}), 0).status,
              SDK_OK);
}

////////////////////////////////////////////////////////////////
// createLinkFromAddress
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, createLinkFromAddress_nonexistent_channel) {
    std::string channelGid = "channel1";
    EXPECT_EQ(
        sdk.createLinkFromAddress(*sdk.getNM(), channelGid, "", std::vector<std::string>({""}), 0)
            .status,
        SDK_INVALID_ARGUMENT);
}

TEST_F(RaceSdkTestFixture, createLinkFromAddress_unavailable_channel) {
    sdk.getCommsWrapper("MockComms-0")
        ->onChannelStatusChanged(0, channelGid, CHANNEL_UNAVAILABLE, ChannelProperties(), 0);
    EXPECT_EQ(
        sdk.createLinkFromAddress(*sdk.getNM(), channelGid, "", std::vector<std::string>({""}), 0)
            .status,
        SDK_INVALID_ARGUMENT);
}

TEST_F(RaceSdkTestFixture, createLinkFromAddress_available_channel) {
    EXPECT_EQ(
        sdk.createLinkFromAddress(*sdk.getNM(), channelGid, "", std::vector<std::string>({""}), 0)
            .status,
        SDK_OK);
}

////////////////////////////////////////////////////////////////
// sendAmpMessage
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, sendAmpMessage_valid) {
    ClrMsg msg("{\"ampIndex\":1,\"body\":\"some message\"}", "test persona", "some destination", 0,
               0, 0);

    EXPECT_CALL(*mockNM, processClrMsg(_, msg)).Times(1);

    EXPECT_EQ(
        sdk.sendAmpMessage("MockArtifactManager-0", "some destination", "some message").status,
        SDK_OK);
}

TEST_F(RaceSdkTestFixture, sendAmpMessage_invalid_plugin_id) {
    EXPECT_CALL(*mockNM, processClrMsg(_, _)).Times(0);

    EXPECT_EQ(sdk.sendAmpMessage("invalid pluginId", "some destination", "some message").status,
              SDK_INVALID_ARGUMENT);
}

////////////////////////////////////////////////////////////////
// LinkID == "" is rejected
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, onLinkStatusChanged_rejects) {
    RaceHandle handle = 42;
    LinkID linkId = "";
    EXPECT_EQ(sdk.getCommsWrapper("MockComms-0")
                  ->onLinkStatusChanged(handle, linkId, LINK_CREATED, LinkProperties(), 0)
                  .status,
              SDK_INVALID_ARGUMENT);
}

TEST_F(RaceSdkTestFixture, generateConnectionId_rejects) {
    LinkID linkId = "";
    EXPECT_EQ(sdk.getCommsWrapper("MockComms-0")->generateConnectionId(linkId), "");
}

////////////////////////////////////////////////////////////////
// ConnectionID == "" is rejected
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, onConnectionStatusChanged_rejects) {
    RaceHandle handle = 42;
    ConnectionID connId = "";
    EXPECT_EQ(sdk.getCommsWrapper("MockComms-0")
                  ->onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, LinkProperties(), 0)
                  .status,
              SDK_INVALID_ARGUMENT);
}

TEST_F(RaceSdkTestFixture, receiveEncPkg_rejects) {
    EncPkg pkg(0, 0, {});
    EXPECT_EQ(sdk.getCommsWrapper("MockComms-0")
                  ->receiveEncPkg(pkg, std::vector<ConnectionID>{"valid1", "", "valid2"}, 0)
                  .status,
              SDK_INVALID_ARGUMENT);
}

////////////////////////////////////////////////////////////////
// Bootstrap tests
////////////////////////////////////////////////////////////////

static RaceConfig createBootstrapRaceConfig() {
    RaceConfig config;
    config.androidPythonPath = "";
    // config.plugins;
    // config.enabledChannels;
    config.isPluginFetchOnStartEnabled = true;
    config.isVoaEnabled = true;
    config.wrapperQueueMaxSize = 1000000;
    config.wrapperTotalMaxSize = 1000000000;
    config.logLevel = RaceLog::LL_DEBUG;
    // config.logLevelStdout = RaceLog::LL_DEBUG;
    config.logRaceConfig = false;
    config.logNMConfig = false;
    config.logCommsConfig = false;
    config.msgLogLength = 256;

    ChannelProperties channelProperties;
    channelProperties.channelStatus = CHANNEL_ENABLED;
    channelProperties.channelGid = "channel1";

    ChannelProperties bootstrapChannelProperties;
    bootstrapChannelProperties.channelStatus = CHANNEL_ENABLED;
    bootstrapChannelProperties.channelGid = "channel2";
    bootstrapChannelProperties.connectionType = CT_LOCAL;
    bootstrapChannelProperties.bootstrap = true;

    ChannelProperties altBootstrapChannelProperties;
    altBootstrapChannelProperties.channelStatus = CHANNEL_ENABLED;
    altBootstrapChannelProperties.channelGid = "channel3";
    altBootstrapChannelProperties.connectionType = CT_LOCAL;
    altBootstrapChannelProperties.bootstrap = true;

    ChannelRole role;
    role.roleName = "role";
    role.linkSide = LS_BOTH;
    channelProperties.roles = {role};
    bootstrapChannelProperties.roles = {role};
    altBootstrapChannelProperties.roles = {role};

    config.channels = {channelProperties, bootstrapChannelProperties,
                       altBootstrapChannelProperties};

    PluginDef networkManagerPluginDef;
    PluginDef commsPluginDef1;
    PluginDef commsPluginDef2;
    PluginDef commsPluginDef3;
    PluginDef ampPluginDef;

    commsPluginDef1.filePath = "MockComms-0";
    commsPluginDef1.channels = {channelProperties.channelGid};
    commsPluginDef2.filePath = "MockComms-1";
    commsPluginDef2.channels = {bootstrapChannelProperties.channelGid};
    commsPluginDef3.filePath = "MockComms-2";
    commsPluginDef3.channels = {altBootstrapChannelProperties.channelGid};

    config.plugins[RaceEnums::PluginType::PT_NM] = {networkManagerPluginDef};
    config.plugins[RaceEnums::PluginType::PT_COMMS] = {commsPluginDef1, commsPluginDef2,
                                                       commsPluginDef3};
    config.plugins[RaceEnums::PluginType::PT_ARTIFACT_MANAGER] = {ampPluginDef};

    config.environmentTags = {{"", {}}};

    return config;
}

class BootstrapTestFixture : public ::testing::Test {
public:
    BootstrapTestFixture() :
        appConfig(createDefaultAppConfig()),
        raceConfig(createBootstrapRaceConfig()),
        mockNM(std::make_shared<MockRacePluginNM>()),
        mockComms(std::make_shared<MockRacePluginComms>()),
        mockBootstrapComms(std::make_shared<MockRaceBootstrapPluginComms>()),
        mockAltBootstrapComms(std::make_shared<MockRaceBootstrapPluginComms>()),
        mockArtifactManagerPlugin(std::make_shared<MockRacePluginArtifactManager>()),
        pluginLoader({mockNM}, {mockComms, mockBootstrapComms, mockAltBootstrapComms},
                     {mockArtifactManagerPlugin}),
        sdk(appConfig, raceConfig, pluginLoader, std::make_shared<MockFileSystemHelper>()),
        mockApp(&sdk) {
        createAppDirectories(appConfig);
        ::testing::DefaultValue<PluginResponse>::Set(PLUGIN_OK);

        sdk.initRaceSystem(&mockApp);
        setupMockArtifactManager();

        channelProperties = raceConfig.channels[0];
        channelGid = channelProperties.channelGid;
        bootstrapChannelProperties = raceConfig.channels[1];
        bootstrapChannelGid = bootstrapChannelProperties.channelGid;
        altBootstrapChannelProperties = raceConfig.channels[2];
        altBootstrapChannelGid = altBootstrapChannelProperties.channelGid;

        sdk.getNM()->activateChannel(channelGid, "role", RACE_BLOCKING);
        sdk.getNM()->activateChannel(bootstrapChannelGid, "role", RACE_BLOCKING);
        sdk.getNM()->activateChannel(altBootstrapChannelGid, "role", RACE_BLOCKING);
        waitForCallbacks();
        sdk.getCommsWrapper("MockComms-0")
            ->onChannelStatusChanged(0, channelGid, CHANNEL_AVAILABLE, channelProperties, 0);
        sdk.getCommsWrapper("MockComms-1")
            ->onChannelStatusChanged(0, bootstrapChannelGid, CHANNEL_AVAILABLE,
                                     bootstrapChannelProperties, 0);
        sdk.getCommsWrapper("MockComms-2")
            ->onChannelStatusChanged(0, altBootstrapChannelGid, CHANNEL_AVAILABLE,
                                     altBootstrapChannelProperties, 0);

        deviceInfo.architecture = "x86_64";
        deviceInfo.platform = "linux";
        deviceInfo.nodeType = "client";

        bootstrapLinkProperties.linkType = LT_BIDI;
        bootstrapLinkProperties.connectionType = CT_LOCAL;
        bootstrapLinkProperties.transmissionType = TT_UNICAST;
        bootstrapLinkProperties.sendType = ST_EPHEM_SYNC;
        bootstrapLinkProperties.channelGid = bootstrapChannelGid;

        altBootstrapLinkProperties.linkType = LT_BIDI;
        altBootstrapLinkProperties.connectionType = CT_LOCAL;
        altBootstrapLinkProperties.transmissionType = TT_UNICAST;
        altBootstrapLinkProperties.sendType = ST_EPHEM_SYNC;
        altBootstrapLinkProperties.channelGid = bootstrapChannelGid;
    }
    virtual ~BootstrapTestFixture() {}
    virtual void SetUp() override {}
    virtual void TearDown() override {
        sdk.cleanShutdown();
    }

    void setupMockArtifactManager() {
        mockArtifactManager = new MockArtifactManager;
        EXPECT_CALL(*mockArtifactManager, getIds())
            .WillRepeatedly(::testing::Return(std::vector<std::string>{"MockArtifactManager-0"}));
        sdk.setArtifactManager(mockArtifactManager);
    }

    void waitForCallbacks() {
        if (sdk.getBootstrapThread()) {
            sdk.getBootstrapThread()->waitForCallbacks();
        }
        sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
        sdk.getCommsWrapper("MockComms-1")->waitForCallbacks();
        sdk.getCommsWrapper("MockComms-2")->waitForCallbacks();
        sdk.getNM()->waitForCallbacks();
    }

public:
    AppConfig appConfig;
    RaceConfig raceConfig;
    std::shared_ptr<MockRacePluginNM> mockNM;
    std::shared_ptr<MockRacePluginComms> mockComms;
    std::shared_ptr<MockRaceBootstrapPluginComms> mockBootstrapComms;
    std::shared_ptr<MockRaceBootstrapPluginComms> mockAltBootstrapComms;
    std::shared_ptr<MockRacePluginArtifactManager> mockArtifactManagerPlugin;
    MockPluginLoader pluginLoader;
    // Keep a raw pointer here since it is wrapped in a unique_ptr in the SDK class
    MockArtifactManager *mockArtifactManager;
    TestableRaceSdk sdk;
    MockRaceApp mockApp;

    std::string channelGid;
    std::string bootstrapChannelGid;
    std::string altBootstrapChannelGid;
    ChannelProperties channelProperties;
    ChannelProperties bootstrapChannelProperties;
    ChannelProperties altBootstrapChannelProperties;
    LinkProperties bootstrapLinkProperties;
    LinkProperties altBootstrapLinkProperties;
    DeviceInfo deviceInfo;
    std::string passphrase = "password1";
};

inline bool operator==(const DeviceInfo &lhs, const DeviceInfo &rhs) {
    return lhs.platform == rhs.platform && lhs.architecture == rhs.architecture &&
           lhs.nodeType == rhs.nodeType;
}

/**
 * @brief Calling prepareToBootstrap should cause a networkManager to be called
 *
 */
TEST_F(BootstrapTestFixture, bootstrap_prepareToBootstrap_calls_createBootstrapLink) {
    EXPECT_CALL(*mockBootstrapComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .Times(1);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    sdk.cleanShutdown();
}

/**
 * @brief Calling prepareToBootstrap should select the preferred bootstrap channel
 *
 */
TEST_F(BootstrapTestFixture, bootstrap_prepareToBootstrap_selects_pref_channel) {
    // Select bootstrapChannelGid if that is specified
    EXPECT_CALL(*mockBootstrapComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .Times(1);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();

    // Select bootstrapChannelGid if that is specified
    EXPECT_CALL(*mockAltBootstrapComms, createBootstrapLink(_, altBootstrapChannelGid, passphrase))
        .Times(1);
    sdk.prepareToBootstrap(deviceInfo, passphrase, altBootstrapChannelGid);
    waitForCallbacks();

    // Select bootstrapChannelGid (first) if no preferred channel ID specified
    EXPECT_CALL(*mockBootstrapComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .Times(1);
    sdk.prepareToBootstrap(deviceInfo, passphrase, std::string());
    waitForCallbacks();

    sdk.cleanShutdown();
}

/**
 * @brief Calling prepareToBootstrap adds an entry to the pending bootstrap map
 *
 */
TEST_F(BootstrapTestFixture, bootstrap_pending_bootstrap_added) {
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    sdk.cleanShutdown();
}

/**
 * @brief Calling prepareToBootstrap with invalid deviceInfo should not cause a networkManager to be
 * called or add an entry to the pending bootstrap map
 *
 */
TEST_F(BootstrapTestFixture, bootstrap_bad_device_info) {
    DeviceInfo deviceInfo;  // invalid platform, architecture, nodeType
    std::string passphrase = "password1";
    EXPECT_CALL(*mockNM, prepareToBootstrap(_, _, _, _)).Times(0);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();

    sdk.cleanShutdown();
}

TEST_F(BootstrapTestFixture, bootstrap_bad_device_info2) {
    deviceInfo.architecture = "invalid architecture";
    EXPECT_CALL(*mockNM, prepareToBootstrap(_, _, _, _)).Times(0);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    sdk.cleanShutdown();
}

TEST_F(BootstrapTestFixture, bootstrap_bad_device_info3) {
    deviceInfo.platform = "invalid platform";
    EXPECT_CALL(*mockNM, prepareToBootstrap(_, _, _, _)).Times(0);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    sdk.cleanShutdown();
}

TEST_F(BootstrapTestFixture, bootstrap_bad_device_info4) {
    deviceInfo.nodeType = "invalid node type";
    EXPECT_CALL(*mockNM, prepareToBootstrap(_, _, _, _)).Times(0);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();

    sdk.cleanShutdown();
}

/**
 * @brief bootstrapping when there are no bootstrap channels should not result in a pending
 * bootstrap
 */
TEST_F(BootstrapTestFixture, bootstrap_prepareToBootstrap_no_available_channels) {
    // bootstrap channel is not unavailable
    sdk.getCommsWrapper("MockComms-1")
        ->onChannelStatusChanged(0, bootstrapChannelGid, CHANNEL_UNAVAILABLE,
                                 bootstrapChannelProperties, 0);
    EXPECT_CALL(*mockBootstrapComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .Times(0);
    EXPECT_CALL(*mockNM, onBootstrapFinished(_, BOOTSTRAP_FAILED))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_OK));
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    sdk.cleanShutdown();
}

/**
 * @brief onLinkStatusChanged with LINK_CREATED from the bootstrap plugin calls serveFiles on the
 * bootstrap plugin
 */
TEST_F(BootstrapTestFixture, onLinkStatusChanged_causes_network_manager_prepareToBootstrap) {
    RaceHandle handle = NULL_RACE_HANDLE;
    EXPECT_CALL(*mockBootstrapComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .WillOnce([&handle](RaceHandle receivedHandle, std::string, std::string) {
            handle = receivedHandle;
            return PLUGIN_OK;
        });

    LinkID linkId = sdk.getCommsWrapper("MockComms-1")->generateLinkId(bootstrapChannelGid);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    EXPECT_CALL(*mockNM, prepareToBootstrap(_, _, _, deviceInfo));
    sdk.getCommsWrapper("MockComms-1")
        ->onLinkStatusChanged(handle, linkId, LINK_CREATED, bootstrapLinkProperties, RACE_BLOCKING);
    waitForCallbacks();
    sdk.cleanShutdown();
}

/**
 * @brief If bootstrap link creation fails, failure is handled and serve files is not called
 */
TEST_F(BootstrapTestFixture, onLinkStatusChanged_failed) {
    RaceHandle handle = NULL_RACE_HANDLE;
    EXPECT_CALL(*mockBootstrapComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .WillOnce([&handle](RaceHandle receivedHandle, std::string, std::string) {
            handle = receivedHandle;
            return PLUGIN_OK;
        });
    EXPECT_CALL(*mockNM, onBootstrapFinished(_, BOOTSTRAP_FAILED))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_OK));

    LinkID linkId = sdk.getCommsWrapper("MockComms-1")->generateLinkId(bootstrapChannelGid);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    EXPECT_CALL(*mockNM, prepareToBootstrap(_, _, _, deviceInfo)).Times(0);
    sdk.getCommsWrapper("MockComms-1")
        ->onLinkStatusChanged(handle, linkId, LINK_DESTROYED, {}, RACE_BLOCKING);

    waitForCallbacks();
    sdk.cleanShutdown();
}

/**
 * @brief If bootstrap link creation fails, failure is handled and serve files is not called
 */
TEST_F(BootstrapTestFixture, prepareToBootstrap_bootstrapFailed) {
    RaceHandle handle = NULL_RACE_HANDLE;
    RaceHandle handle2 = NULL_RACE_HANDLE;
    EXPECT_CALL(*mockBootstrapComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .WillOnce([&handle](RaceHandle receivedHandle, std::string, std::string) {
            handle = receivedHandle;
            return PLUGIN_OK;
        });
    EXPECT_CALL(*mockNM, onBootstrapFinished(_, BOOTSTRAP_FAILED))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_OK));

    LinkID linkId = sdk.getCommsWrapper("MockComms-1")->generateLinkId(bootstrapChannelGid);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    EXPECT_CALL(*mockNM, prepareToBootstrap(_, _, _, deviceInfo))
        .WillOnce(
            [&handle2](RaceHandle receivedHandle, std::string /*linkId*/, std::string, DeviceInfo) {
                handle2 = receivedHandle;
                return PLUGIN_OK;
            });
    sdk.getCommsWrapper("MockComms-1")
        ->onLinkStatusChanged(handle, linkId, LINK_CREATED, bootstrapLinkProperties, RACE_BLOCKING);
    waitForCallbacks();
    sdk.getNM()->bootstrapFailed(handle2);
    sdk.cleanShutdown();
}

/**
 * @brief If bootstrap link creation fails, failure is handled and serve files is not called
 */
TEST_F(BootstrapTestFixture, bootstrapDevice) {
    RaceHandle handle = NULL_RACE_HANDLE;
    RaceHandle handle2 = NULL_RACE_HANDLE;
    EXPECT_CALL(*mockBootstrapComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .WillOnce([&handle](RaceHandle receivedHandle, std::string, std::string) {
            handle = receivedHandle;
            return PLUGIN_OK;
        });

    LinkID linkId = sdk.getCommsWrapper("MockComms-1")->generateLinkId(bootstrapChannelGid);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    EXPECT_CALL(*mockNM, prepareToBootstrap(_, _, _, deviceInfo))
        .WillOnce(
            [&handle2](RaceHandle receivedHandle, std::string /*linkId*/, std::string, DeviceInfo) {
                handle2 = receivedHandle;
                return PLUGIN_OK;
            });
    sdk.getCommsWrapper("MockComms-1")
        ->onLinkStatusChanged(handle, linkId, LINK_CREATED, bootstrapLinkProperties, RACE_BLOCKING);
    waitForCallbacks();

    EXPECT_CALL(*mockArtifactManager, acquirePlugin(_, _, _, _, _))
        .WillRepeatedly([](std::string destPath, std::string pluginName, std::string, std::string,
                           std::string) {
            fs::create_directory(destPath + "/" + pluginName);
            return true;
        });
    EXPECT_CALL(*mockBootstrapComms, serveFiles(linkId, _)).Times(1);
    EXPECT_CALL(*mockBootstrapComms, openConnection(_, _, _, _, _))
        .WillOnce([&handle2](RaceHandle receivedHandle, LinkType, LinkID, std::string, int32_t) {
            handle2 = receivedHandle;
            return PLUGIN_OK;
        });

    sdk.getNM()->bootstrapDevice(handle2, {});
    waitForCallbacks();
    sdk.cleanShutdown();
}

/**
 * @brief if the onConnectionStatusChanged response is CONNECTION_CLOSED instead of CONNECTION_OPEN,
 * clean up the pendingBootstrap
 */
TEST_F(BootstrapTestFixture, openConnection_failed) {
    RaceHandle handle = NULL_RACE_HANDLE;
    RaceHandle handle2 = NULL_RACE_HANDLE;
    EXPECT_CALL(*mockBootstrapComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .WillOnce([&handle](RaceHandle receivedHandle, std::string, std::string) {
            handle = receivedHandle;
            return PLUGIN_OK;
        });

    LinkID linkId = sdk.getCommsWrapper("MockComms-1")->generateLinkId(bootstrapChannelGid);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    EXPECT_CALL(*mockNM, prepareToBootstrap(_, _, _, deviceInfo))
        .WillOnce(
            [&handle2](RaceHandle receivedHandle, std::string /*linkId*/, std::string, DeviceInfo) {
                handle2 = receivedHandle;
                return PLUGIN_OK;
            });
    sdk.getCommsWrapper("MockComms-1")
        ->onLinkStatusChanged(handle, linkId, LINK_CREATED, bootstrapLinkProperties, RACE_BLOCKING);
    waitForCallbacks();

    EXPECT_CALL(*mockArtifactManager, acquirePlugin(_, _, _, _, _))
        .WillRepeatedly([](std::string destPath, std::string pluginName, std::string, std::string,
                           std::string) {
            fs::create_directory(destPath + "/" + pluginName);
            return true;
        });
    EXPECT_CALL(*mockBootstrapComms, serveFiles(linkId, _)).Times(1);
    EXPECT_CALL(*mockBootstrapComms, openConnection(_, _, _, _, _))
        .WillOnce([&handle2](RaceHandle receivedHandle, LinkType, LinkID, std::string, int32_t) {
            handle2 = receivedHandle;
            return PLUGIN_OK;
        });

    EXPECT_CALL(*mockNM, onBootstrapFinished(_, BOOTSTRAP_FAILED))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_OK));

    sdk.getNM()->bootstrapDevice(handle2, {});

    waitForCallbacks();
    const ConnectionID connectionId =
        sdk.getCommsWrapper("MockComms-1")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-1")
        ->onConnectionStatusChanged(handle2, connectionId, CONNECTION_CLOSED,
                                    bootstrapLinkProperties, RACE_BLOCKING);
    waitForCallbacks();
    sdk.cleanShutdown();
}

/**
 * @brief receiveEncPkg from the bootstrap plugin results in onBootstrapPkgReceived for the
 * networkManager
 */
TEST_F(BootstrapTestFixture, receiveEncPkg_causes_onBootstrapPkgReceived) {
    RaceHandle handle = NULL_RACE_HANDLE;
    RaceHandle handle2 = NULL_RACE_HANDLE;
    EXPECT_CALL(*mockBootstrapComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .WillOnce([&handle](RaceHandle receivedHandle, std::string, std::string) {
            handle = receivedHandle;
            return PLUGIN_OK;
        });

    LinkID linkId = sdk.getCommsWrapper("MockComms-1")->generateLinkId(bootstrapChannelGid);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    EXPECT_CALL(*mockNM, prepareToBootstrap(_, _, _, deviceInfo))
        .WillOnce(
            [&handle2](RaceHandle receivedHandle, std::string /*linkId*/, std::string, DeviceInfo) {
                handle2 = receivedHandle;
                return PLUGIN_OK;
            });
    sdk.getCommsWrapper("MockComms-1")
        ->onLinkStatusChanged(handle, linkId, LINK_CREATED, bootstrapLinkProperties, RACE_BLOCKING);
    waitForCallbacks();

    EXPECT_CALL(*mockArtifactManager, acquirePlugin(_, _, _, _, _))
        .WillRepeatedly([](std::string destPath, std::string pluginName, std::string, std::string,
                           std::string) {
            fs::create_directory(destPath + "/" + pluginName);
            return true;
        });
    EXPECT_CALL(*mockBootstrapComms, serveFiles(linkId, _)).Times(1);
    EXPECT_CALL(*mockBootstrapComms, openConnection(_, _, _, _, _))
        .WillOnce([&handle2](RaceHandle receivedHandle, LinkType, LinkID, std::string, int32_t) {
            handle2 = receivedHandle;
            return PLUGIN_OK;
        });

    sdk.getNM()->bootstrapDevice(handle2, {});

    waitForCallbacks();
    const ConnectionID connectionId =
        sdk.getCommsWrapper("MockComms-1")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-1")
        ->onConnectionStatusChanged(handle2, connectionId, CONNECTION_OPEN, bootstrapLinkProperties,
                                    RACE_BLOCKING);
    waitForCallbacks();

    std::string key_string = "key";
    EXPECT_CALL(*mockNM, onBootstrapPkgReceived("bootstrap-client",
                                                RawData{key_string.begin(), key_string.end()}))
        .Times(1);
    EXPECT_CALL(*mockNM, processEncPkg(_, _, _)).Times(0);

    std::string bootstrapPkgData = R"({
        "persona": "bootstrap-client",
        "key": "a2V5"
})";
    EncPkg pkg(0, 0, {bootstrapPkgData.begin(), bootstrapPkgData.end()});
    pkg.setPackageType(PKG_TYPE_SDK);

    sdk.getCommsWrapper("MockComms-1")->receiveEncPkg(pkg, {connectionId}, RACE_BLOCKING);

    waitForCallbacks();

    sdk.cleanShutdown();
}

/**
 * @brief serveFiles copies comms plugins downloaded via artifact manager to the plugin dir
 */
TEST_F(BootstrapTestFixture, serveFiles_copies_comms_plugins) {
    std::string bundleDestPath;
    RaceHandle handle = NULL_RACE_HANDLE;
    RaceHandle handle2 = NULL_RACE_HANDLE;
    EXPECT_CALL(*mockBootstrapComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .WillOnce([&handle](RaceHandle receivedHandle, std::string, std::string) {
            handle = receivedHandle;
            return PLUGIN_OK;
        });

    LinkID linkId = sdk.getCommsWrapper("MockComms-1")->generateLinkId(bootstrapChannelGid);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    EXPECT_CALL(*mockNM, prepareToBootstrap(_, _, _, deviceInfo))
        .WillOnce([&handle2, &bundleDestPath](RaceHandle receivedHandle, std::string /*linkId*/,
                                              std::string configPath, DeviceInfo) {
            // configPath will be "bootstrap-file/{timestamp}"
            std::string bootstrapTimeStamp = fs::path(configPath).filename().native();
            bundleDestPath = "/tmp/test-files/bootstrapFilesDirectory/" + bootstrapTimeStamp;
            handle2 = receivedHandle;
            return PLUGIN_OK;
        });
    sdk.getCommsWrapper("MockComms-1")
        ->onLinkStatusChanged(handle, linkId, LINK_CREATED, bootstrapLinkProperties, RACE_BLOCKING);
    waitForCallbacks();

    EXPECT_CALL(*mockArtifactManager, acquirePlugin(_, _, _, _, _))
        .WillRepeatedly([](std::string destPath, std::string pluginName, std::string, std::string,
                           std::string) {
            fs::create_directory(destPath + "/" + pluginName);
            return true;
        });
    EXPECT_CALL(*mockBootstrapComms, serveFiles(linkId, _)).Times(1);
    EXPECT_CALL(*mockBootstrapComms, openConnection(_, _, _, _, _))
        .WillOnce([&handle2](RaceHandle receivedHandle, LinkType, LinkID, std::string, int32_t) {
            handle2 = receivedHandle;
            return PLUGIN_OK;
        });

    sdk.getNM()->bootstrapDevice(handle2, {channelGid, bootstrapChannelGid});

    waitForCallbacks();
    const ConnectionID connectionId =
        sdk.getCommsWrapper("MockComms-1")->generateConnectionId(linkId);
    sdk.getCommsWrapper("MockComms-1")
        ->onConnectionStatusChanged(handle2, connectionId, CONNECTION_OPEN, bootstrapLinkProperties,
                                    RACE_BLOCKING);
    waitForCallbacks();

    std::cout << "bundleDestPath: " << bundleDestPath << std::endl;
    EXPECT_TRUE(fs::exists(bundleDestPath)) << bundleDestPath << " does not exist";
    EXPECT_TRUE(fs::exists(bundleDestPath + "/race"));
    EXPECT_TRUE(fs::exists(bundleDestPath + "/artifacts/network-manager/MockNM-0"));
    EXPECT_TRUE(fs::exists(bundleDestPath + "/artifacts/comms/MockComms-0"));
    EXPECT_TRUE(fs::exists(bundleDestPath + "/artifacts/comms/MockComms-1"));
    EXPECT_TRUE(fs::exists(bundleDestPath + "/artifacts/artifact-manager/MockArtifactManager-0"));

    std::string key_string = "key";
    EXPECT_CALL(*mockNM, onBootstrapPkgReceived("bootstrap-client",
                                                RawData{key_string.begin(), key_string.end()}))
        .Times(1);
    EXPECT_CALL(*mockNM, processEncPkg(_, _, _)).Times(0);

    std::string bootstrapPkgData = R"({
        "persona": "bootstrap-client",
        "key": "a2V5"
})";
    EncPkg pkg(0, 0, {bootstrapPkgData.begin(), bootstrapPkgData.end()});
    pkg.setPackageType(PKG_TYPE_SDK);

    sdk.getCommsWrapper("MockComms-1")->receiveEncPkg(pkg, {connectionId}, RACE_BLOCKING);

    waitForCallbacks();

    sdk.cleanShutdown();
}

/**
 * @brief if acquire plugin fails, the failure is handled properly
 */
TEST_F(BootstrapTestFixture, acquirePluginn_fails) {
    RaceHandle handle = NULL_RACE_HANDLE;
    RaceHandle handle2 = NULL_RACE_HANDLE;
    EXPECT_CALL(*mockBootstrapComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .WillOnce([&handle](RaceHandle receivedHandle, std::string, std::string) {
            handle = receivedHandle;
            return PLUGIN_OK;
        });

    LinkID linkId = sdk.getCommsWrapper("MockComms-1")->generateLinkId(bootstrapChannelGid);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    EXPECT_CALL(*mockNM, prepareToBootstrap(_, _, _, deviceInfo))
        .WillOnce(
            [&handle2](RaceHandle receivedHandle, std::string /*linkId*/, std::string, DeviceInfo) {
                handle2 = receivedHandle;
                return PLUGIN_OK;
            });
    sdk.getCommsWrapper("MockComms-1")
        ->onLinkStatusChanged(handle, linkId, LINK_CREATED, bootstrapLinkProperties, RACE_BLOCKING);
    waitForCallbacks();

    EXPECT_CALL(*mockArtifactManager, acquirePlugin(_, _, _, _, _)).Times(5);
    EXPECT_CALL(*mockBootstrapComms, serveFiles(linkId, _)).Times(0);
    EXPECT_CALL(*mockBootstrapComms, openConnection(_, _, _, _, _)).Times(0);
    EXPECT_CALL(*mockNM, onBootstrapFinished(_, BOOTSTRAP_FAILED))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_OK));

    sdk.getNM()->bootstrapDevice(handle2, {channelGid, bootstrapChannelGid});
    waitForCallbacks();

    sdk.cleanShutdown();
}

/**
 * @brief IF the symlink fails, the error is handled properly
 */
TEST_F(BootstrapTestFixture, serveFiles_symlink_fails) {
    RaceHandle handle = NULL_RACE_HANDLE;
    RaceHandle handle2 = NULL_RACE_HANDLE;
    EXPECT_CALL(*mockBootstrapComms, createBootstrapLink(_, bootstrapChannelGid, passphrase))
        .WillOnce([&handle](RaceHandle receivedHandle, std::string, std::string) {
            handle = receivedHandle;
            return PLUGIN_OK;
        });

    LinkID linkId = sdk.getCommsWrapper("MockComms-1")->generateLinkId(bootstrapChannelGid);
    sdk.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelGid);
    waitForCallbacks();
    EXPECT_CALL(*mockNM, prepareToBootstrap(_, _, _, deviceInfo))
        .WillOnce([&handle2](RaceHandle receivedHandle, std::string /*linkId*/,
                             std::string configPath, DeviceInfo) {
            // create empty directory to prevent symlink from working.
            // configPath will be "bootstrap-file/{timestamp}"
            std::string bootstrapTimeStamp = fs::path(configPath).filename().native();
            fs::create_directories("/tmp/test-files/bootstrapFilesDirectory/" + bootstrapTimeStamp +
                                   "/artifacts/comms/MockComms-0");
            handle2 = receivedHandle;
            return PLUGIN_OK;
        });
    sdk.getCommsWrapper("MockComms-1")
        ->onLinkStatusChanged(handle, linkId, LINK_CREATED, bootstrapLinkProperties, RACE_BLOCKING);
    waitForCallbacks();

    EXPECT_CALL(*mockArtifactManager, acquirePlugin(_, _, _, _, _))
        .WillRepeatedly([](std::string destPath, std::string pluginName, std::string, std::string,
                           std::string) {
            fs::create_directory(destPath + "/" + pluginName);
            return true;
        });
    EXPECT_CALL(*mockBootstrapComms, serveFiles(linkId, _)).Times(0);
    EXPECT_CALL(*mockBootstrapComms, openConnection(_, _, _, _, _)).Times(0);
    EXPECT_CALL(*mockNM, onBootstrapFinished(_, BOOTSTRAP_FAILED))
        .Times(1)
        .WillOnce(::testing::Return(PLUGIN_OK));

    sdk.getNM()->bootstrapDevice(handle2, {channelGid, bootstrapChannelGid});
    waitForCallbacks();

    sdk.cleanShutdown();
}

////////////////////////////////////////////////////////////////
// User input
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, requestPluginUserInput_network_manager) {
    EXPECT_EQ(sdk.getNM()->requestPluginUserInput("key", "What?", false).status, SDK_OK);

    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
}

TEST_F(RaceSdkTestFixture, requestCommonUserInput_network_manager_valid_key) {
    EXPECT_EQ(sdk.getNM()->requestCommonUserInput("hostname").status, SDK_OK);

    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
}

TEST_F(RaceSdkTestFixture, requestCommonUserInput_network_manager_invalid_key) {
    EXPECT_EQ(sdk.getNM()->requestCommonUserInput("not-a-valid-user-input-key").status,
              SDK_INVALID_ARGUMENT);

    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
}

TEST_F(RaceSdkTestFixture, requestPluginUserInput_comms) {
    EXPECT_EQ(
        sdk.getCommsWrapper("MockComms-0")->requestPluginUserInput("key", "What?", false).status,
        SDK_OK);

    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
}

TEST_F(RaceSdkTestFixture, requestCommonUserInput_comms_valid_key) {
    EXPECT_EQ(sdk.getCommsWrapper("MockComms-0")->requestCommonUserInput("hostname").status,
              SDK_OK);

    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
}

TEST_F(RaceSdkTestFixture, requestCommonUserInput_comms_invalid_key) {
    EXPECT_EQ(sdk.getCommsWrapper("MockComms-0")
                  ->requestCommonUserInput("not-a-valid-user-input-key")
                  .status,
              SDK_INVALID_ARGUMENT);

    sdk.getCommsWrapper("MockComms-0")->waitForCallbacks();
    sdk.getNM()->waitForCallbacks();
}

TEST_F(RaceSdkTestFixture, onChannelStatusChanged_nonmatching_ChannelProperties) {
    ChannelProperties testChanProp;
    testChanProp.channelGid = channelGid;
    testChanProp.reliable = !channelProperties.reliable;
    EXPECT_EQ(sdk.onChannelStatusChanged(*sdk.getCommsWrapper("MockComms-0"), 0, channelGid,
                                         CHANNEL_UNAVAILABLE, testChanProp, 0)
                  .status,
              SDK_INVALID_ARGUMENT);
}

/**
 * @brief getLinksForChannel should return an empty vector if an invalid channelGid is supplied.
 *
 */
TEST_F(RaceSdkTestFixture, getLinksForChannel_invalid_channelGid_returns_empty_vector) {
    const std::vector<LinkID> result = sdk.getLinksForChannel("");

    EXPECT_EQ(result.size(), 0);
}

/**
 * @brief getLinksForChannel should return an empty vector if no links have been established by the
 * comms plugin.
 *
 */
TEST_F(RaceSdkTestFixture, getLinksForChannel_no_links_returns_empty_vector) {
    const std::vector<LinkID> result = sdk.getLinksForChannel(channelGid);

    EXPECT_EQ(result.size(), 0);
}

/**
 * @brief getLinksForChannel should return a vector of links that have been established by the comms
 * plugin.
 *
 */
TEST_F(RaceSdkTestFixture, getLinksForChannel_returns_links) {
    const LinkID linkId1 =
        createLinkForTesting(mockComms.get(), &sdk, "MockComms-0", channelGid, {"my persona"});
    std::vector<LinkID> expectedMockLinks = {linkId1};
    const std::vector<LinkID> mockLinks = sdk.getLinksForChannel(channelGid);
    EXPECT_THAT(mockLinks, ::testing::ContainerEq(expectedMockLinks));
}

/**
 * @brief RaceSdk::setChannelEnabled should return true unless client node tries to use a direct
 * channel
 *
 */
TEST_F(RaceSdkTestFixture, setChannelEnabled_clientDirectChannelFails) {
    ChannelProperties directProps, indirectProps;
    directProps.connectionType = CT_DIRECT;
    directProps.channelGid = "twoSixDirectCpp";
    indirectProps.connectionType = CT_INDIRECT;
    indirectProps.channelGid = "twoSixIndirectCpp";

    sdk.channels->add(directProps);
    sdk.channels->add(indirectProps);

    EXPECT_FALSE(sdk.setChannelEnabled("twoSixDirectCpp", true));
    EXPECT_TRUE(sdk.setChannelEnabled("twoSixIndirectCpp", true));
}

/**
 * @brief RaceSdk::setChannelEnabled should return false if the channel does not exist.
 *
 */
TEST_F(RaceSdkTestFixture, setChannelEnabled_returnsFalseChannelDoesNotExist) {
    EXPECT_FALSE(sdk.setChannelEnabled("someFakeChannelId", true));
}

////////////////////////////////////////////////////////////////
// getInitialEnabledChannels
////////////////////////////////////////////////////////////////

TEST_F(RaceSdkTestFixture, getInitialEnabledChannels_defaults_to_all_channnels) {
    EXPECT_EQ((std::vector<std::string>{"MockComms-0/channel1", "MockComms-0/channel2"}),
              sdk.getInitialEnabledChannels());
}

TEST_F(RaceSdkTestFixture, getInitialEnabledChannels_uses_explicit_config) {
    const_cast<RaceConfig &>(sdk.getRaceConfig())
        .initialEnabledChannels.push_back("MockComms-0/channel2");
    EXPECT_EQ((std::vector<std::string>{"MockComms-0/channel2"}), sdk.getInitialEnabledChannels());
}
