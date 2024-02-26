
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

#include "JsonTypes.h"
#include "LinkUserModel.h"

class LinkUserModelToTest : public LinkUserModel {
public:
    using LinkUserModel::LinkUserModel;
    MOCK_METHOD(MarkovModel::UserAction, getNextUserAction, (), (override));
};

class TestLinkUserModel : public ::testing::Test {
public:
    std::atomic<uint64_t> nextActionId{0};
    LinkUserModelToTest model{"LinkID", nextActionId};
    ActionTimeline timeline;

    void verifyAction(size_t index, Timestamp expectedTimestamp, uint64_t expectedActionId,
                      ActionType expectedAction) {
        auto &action = timeline.at(index);
        EXPECT_NEAR(expectedTimestamp, action.timestamp, 0.001) << "for action at index " << index;
        EXPECT_EQ(expectedActionId, action.actionId) << "for action at index " << index;
        ActionJson actionJson = nlohmann::json::parse(action.json);
        EXPECT_EQ("LinkID", actionJson.linkId) << "for action at index " << index;
        EXPECT_EQ(expectedAction, actionJson.type) << "for action at index " << index;
    }
};

TEST_F(TestLinkUserModel, should_generate_single_timeline) {
    EXPECT_CALL(model, getNextUserAction())
        .WillOnce(::testing::Return(MarkovModel::UserAction::FETCH))  // 1000
        .WillOnce(::testing::Return(MarkovModel::UserAction::WAIT))   // 1010
        .WillOnce(::testing::Return(MarkovModel::UserAction::FETCH))
        .WillOnce(::testing::Return(MarkovModel::UserAction::POST))
        .WillOnce(::testing::Return(MarkovModel::UserAction::WAIT))  // 1020
        .WillOnce(::testing::Return(MarkovModel::UserAction::WAIT))  // 1030
        .WillOnce(::testing::Return(MarkovModel::UserAction::POST))
        .WillOnce(::testing::Return(MarkovModel::UserAction::WAIT));  // 1040

    timeline = model.getTimeline(1000.0, 1040.0);
    ASSERT_EQ(4, timeline.size());
    verifyAction(0, 1000.0, 1, ACTION_FETCH);
    verifyAction(1, 1010.0, 2, ACTION_FETCH);
    verifyAction(2, 1010.0, 3, ACTION_POST);
    verifyAction(3, 1030.0, 4, ACTION_POST);
}

TEST_F(TestLinkUserModel, should_generate_non_overlapping_timelines_without_any_cached_actions) {
    EXPECT_CALL(model, getNextUserAction())
        .WillOnce(::testing::Return(MarkovModel::UserAction::FETCH))  // 1000
        .WillOnce(::testing::Return(MarkovModel::UserAction::WAIT))   // 1010
        .WillOnce(::testing::Return(MarkovModel::UserAction::FETCH))
        .WillOnce(::testing::Return(MarkovModel::UserAction::POST))
        .WillOnce(::testing::Return(MarkovModel::UserAction::WAIT))  // 1020
        .WillOnce(::testing::Return(MarkovModel::UserAction::WAIT))  // 1030
        .WillOnce(::testing::Return(MarkovModel::UserAction::POST))
        .WillOnce(::testing::Return(MarkovModel::UserAction::WAIT));  // 1040

    timeline = model.getTimeline(1000.0, 1020.0);
    ASSERT_EQ(3, timeline.size());
    verifyAction(0, 1000.0, 1, ACTION_FETCH);
    verifyAction(1, 1010.0, 2, ACTION_FETCH);
    verifyAction(2, 1010.0, 3, ACTION_POST);

    timeline = model.getTimeline(1020.0, 1040.0);
    ASSERT_EQ(1, timeline.size());
    verifyAction(0, 1030.0, 4, ACTION_POST);
}

TEST_F(TestLinkUserModel, should_generate_overlapping_timelines_with_cached_actions) {
    EXPECT_CALL(model, getNextUserAction())
        .WillOnce(::testing::Return(MarkovModel::UserAction::FETCH))  // 1000
        .WillOnce(::testing::Return(MarkovModel::UserAction::WAIT))   // 1010
        .WillOnce(::testing::Return(MarkovModel::UserAction::FETCH))
        .WillOnce(::testing::Return(MarkovModel::UserAction::POST))
        .WillOnce(::testing::Return(MarkovModel::UserAction::WAIT))  // 1020
        // 2nd time it's called, this wait will apply to time=1010 again
        .WillOnce(::testing::Return(MarkovModel::UserAction::WAIT))  // 1020
        .WillOnce(::testing::Return(MarkovModel::UserAction::POST))
        .WillOnce(::testing::Return(MarkovModel::UserAction::WAIT))   // 1030
        .WillOnce(::testing::Return(MarkovModel::UserAction::WAIT));  // 1040

    timeline = model.getTimeline(1000.0, 1020.0);
    ASSERT_EQ(3, timeline.size());
    verifyAction(0, 1000.0, 1, ACTION_FETCH);
    verifyAction(1, 1010.0, 2, ACTION_FETCH);
    verifyAction(2, 1010.0, 3, ACTION_POST);

    timeline = model.getTimeline(1010.0, 1040.0);
    ASSERT_EQ(3, timeline.size());
    verifyAction(0, 1010.0, 2, ACTION_FETCH);
    verifyAction(1, 1010.0, 3, ACTION_POST);
    verifyAction(2, 1020.0, 4, ACTION_POST);
}