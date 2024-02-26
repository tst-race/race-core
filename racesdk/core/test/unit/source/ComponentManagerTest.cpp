
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

#include "../../../source/decomposed-comms/ComponentManager.h"
#include "../../../source/plugin-loading/IComponentPlugin.h"
#include "../../common/LogExpect.h"
#include "../../common/MockComponentPlugin.h"
#include "../../common/MockRaceSdkComms.h"
#include "../../common/helpers.h"
#include "../../common/race_printers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;
using ::testing::Return;

class TestableComponentManager : public ComponentManager {
public:
    using ComponentManager::ComponentManager;
    using ComponentManager::manager;
};

class ComponentManagerTestFixture : public ::testing::Test {
public:
    ComponentManagerTestFixture() :
        test_info(testing::UnitTest::GetInstance()->current_test_info()),
        logger(test_info->test_suite_name(), test_info->name()),
        sdk(logger),
        composition("composition_id", "transport_id", "usermodel_id", {"encoding_id"},
                    RaceEnums::NT_CLIENT, "linux", "x86-64"),
        transportPlugin("transportPlugin", logger),
        usermodelPlugin("usermodelPlugin", logger),
        encodingPlugin("encodingPlugin", logger),
        manager(sdk, composition, transportPlugin, usermodelPlugin,
                std::unordered_map<std::string, IComponentPlugin *>{{"encoder", &encodingPlugin}}) {
    }

    virtual ~ComponentManagerTestFixture(){};
    virtual void SetUp() override {}
    virtual void TearDown() override {
        manager.shutdown();
        manager.waitForCallbacks();
        logger.check();
    }

    void activateChannels() {
        manager.init({});
        manager.activateChannel(42, composition.id, "some role name");
        manager.waitForCallbacks();
        transportPlugin.transport->sdk.updateState(ComponentState::COMPONENT_STATE_STARTED);
        usermodelPlugin.userModel->sdk.updateState(ComponentState::COMPONENT_STATE_STARTED);
        encodingPlugin.encoding->sdk.updateState(ComponentState::COMPONENT_STATE_STARTED);
        manager.waitForCallbacks();
    }

    LinkID createLink() {
        LinkID newLinkId;
        std::promise<void> wait;
        EXPECT_CALL(*transportPlugin.transport, createLink(_, _))
            .WillOnce([this, &newLinkId](RaceHandle handle, const LinkID &linkId) {
                LOG_EXPECT(this->logger, "createLink", handle, linkId);
                transportPlugin.transport->sdk.onLinkStatusChanged(handle, linkId, LINK_CREATED,
                                                                   {});
                newLinkId = linkId;
                return ComponentStatus::COMPONENT_OK;
            });
        EXPECT_CALL(*usermodelPlugin.userModel, addLink(_, _))
            .WillOnce([this, &wait](const LinkID &linkId, const LinkParameters &params) {
                LOG_EXPECT(this->logger, "addLink", linkId, params);
                wait.set_value();
                return ComponentStatus::COMPONENT_OK;
            });
        manager.createLink(43, composition.id);
        wait.get_future().wait();
        manager.waitForCallbacks();
        return newLinkId;
    }

    ConnectionID createConnection(LinkID linkId) {
        ConnectionID newConnId;
        // There may be a future CONNECTION_CLOSED call. Allow it by setting the base case to
        // AnyNumber.
        EXPECT_CALL(sdk, onConnectionStatusChanged(_, _, _, _, _)).Times(testing::AnyNumber());
        EXPECT_CALL(sdk, onConnectionStatusChanged(_, _, CONNECTION_OPEN, _, _))
            .WillOnce([&newConnId](RaceHandle /* handle */, ConnectionID connId,
                                   ConnectionStatus /* status */, LinkProperties /* properties */,
                                   int32_t /* timeout */) {
                newConnId = connId;
                return SDK_OK;
            });
        manager.openConnection(44, LT_BIDI, linkId, "", RACE_UNLIMITED);
        manager.waitForCallbacks();
        return newConnId;
    }

    void log_expect(std::string name) {
        manager.waitForCallbacks();
        LOG_EXPECT(this->logger, name, manager);
    }

public:
    const testing::TestInfo *test_info;
    LogExpect logger;

    MockRaceSdkComms sdk;
    Composition composition;
    MockComponentPlugin transportPlugin;
    MockComponentPlugin usermodelPlugin;
    MockComponentPlugin encodingPlugin;
    TestableComponentManager manager;
};

TEST_F(ComponentManagerTestFixture, test_constructor) {
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_init) {
    log_expect(__func__);
    manager.init({});
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_shutdown) {
    log_expect(__func__);
    manager.shutdown();
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_activate) {
    manager.init({});
    log_expect(__func__);
    manager.activateChannel(42, composition.id, "some role name");
    manager.waitForCallbacks();
    transportPlugin.transport->sdk.updateState(ComponentState::COMPONENT_STATE_STARTED);
    usermodelPlugin.userModel->sdk.updateState(ComponentState::COMPONENT_STATE_STARTED);
    encodingPlugin.encoding->sdk.updateState(ComponentState::COMPONENT_STATE_STARTED);
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_activate2) {
    manager.init({});
    log_expect(__func__);
    EXPECT_CALL(transportPlugin, createTransport(_, _, _, _))
        .WillOnce([this](std::string name, ITransportSdk *sdk, std::string roleName,
                         PluginConfig pluginConfig) {
            LOG_EXPECT(this->logger, this->transportPlugin.id + ".createTransport", name, roleName,
                       pluginConfig);
            this->transportPlugin.transport = std::make_shared<MockTransport>(this->logger, *sdk);
            transportPlugin.transport->sdk.updateState(ComponentState::COMPONENT_STATE_STARTED);
            return transportPlugin.transport;
        });
    EXPECT_CALL(usermodelPlugin, createUserModel(_, _, _, _))
        .WillOnce([this](std::string name, IUserModelSdk *sdk, std::string roleName,
                         PluginConfig pluginConfig) {
            LOG_EXPECT(this->logger, this->usermodelPlugin.id + ".createUserModel", name, roleName,
                       pluginConfig);
            usermodelPlugin.userModel = std::make_shared<MockUserModel>(this->logger, *sdk);
            usermodelPlugin.userModel->sdk.updateState(ComponentState::COMPONENT_STATE_STARTED);
            return usermodelPlugin.userModel;
        });
    EXPECT_CALL(encodingPlugin, createEncoding(_, _, _, _))
        .WillOnce([this](std::string name, IEncodingSdk *sdk, std::string roleName,
                         PluginConfig pluginConfig) {
            LOG_EXPECT(this->logger, this->encodingPlugin.id + ".createEncoding", name, roleName,
                       pluginConfig);
            encodingPlugin.encoding = std::make_shared<MockEncoding>(this->logger, *sdk);
            encodingPlugin.encoding->sdk.updateState(ComponentState::COMPONENT_STATE_STARTED);
            return encodingPlugin.encoding;
        });
    manager.activateChannel(42, composition.id, "some role name");
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_deactivate) {
    manager.init({});
    manager.activateChannel(42, composition.id, "some role name");
    log_expect(__func__);
    manager.deactivateChannel(43, composition.id);
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_create_link) {
    activateChannels();
    log_expect(__func__);
    std::promise<void> wait;
    EXPECT_CALL(*transportPlugin.transport, createLink(_, _))
        .WillOnce([this](RaceHandle handle, const LinkID &linkId) {
            LOG_EXPECT(this->logger, "createLink", handle, linkId);
            transportPlugin.transport->sdk.onLinkStatusChanged(handle, linkId, LINK_CREATED, {});
            return ComponentStatus::COMPONENT_OK;
        });
    EXPECT_CALL(*usermodelPlugin.userModel, addLink(_, _))
        .WillOnce([this, &wait](const LinkID &linkId, const LinkParameters &params) {
            LOG_EXPECT(this->logger, "addLink", linkId, params);
            wait.set_value();
            return ComponentStatus::COMPONENT_OK;
        });
    manager.createLink(43, composition.id);
    wait.get_future().wait();
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_load_link_address) {
    activateChannels();
    log_expect(__func__);
    std::promise<void> wait;
    EXPECT_CALL(*transportPlugin.transport, loadLinkAddress(_, _, _))
        .WillOnce(
            [this](RaceHandle handle, const LinkID &linkId, const std::string & /* linkAddress */) {
                LOG_EXPECT(this->logger, "loadLinkAddress", handle, linkId);
                transportPlugin.transport->sdk.onLinkStatusChanged(handle, linkId, LINK_LOADED, {});
                return ComponentStatus::COMPONENT_OK;
            });
    EXPECT_CALL(*usermodelPlugin.userModel, addLink(_, _))
        .WillOnce([this, &wait](const LinkID &linkId, const LinkParameters &params) {
            LOG_EXPECT(this->logger, "addLink", linkId, params);
            wait.set_value();
            return ComponentStatus::COMPONENT_OK;
        });
    manager.loadLinkAddress(43, composition.id, "link address");
    wait.get_future().wait();
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_create_link_from_address) {
    activateChannels();
    log_expect(__func__);
    std::promise<void> wait;
    EXPECT_CALL(*transportPlugin.transport, createLinkFromAddress(_, _, _))
        .WillOnce([this](RaceHandle handle, const LinkID &linkId,
                         const std::string & /* linkAddress */) {
            LOG_EXPECT(this->logger, "createLinkFromAddress", handle, linkId);
            transportPlugin.transport->sdk.onLinkStatusChanged(handle, linkId, LINK_CREATED, {});
            return ComponentStatus::COMPONENT_OK;
        });
    EXPECT_CALL(*usermodelPlugin.userModel, addLink(_, _))
        .WillOnce([this, &wait](const LinkID &linkId, const LinkParameters &params) {
            LOG_EXPECT(this->logger, "addLink", linkId, params);
            wait.set_value();
            return ComponentStatus::COMPONENT_OK;
        });
    manager.createLinkFromAddress(43, composition.id, "link address");
    wait.get_future().wait();
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_destroy_link) {
    activateChannels();
    LinkID linkId = createLink();
    log_expect(__func__);
    manager.destroyLink(44, linkId);
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_open_connection) {
    activateChannels();
    LinkID linkId = createLink();
    log_expect(__func__);
    manager.openConnection(44, LT_BIDI, linkId, "", RACE_UNLIMITED);
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_close_connection) {
    activateChannels();
    LinkID linkId = createLink();
    ConnectionID connId = createConnection(linkId);
    log_expect(__func__);
    manager.closeConnection(45, connId);
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_send_package) {
    manager.manager.mode = CMTypes::EncodingMode::SINGLE;
    activateChannels();
    LinkID linkId = createLink();
    ConnectionID connId = createConnection(linkId);
    EXPECT_CALL(*usermodelPlugin.userModel, getTimeline(_, _))
        .WillOnce([this](Timestamp startTime, Timestamp endTime) {
            auto start = "<Timestamp>";
            auto range = endTime - startTime;
            LOG_EXPECT(this->logger, "getTimeline", start, range);
            Action action;
            action.actionId = 1;
            action.json = "";
            // The action should not happen during this test. Schedule it for 5 minutes from now.
            action.timestamp = helper::currentTime() + 300;
            return ActionTimeline{action};
        });
    EXPECT_CALL(*transportPlugin.transport, getActionParams(_))
        .WillOnce([this, &linkId](const Action &action) {
            LOG_EXPECT(this->logger, "getActionParams", action);
            EncodingParameters params;
            params.encodePackage = true;
            params.linkId = linkId;
            params.type = "*/*";
            return std::vector<EncodingParameters>{params};
        });
    usermodelPlugin.userModel->sdk.onTimelineUpdated();
    manager.waitForCallbacks();
    EncPkg pkg(0, 0, {1, 2, 3, 4, 5, 6, 7, 8, 9, 0});
    log_expect(__func__);
    manager.sendPackage(45, connId, pkg, RACE_UNLIMITED, 0);
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_request_plugin_user_input) {
    // Don't use activateChannel() to be sure it still works while components are starting up.
    manager.init({});
    manager.activateChannel(42, composition.id, "some role name");
    manager.waitForCallbacks();

    // TODO: use non-null handle
    log_expect(__func__);
    auto response = transportPlugin.transport->sdk.requestPluginUserInput("key", "prompt", true);
    LOG_EXPECT(this->logger, "requestPluginUserInput response", response);
    log_expect(__func__);
    manager.onUserInputReceived(0, true, "answer");
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_request_common_user_input) {
    // Don't use activateChannel() to be sure it still works while components are starting up.
    manager.init({});
    manager.activateChannel(42, composition.id, "some role name");
    manager.waitForCallbacks();

    // TODO: use non-null handle
    log_expect(__func__);
    auto response = transportPlugin.transport->sdk.requestCommonUserInput("key");
    LOG_EXPECT(this->logger, "requestCommonUserInput response", response);
    log_expect(__func__);
    manager.onUserInputReceived(0, true, "answer");
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_on_event) {
    Event event;
    event.json = "{\"key\": \"value\"}";
    activateChannels();
    transportPlugin.transport->sdk.onEvent(event);
}

TEST_F(ComponentManagerTestFixture, test_on_receive_bad_param_type) {
    activateChannels();
    LinkID linkId = createLink();
    ConnectionID connId = createConnection(linkId);
    log_expect(__func__);

    EncodingParameters params;
    params.linkId = {};
    params.type = "";
    params.encodePackage = {};
    params.json = {};

    transportPlugin.transport->sdk.onReceive(linkId, params,
                                             {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8});
    log_expect(__func__);
}

TEST_F(ComponentManagerTestFixture, test_on_receive) {
    manager.manager.mode = CMTypes::EncodingMode::SINGLE;
    activateChannels();
    LinkID linkId = createLink();
    ConnectionID connId = createConnection(linkId);
    log_expect(__func__);
    RaceHandle handle;
    EXPECT_CALL(*encodingPlugin.encoding, decodeBytes(_, _, _))
        .WillOnce([this, &handle](RaceHandle decodeHandle, const EncodingParameters &params,
                                  const std::vector<uint8_t> &bytes) {
            LOG_EXPECT(this->logger, "decodeBytes", decodeHandle, params, bytes.size());
            handle = decodeHandle;
            return ComponentStatus::COMPONENT_OK;
        });

    EncodingParameters params;
    params.linkId = {};
    params.type = "*/*";
    params.encodePackage = {};
    params.json = {};

    transportPlugin.transport->sdk.onReceive(linkId, params,
                                             {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8});
    log_expect(__func__);
    encodingPlugin.encoding->sdk.onBytesDecoded(handle, {0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9},
                                                ENCODE_OK);
    log_expect(__func__);
}
