
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

#include "../../../source/RaceRegistry.h"
#include "../../../source/RaceRegistryApp.h"
#include "MockRaceSdkApp.h"
#include "OpenTracingHelpers.h"
#include "gmock/gmock.h"

// printer to avoid valgrind complaining about gtest printers
static std::ostream &operator<<(std::ostream &os, const AppConfig & /*config*/) {
    os << "<AppConfig>"
       << "\n";
    return os;
}

class MockAppOutput : public IRaceTestAppOutput {
    MOCK_METHOD(void, writeOutput, (const std::string &output), (override));
};

class RaceRegistryTestFixture : public ::testing::Test {
public:
    RaceRegistryTestFixture() :
        tracer(createTracer("", "race-client-00001")),
        registry(mockSdk, tracer),
        app(output, mockSdk, tracer, registry) {}
    virtual ~RaceRegistryTestFixture() {}
    virtual void SetUp() override {}
    virtual void TearDown() override {}

public:
    MockRaceSdkApp mockSdk;
    MockAppOutput output;
    std::shared_ptr<opentracing::Tracer> tracer;
    RaceRegistry registry;
    RaceRegistryApp app;
};

using testing::_;

TEST_F(RaceRegistryTestFixture, test_registry_response) {
    ClrMsg msg("{\"message\": \"some message\", \"ampIndex\": 42}", "other persona",
               "race-client-00001", 1234567890, 0, 0, 0, 0);
    EXPECT_CALL(mockSdk, sendClientMessage(_)).Times(1);
    app.handleReceivedMessage(msg);
}

TEST_F(RaceRegistryTestFixture, test_app_invalid_message) {
    ClrMsg msg("invalid json", "other persona", "race-client-00001", 1234567890, 0, 0, 0, 0);
    EXPECT_CALL(mockSdk, sendClientMessage(_)).Times(0);
    app.handleReceivedMessage(msg);
}