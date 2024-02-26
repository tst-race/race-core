
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

#include "../../../source/AppWrapper.h"
#include "../../common/MockRaceApp.h"
#include "../../common/MockRaceSdk.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(AppWrapper, test_constructor) {
    MockRaceApp mockApp;
    MockRaceSdk sdk;
    AppWrapper wrapper(&mockApp, sdk);
}

TEST(AppWrapper, startHandler) {
    MockRaceApp mockApp;
    MockRaceSdk sdk;
    AppWrapper wrapper(&mockApp, sdk);
    wrapper.startHandler();

    // destructor should stop handler thread
}

TEST(AppWrapper, start_stop_handler) {
    MockRaceApp mockApp;
    MockRaceSdk sdk;
    AppWrapper wrapper(&mockApp, sdk);
    wrapper.startHandler();
    wrapper.stopHandler();
}

TEST(AppWrapper, handleReceivedMessage) {
    MockRaceApp mockApp;
    MockRaceSdk sdk;
    AppWrapper wrapper(&mockApp, sdk);

    std::string messageText = "my message";
    const ClrMsg sentMessage(messageText, "from sender", "to recipient", 1, 0);

    EXPECT_CALL(mockApp, handleReceivedMessage(sentMessage)).Times(1);

    wrapper.startHandler();
    wrapper.handleReceivedMessage(sentMessage);
    wrapper.stopHandler();
}