
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

#include "../../../source/TestHarnessWrapper.h"
#include "../../common/MockPluginNMTestHarness.h"
#include "../../common/MockRaceSdk.h"
#include "gtest/gtest.h"

using ::testing::Return;

class TestableTestHarnessWrapper : public TestHarnessWrapper {
public:
    TestableTestHarnessWrapper(RaceSdk &sdk) : TestHarnessWrapper(sdk) {}
    void setTestHarness(std::shared_ptr<MockPluginNMTestHarness> &testHarness) {
        // Replace the auto-constructed real test harness with the mock test harness
        mTestHarness = testHarness;
    }
};

class TestHarnessWrapperTest : public ::testing::Test {
public:
    MockRaceSdk sdk;
    std::shared_ptr<MockPluginNMTestHarness> mockTestHarness;
    TestableTestHarnessWrapper wrapper{sdk};

    TestHarnessWrapperTest() {
        mockTestHarness = std::make_shared<MockPluginNMTestHarness>(&wrapper);
        wrapper.setTestHarness(mockTestHarness);
    }
};

TEST_F(TestHarnessWrapperTest, processNMBypassMsg) {
    const std::string route = "test-route";
    const ClrMsg sentMessage("test message", "from sender", "to recipient", 1, 0);
    RaceHandle handle = 42;

    EXPECT_CALL(*mockTestHarness, processNMBypassMsg(handle, route, sentMessage)).Times(1);

    wrapper.startHandler();
    wrapper.processNMBypassMsg(handle, sentMessage, route, 0);
    wrapper.stopHandler();
}

TEST_F(TestHarnessWrapperTest, openRecvConnection) {
    RaceHandle handle = 42;

    EXPECT_CALL(*mockTestHarness, openRecvConnection(handle, "test-persona", "test-route"))
        .Times(1);

    wrapper.startHandler();
    wrapper.openRecvConnection(handle, "test-persona", "test-route", 0);
    wrapper.stopHandler();
}

TEST_F(TestHarnessWrapperTest, rpcDeactivateChannel) {
    EXPECT_CALL(*mockTestHarness, rpcDeactivateChannel("test-channel")).Times(1);

    wrapper.startHandler();
    wrapper.rpcDeactivateChannel("test-channel", 0);
    wrapper.stopHandler();
}

TEST_F(TestHarnessWrapperTest, rpcDestroyLink) {
    EXPECT_CALL(*mockTestHarness, rpcDestroyLink("test-link")).Times(1);

    wrapper.startHandler();
    wrapper.rpcDestroyLink("test-link", 0);
    wrapper.stopHandler();
}

TEST_F(TestHarnessWrapperTest, rpcCloseConnection) {
    EXPECT_CALL(*mockTestHarness, rpcCloseConnection("test-conn")).Times(1);

    wrapper.startHandler();
    wrapper.rpcCloseConnection("test-conn", 0);
    wrapper.stopHandler();
}
