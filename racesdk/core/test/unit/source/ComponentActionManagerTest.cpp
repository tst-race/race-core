
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

#include "../../../source/decomposed-comms/ComponentActionManager.h"
#include "../../common/LogExpect.h"
#include "../../common/MockComponentManagerInternal.h"
#include "../../common/helpers.h"
#include "../../common/race_printers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;
using ::testing::Return;

class TestableComponentActionManager : public ComponentActionManager {
public:
    using ComponentActionManager::ComponentActionManager;
    using ComponentActionManager::updateGlobalTimeline;
    using ComponentActionManager::updateTimeline;

    TestableComponentActionManager(ComponentManagerInternal &cm) : ComponentActionManager(cm) {
        maxEncodingTime = 0.1;
    }

    bool testActionThreadLogic(Timestamp now) {
        return actionThreadLogic(now);
    }

    void testUpdateActionTimestamp() {
        updateActionTimestamp();
    }

    void testUpdateEncodeTimestamp() {
        updateEncodeTimestamp();
    }

protected:
    double currentTime() override {
        return nextTime;
    }

public:
    double nextTime = 0;
};

class ComponentActionManagerTestFixture : public ::testing::Test {
public:
    ComponentActionManagerTestFixture() :
        test_info(testing::UnitTest::GetInstance()->current_test_info()),
        logger(test_info->test_suite_name(), test_info->name()),
        mockComponentManager(logger),
        actionManager(mockComponentManager) {}

    virtual void TearDown() override {
        logger.check();
    }

public:
    const testing::TestInfo *test_info;
    LogExpect logger;
    MockComponentManagerInternal mockComponentManager;
    TestableComponentActionManager actionManager;
};

TEST_F(ComponentActionManagerTestFixture, test_constructor) {
    LOG_EXPECT(this->logger, __func__, actionManager);
}

TEST_F(ComponentActionManagerTestFixture, test_onTimelineUpdated) {
    LOG_EXPECT(this->logger, __func__, actionManager);
    actionManager.nextTime = 1000;
    actionManager.onTimelineUpdated(CMTypes::ComponentWrapperHandle{1});
    LOG_EXPECT(this->logger, __func__, actionManager);
}

TEST_F(ComponentActionManagerTestFixture, test_updateGlobalTimeline_keep_actions_before_start) {
    actionManager.actions.push_back(std::make_unique<CMTypes::ActionInfo>(
        CMTypes::ActionInfo{{0.0, 0, {}}, false, "", {}, {}, false}));
    ActionTimeline newActions = {};
    LOG_EXPECT(this->logger, __func__, actionManager);
    actionManager.updateGlobalTimeline(newActions, 1.0);
    LOG_EXPECT(this->logger, __func__, actionManager);
}

TEST_F(ComponentActionManagerTestFixture, test_updateGlobalTimeline_delete_actions_after_end) {
    actionManager.actions.push_back(std::make_unique<CMTypes::ActionInfo>(
        CMTypes::ActionInfo{{2.0, 0, {}}, false, "", {}, {}, false}));
    ActionTimeline newActions = {};
    LOG_EXPECT(this->logger, __func__, actionManager);
    actionManager.updateGlobalTimeline(newActions, 1.0);
    LOG_EXPECT(this->logger, __func__, actionManager);
}

TEST_F(ComponentActionManagerTestFixture, test_updateGlobalTimeline_add_new_actions_after_end) {
    ActionTimeline newActions = {{2.0, 0, {}}};
    LOG_EXPECT(this->logger, __func__, actionManager);
    actionManager.updateGlobalTimeline(newActions, 1.0);
    LOG_EXPECT(this->logger, __func__, actionManager);
}

TEST_F(ComponentActionManagerTestFixture, test_updateGlobalTimeline_keep_action_in_both) {
    actionManager.actions.push_back(std::make_unique<CMTypes::ActionInfo>(
        CMTypes::ActionInfo{{2.0, 0, {}}, false, "", {}, {}, false}));
    ActionTimeline newActions = {{2.0, 0, {}}};
    LOG_EXPECT(this->logger, __func__, actionManager);
    actionManager.updateGlobalTimeline(newActions, 1.0);
    LOG_EXPECT(this->logger, __func__, actionManager);
}

TEST_F(ComponentActionManagerTestFixture, test_updateGlobalTimeline_add_action_in_middle) {
    actionManager.actions.push_back(std::make_unique<CMTypes::ActionInfo>(
        CMTypes::ActionInfo{{3.0, 1, {}}, false, "", {}, {}, false}));
    ActionTimeline newActions = {{2.0, 2, {}}, {3.0, 1, {}}};
    LOG_EXPECT(this->logger, __func__, actionManager);
    actionManager.updateGlobalTimeline(newActions, 1.0);
    LOG_EXPECT(this->logger, __func__, actionManager);
}

TEST_F(ComponentActionManagerTestFixture, test_updateGlobalTimeline_remove_action_in_middle) {
    actionManager.actions.push_back(std::make_unique<CMTypes::ActionInfo>(
        CMTypes::ActionInfo{{2.0, 2, {}}, false, "", {}, {}, false}));
    actionManager.actions.push_back(std::make_unique<CMTypes::ActionInfo>(
        CMTypes::ActionInfo{{3.0, 1, {}}, false, "", {}, {}, false}));
    ActionTimeline newActions = {{3.0, 1, {}}};
    LOG_EXPECT(this->logger, __func__, actionManager);
    actionManager.updateGlobalTimeline(newActions, 1.0);
    LOG_EXPECT(this->logger, __func__, actionManager);
}

TEST_F(ComponentActionManagerTestFixture, test_onSendPackage) {
    double now = 2.0;
    EXPECT_CALL(mockComponentManager.usermodel, onSendPackage(_, _))
        .WillOnce([this, now](const LinkID &linkId, int bytes) {
            LOG_EXPECT(this->logger, "onSendPackage", linkId, bytes);
            return ActionTimeline{{0.0, 1, {}}, {now + 1, 2, {}}};
        });

    EncPkg pkg(0, 0, {0, 1, 2, 3, 4, 5, 6, 7});
    LOG_EXPECT(this->logger, __func__, actionManager);
    actionManager.onSendPackage(now, "mockConnId", pkg);
    LOG_EXPECT(this->logger, __func__, actionManager);
}

TEST_F(ComponentActionManagerTestFixture, test_onSendPackage_existing_actions) {
    actionManager.actions.push_back(std::make_unique<CMTypes::ActionInfo>(
        CMTypes::ActionInfo{{1.0, 2, {}}, false, "", {}, {}, false}));
    actionManager.actions.push_back(std::make_unique<CMTypes::ActionInfo>(
        CMTypes::ActionInfo{{4.0, 1, {}}, false, "", {}, {}, false}));
    double now = 2.0;
    EXPECT_CALL(mockComponentManager.usermodel, onSendPackage(_, _))
        .WillOnce([this](const LinkID &linkId, int bytes) {
            LOG_EXPECT(this->logger, "onSendPackage", linkId, bytes);
            return ActionTimeline{{0.0, 4, {}}, {3.0, 5, {}}, {5.0, 6, {}}};
        });

    EncPkg pkg(0, 0, {0, 1, 2, 3, 4, 5, 6, 7});
    LOG_EXPECT(this->logger, __func__, actionManager);
    actionManager.onSendPackage(now, "mockConnId", pkg);
    LOG_EXPECT(this->logger, __func__, actionManager);
}

TEST_F(ComponentActionManagerTestFixture, test_updateTimeline) {
    using testing::Field;
    Action action1{3.0, 1, {}};
    Action action2{4.0, 2, {}};
    Action action3{5.0, 3, {}};
    ActionTimeline newActions = {action1, action2, action3};
    EXPECT_CALL(mockComponentManager.transport, getActionParams(Field(&Action::actionId, 1)))
        .WillOnce(Return(
            std::vector<EncodingParameters>{EncodingParameters{"mockLinkId", "*/*", true, "{}"}}));
    EXPECT_CALL(mockComponentManager.transport, getActionParams(Field(&Action::actionId, 2)))
        .WillOnce(
            Return(std::vector<EncodingParameters>{EncodingParameters{"*", "*/*", true, "{}"}}));
    EXPECT_CALL(mockComponentManager.transport, getActionParams(Field(&Action::actionId, 3)))
        .WillOnce(Return(
            std::vector<EncodingParameters>{EncodingParameters{"mockLinkId", "*/*", true, "{}"}}));
    LOG_EXPECT(this->logger, __func__, actionManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink2);
    actionManager.updateTimeline(newActions, 1.0);
    LOG_EXPECT(this->logger, __func__, actionManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink2);
}

TEST_F(ComponentActionManagerTestFixture, test_actionThreadLogic_stop) {
    LOG_EXPECT(this->logger, __func__, actionManager);
    actionManager.nextTime = 1000;
    LOG_EXPECT(this->logger, __func__, actionManager.testActionThreadLogic(1));
}

TEST_F(ComponentActionManagerTestFixture, test_actionThreadLogic_fetch) {
    EXPECT_CALL(mockComponentManager, getState()).WillOnce(Return(CMTypes::State::ACTIVATED));
    LOG_EXPECT(this->logger, __func__, actionManager);
    LOG_EXPECT(this->logger, __func__, actionManager.testActionThreadLogic(1));
    LOG_EXPECT(this->logger, __func__, actionManager);
}

TEST_F(ComponentActionManagerTestFixture, test_actionThreadLogic_action) {
    EXPECT_CALL(mockComponentManager, getState()).WillOnce(Return(CMTypes::State::ACTIVATED));
    EXPECT_CALL(mockComponentManager, getPackageHandlesForAction(_))
        .WillOnce([this](CMTypes::ActionInfo *info) {
            LOG_EXPECT(this->logger, "getPackageHandlesForAction", *info)
            return std::vector<CMTypes::PackageFragmentHandle>{CMTypes::PackageFragmentHandle{5},
                                                               CMTypes::PackageFragmentHandle{8},
                                                               CMTypes::PackageFragmentHandle{13}};
        });
    actionManager.nextFetchTime = 2000;
    actionManager.nextTime = 1000;

    auto action = std::make_unique<CMTypes::ActionInfo>();
    action->action.timestamp = 1000;
    actionManager.actions.push_back(std::move(action));
    auto action2 = std::make_unique<CMTypes::ActionInfo>();
    action2->action.timestamp = 1300;
    actionManager.actions.push_back(std::move(action2));
    actionManager.testUpdateActionTimestamp();
    LOG_EXPECT(this->logger, __func__, actionManager);
    LOG_EXPECT(this->logger, __func__, actionManager.testActionThreadLogic(1000));
    LOG_EXPECT(this->logger, __func__, actionManager);
}

TEST_F(ComponentActionManagerTestFixture, test_actionThreadLogic_encode) {
    EXPECT_CALL(mockComponentManager, getState()).WillOnce(Return(CMTypes::State::ACTIVATED));
    actionManager.maxEncodingTime = 0.1;
    actionManager.nextFetchTime = 2000;
    actionManager.nextTime = 1000;
    auto action = std::make_unique<CMTypes::ActionInfo>();
    action->action.timestamp = 1000;
    actionManager.actions.push_back(std::move(action));
    auto action2 = std::make_unique<CMTypes::ActionInfo>();
    action2->action.timestamp = 1300;
    actionManager.actions.push_back(std::move(action2));
    actionManager.testUpdateEncodeTimestamp();
    LOG_EXPECT(this->logger, __func__, actionManager);
    LOG_EXPECT(this->logger, __func__, actionManager.testActionThreadLogic(1000));
    LOG_EXPECT(this->logger, __func__, actionManager);
}
