
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

#include "MarkovModel.h"

class MarkovModelToTest : public MarkovModel {
public:
    MOCK_METHOD(double, getRandom, (), (override));

    void setCurrentState(UserAction userAction) {
        currentState = static_cast<int>(userAction);
    }
};

class TestMarkovModel : public ::testing::Test {
public:
    MarkovModelToTest model;

    void assertTransitionDistribution(MarkovModel::UserAction sourceState, int expectedNumFetch,
                                      int expectedNumPost, int expectedNumWait) {
        // Make sure all expected counts add up to 100
        ASSERT_EQ(100, expectedNumFetch + expectedNumPost + expectedNumWait);

        // Set up a perfectly uniform distribution of 100 random values from 0.0 to 0.99
        double nextRandom = 0.0;
        EXPECT_CALL(model, getRandom())
            .Times(100)
            .WillRepeatedly(::testing::Invoke([&nextRandom]() {
                double tmp = nextRandom;
                nextRandom += 0.01;
                return tmp;
            }));

        int numFetch = 0;
        int numPost = 0;
        int numWait = 0;

        for (int i = 0; i < 100; ++i) {
            model.setCurrentState(sourceState);
            auto action = model.getNextUserAction();
            switch (action) {
                case MarkovModel::UserAction::FETCH:
                    ++numFetch;
                    break;
                case MarkovModel::UserAction::POST:
                    ++numPost;
                    break;
                case MarkovModel::UserAction::WAIT:
                    ++numWait;
                    break;
                default:
                    FAIL() << "Invalid user action enum value: " << static_cast<int>(action);
            }
        }

        // Allow 1-off-from-expected to account for floating point rounding
        EXPECT_NEAR(expectedNumFetch, numFetch, 1);
        EXPECT_NEAR(expectedNumPost, numPost, 1);
        EXPECT_NEAR(expectedNumWait, numWait, 1);
    }
};

TEST_F(TestMarkovModel, all_transitions_from_fetch) {
    assertTransitionDistribution(MarkovModel::UserAction::FETCH, 0, 100, 0);
}

TEST_F(TestMarkovModel, all_transitions_from_post) {
    assertTransitionDistribution(MarkovModel::UserAction::POST, 0, 0, 100);
}

TEST_F(TestMarkovModel, all_transitions_from_wait) {
    assertTransitionDistribution(MarkovModel::UserAction::WAIT, 100, 0, 0);
}