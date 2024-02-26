
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

#ifndef __TEST_UNIT_SOURCE_MOCK_RACE_TEST_APP_OUTPUT_H__
#define __TEST_UNIT_SOURCE_MOCK_RACE_TEST_APP_OUTPUT_H__

#include "racetestapp/IRaceTestAppOutput.h"
#include "gmock/gmock.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
class MockRaceTestAppOutput : public IRaceTestAppOutput {
public:
    MOCK_METHOD(void, writeOutput, (const std::string &), (override));
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
