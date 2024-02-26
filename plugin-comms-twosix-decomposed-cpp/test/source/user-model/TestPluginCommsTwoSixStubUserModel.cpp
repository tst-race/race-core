
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <race/mocks/MockUserModelSdk.h>

#include "JsonTypes.h"
#include "LinkUserModel.h"
#include "PluginCommsTwoSixStubUserModel.h"

class MockLinkUserModel : public LinkUserModel {
public:
    using LinkUserModel::LinkUserModel;
    MOCK_METHOD(ActionTimeline, getTimeline, (Timestamp start, Timestamp end), (override));
};

class UserModelToTest : public PluginCommsTwoSixStubUserModel {
public:
    std::unordered_map<LinkID, std::shared_ptr<MockLinkUserModel>> &mockLinkUserModels;
    std::atomic<uint64_t> nextActionId{0};

    UserModelToTest(
        IUserModelSdk *sdk,
        std::unordered_map<LinkID, std::shared_ptr<MockLinkUserModel>> &mockLinkUserModels) :
        PluginCommsTwoSixStubUserModel(sdk), mockLinkUserModels(mockLinkUserModels) {}

    std::shared_ptr<LinkUserModel> createLinkUserModel(const LinkID &linkId) override {
        auto model = std::make_shared<MockLinkUserModel>(linkId, nextActionId);
        mockLinkUserModels[linkId] = model;
        return model;
    }
};

class TestPluginCommsTwoSixUserModel : public ::testing::Test {
public:
    ::testing::NiceMock<MockUserModelSdk> sdk;
    std::unordered_map<LinkID, std::shared_ptr<MockLinkUserModel>> mockLinkUserModels;
    UserModelToTest userModel{&sdk, mockLinkUserModels};
    ActionTimeline timeline;

    void verifyAction(size_t index, Timestamp expectedTimestamp, uint64_t expectedActionId,
                      const LinkID &expectedLinkId, ActionType expectedAction) {
        auto &action = timeline.at(index);
        EXPECT_NEAR(expectedTimestamp, action.timestamp, 0.001) << "for action at index " << index;
        EXPECT_EQ(expectedActionId, action.actionId);
        ActionJson actionJson = nlohmann::json::parse(action.json);
        EXPECT_EQ(expectedLinkId, actionJson.linkId) << "for action at index " << index;
        EXPECT_EQ(expectedAction, actionJson.type) << "for action at index " << index;
    }

    Action createAction(Timestamp timestamp, uint64_t actionId, const LinkID &linkId,
                        ActionType actionType) {
        Action action;
        action.timestamp = timestamp;
        action.actionId = actionId;

        ActionJson actionJson;
        actionJson.linkId = linkId;
        actionJson.type = actionType;
        action.json = nlohmann::json(actionJson).dump();

        return action;
    }
};

TEST_F(TestPluginCommsTwoSixUserModel,
       should_notify_sdk_to_update_timeline_after_adding_and_removing_link) {
    EXPECT_CALL(sdk, onTimelineUpdated());
    userModel.addLink("LinkID_1", {});

    ::testing::Mock::VerifyAndClearExpectations(&sdk);

    EXPECT_CALL(sdk, onTimelineUpdated());
    userModel.removeLink("LinkID_1");
}

TEST_F(TestPluginCommsTwoSixUserModel, should_generate_empty_timeline_when_no_links) {
    timeline = userModel.getTimeline(1000.0, 1004.0);
    ASSERT_EQ(0, timeline.size());
}

TEST_F(TestPluginCommsTwoSixUserModel, should_generate_timeline_with_only_one_link) {
    userModel.addLink("LinkID_1", {});
    EXPECT_CALL(*mockLinkUserModels["LinkID_1"], getTimeline(1000.0, 1004.0))
        .WillOnce(::testing::Return(ActionTimeline({
            createAction(1001.0, 1, "LinkID_1", ACTION_FETCH),
            createAction(1003.0, 2, "LinkID_1", ACTION_POST),
        })));

    timeline = userModel.getTimeline(1000.0, 1004.0);
    ASSERT_EQ(2, timeline.size());
    verifyAction(0, 1001.0, 1, "LinkID_1", ACTION_FETCH);
    verifyAction(1, 1003.0, 2, "LinkID_1", ACTION_POST);
}

TEST_F(TestPluginCommsTwoSixUserModel, should_generate_timeline_with_multiple_links) {
    userModel.addLink("LinkID_1", {});
    EXPECT_CALL(*mockLinkUserModels["LinkID_1"], getTimeline(900.0, 1100.0))
        .WillOnce(::testing::Return(ActionTimeline({
            createAction(1001.0, 1, "LinkID_1", ACTION_FETCH),
            createAction(1003.0, 2, "LinkID_1", ACTION_POST),
        })));

    userModel.addLink("LinkID_2", {});
    EXPECT_CALL(*mockLinkUserModels["LinkID_2"], getTimeline(900.0, 1100.0))
        .WillOnce(::testing::Return(ActionTimeline({
            createAction(1000.0, 3, "LinkID_2", ACTION_POST),
            createAction(1001.0, 4, "LinkID_2", ACTION_FETCH),
            createAction(1004.0, 5, "LinkID_2", ACTION_POST),
        })));

    timeline = userModel.getTimeline(900.0, 1100.0);
    ASSERT_EQ(5, timeline.size());
    verifyAction(0, 1000.0, 3, "LinkID_2", ACTION_POST);
    verifyAction(1, 1001.0, 1, "LinkID_1", ACTION_FETCH);
    verifyAction(2, 1001.0, 4, "LinkID_2", ACTION_FETCH);
    verifyAction(3, 1003.0, 2, "LinkID_1", ACTION_POST);
    verifyAction(4, 1004.0, 5, "LinkID_2", ACTION_POST);
}

TEST_F(TestPluginCommsTwoSixUserModel, should_not_include_removed_link_in_generated_timeline) {
    userModel.addLink("LinkID_1", {});
    EXPECT_CALL(*mockLinkUserModels["LinkID_1"], getTimeline(900.0, 1100.0))
        .WillOnce(::testing::Return(ActionTimeline({
            createAction(1001.0, 1, "LinkID_1", ACTION_FETCH),
        })));
    EXPECT_CALL(*mockLinkUserModels["LinkID_1"], getTimeline(1100.0, 1300.0)).Times(0);

    userModel.addLink("LinkID_2", {});
    EXPECT_CALL(*mockLinkUserModels["LinkID_2"], getTimeline(900.0, 1100.0))
        .WillOnce(::testing::Return(ActionTimeline({
            createAction(1000.0, 2, "LinkID_2", ACTION_POST),
        })));
    EXPECT_CALL(*mockLinkUserModels["LinkID_2"], getTimeline(1100.0, 1300.0))
        .WillOnce(::testing::Return(ActionTimeline({
            createAction(1400.0, 3, "LinkID_2", ACTION_FETCH),
        })));

    timeline = userModel.getTimeline(900.0, 1100.0);
    ASSERT_EQ(2, timeline.size());
    verifyAction(0, 1000.0, 2, "LinkID_2", ACTION_POST);
    verifyAction(1, 1001.0, 1, "LinkID_1", ACTION_FETCH);

    userModel.removeLink("LinkID_1");

    timeline = userModel.getTimeline(1100.0, 1300.0);
    ASSERT_EQ(1, timeline.size());
    verifyAction(0, 1400.0, 3, "LinkID_2", ACTION_FETCH);
}

// TODO remove when immutable-first-action requirement is removed
TEST_F(TestPluginCommsTwoSixUserModel, should_offset_added_link_into_regenerated_timeline) {
    userModel.addLink("LinkID_1", {});
    EXPECT_CALL(*mockLinkUserModels["LinkID_1"], getTimeline(1000.0, 2000.0))
        .WillOnce(::testing::Return(ActionTimeline({
            createAction(1100.0, 1, "LinkID_1", ACTION_FETCH),
            createAction(1500.0, 2, "LinkID_1", ACTION_POST),
        })));
    EXPECT_CALL(*mockLinkUserModels["LinkID_1"], getTimeline(1500.0, 2500.0))
        .WillOnce(::testing::Return(ActionTimeline({
            createAction(1500.0, 2, "LinkID_1", ACTION_POST),
            createAction(2100.0, 3, "LinkID_1", ACTION_FETCH),
        })));

    timeline = userModel.getTimeline(1000.0, 2000.0);
    ASSERT_EQ(2, timeline.size());
    verifyAction(0, 1100.0, 1, "LinkID_1", ACTION_FETCH);
    verifyAction(1, 1500.0, 2, "LinkID_1", ACTION_POST);

    userModel.addLink("LinkID_2", {});
    EXPECT_CALL(*mockLinkUserModels["LinkID_2"], getTimeline(1501.0, 2500.0))
        .WillOnce(::testing::Return(ActionTimeline({
            createAction(1501.0, 4, "LinkID_2", ACTION_FETCH),
            createAction(2100.0, 5, "LinkID_2", ACTION_POST),
        })));

    timeline = userModel.getTimeline(1500.0, 2500.0);
    ASSERT_EQ(4, timeline.size());
    verifyAction(0, 1500.0, 2, "LinkID_1", ACTION_POST);
    verifyAction(1, 1501.0, 4, "LinkID_2", ACTION_FETCH);
    verifyAction(2, 2100.0, 3, "LinkID_1", ACTION_FETCH);
    verifyAction(3, 2100.0, 5, "LinkID_2", ACTION_POST);
}
