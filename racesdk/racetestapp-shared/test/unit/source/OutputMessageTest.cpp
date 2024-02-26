
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

#include "../../common/MockRaceTestAppOutput.h"
#include "gtest/gtest.h"
#include "racetestapp/raceTestAppHelpers.h"

TEST(outputMessage, simple_message) {
    MockRaceTestAppOutput output;

    ClrMsg messageToLog("test-id some message", "sender", "recipient", 123456, 654321);

    EXPECT_CALL(
        output,
        writeOutput(::testing::HasSubstr(
            "checksum: 0a3bd68080d24dc6d1506038a9a5eb6e5f6ac57b, size: 20, nonce: 654321, from: "
            "sender, to: recipient, test-id: test-id, sent-time: 123456, traceid: 0, message: "
            "test-id some message")));

    rtah::outputMessage(output, messageToLog);
}

TEST(outputMessage, oversized_message) {
    MockRaceTestAppOutput output;

    ClrMsg messageToLog("test-id " + std::string(5000, 'A'), "sender", "recipient", 123456, 654321);

    EXPECT_CALL(
        output,
        writeOutput(::testing::HasSubstr(
            "checksum: 0a1cdb754820d77662626dc50378006a5b065cc3, size: 5008, nonce: 654321, from: "
            "sender, to: recipient, test-id: test-id, sent-time: 123456, traceid: 0, message: "
            "test-id " +
            std::string(256 - 8 - 3, 'A') + "...")));

    rtah::outputMessage(output, messageToLog);
}
