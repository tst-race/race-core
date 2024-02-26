
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

#include "../../../source/decomposed-comms/ComponentLifetimeManager.h"
#include "../../common/LogExpect.h"
#include "../../common/MockComponentManagerInternal.h"
#include "../../common/helpers.h"
#include "../../common/race_printers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;
using ::testing::Return;

class ComponentLifetimeManagerTestFixture : public ::testing::Test {
public:
    ComponentLifetimeManagerTestFixture() :
        test_info(testing::UnitTest::GetInstance()->current_test_info()),
        logger(test_info->test_suite_name(), test_info->name()),
        composition("id", "transport", "usermodel", {"encoding"}, {}, {}, {}),
        mockComponentManager(logger),
        transportPlugin("transport", logger),
        usermodelPlugin("usermodel", logger),
        encodingPlugin("encoding", logger),
        lifetimeManager(mockComponentManager, composition, transportPlugin, usermodelPlugin,
                        {{"encoding", &encodingPlugin}}) {}

    virtual void TearDown() override {
        logger.check();
        lifetimeManager.teardown();
    }

public:
    const testing::TestInfo *test_info;
    LogExpect logger;
    Composition composition;
    MockComponentManagerInternal mockComponentManager;
    MockComponentPlugin transportPlugin;
    MockComponentPlugin usermodelPlugin;
    MockComponentPlugin encodingPlugin;
    ComponentLifetimeManager lifetimeManager;
};

TEST_F(ComponentLifetimeManagerTestFixture, test_activate_channel) {
    lifetimeManager.state = CMTypes::State::UNACTIVATED;
    LOG_EXPECT(this->logger, __func__, lifetimeManager);
    lifetimeManager.activateChannel({0}, {1}, "id", "role");
    LOG_EXPECT(this->logger, __func__, lifetimeManager);
}

TEST_F(ComponentLifetimeManagerTestFixture, test_activate_channel_with_callbacks) {
    lifetimeManager.state = CMTypes::State::UNACTIVATED;
    EXPECT_CALL(mockComponentManager, setup()).WillOnce([this]() {
        LOG_EXPECT(this->logger, "mockComponentManager::setup", lifetimeManager);
        lifetimeManager.setup();
    });

    LOG_EXPECT(this->logger, __func__, lifetimeManager);
    lifetimeManager.activateChannel({0}, {1}, "id", "role");
    LOG_EXPECT(this->logger, __func__, lifetimeManager);
    lifetimeManager.updateState({1}, "encoding", COMPONENT_STATE_STARTED);
    lifetimeManager.updateState({2}, "usermodel", COMPONENT_STATE_STARTED);
    lifetimeManager.updateState({3}, "transport", COMPONENT_STATE_STARTED);
    LOG_EXPECT(this->logger, __func__, lifetimeManager);
}