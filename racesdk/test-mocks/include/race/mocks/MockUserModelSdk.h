
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

#ifndef _MockUserModelSdk
#define _MockUserModelSdk

// System
#include <gmock/gmock.h>

// SDK Core
#include <IUserModelSdk.h>

class MockUserModelSdk : public IUserModelSdk {
public:
    MOCK_METHOD(std::string, getActivePersona, (), (override));
    MOCK_METHOD(ChannelResponse, requestPluginUserInput,
                (const std::string &key, const std::string &prompt, bool cache), (override));
    MOCK_METHOD(ChannelResponse, requestCommonUserInput, (const std::string &key), (override));
    MOCK_METHOD(ChannelResponse, updateState, (ComponentState state), (override));
    MOCK_METHOD(ChannelResponse, makeDir, (const std::string &directoryPath), (override));
    MOCK_METHOD(ChannelResponse, removeDir, (const std::string &directoryPath), (override));
    MOCK_METHOD(std::vector<std::string>, listDir, (const std::string &directoryPath), (override));
    MOCK_METHOD(std::vector<std::uint8_t>, readFile, (const std::string &filepath), (override));
    MOCK_METHOD(ChannelResponse, appendFile,
                (const std::string &filepath, const std::vector<std::uint8_t> &data), (override));
    MOCK_METHOD(ChannelResponse, writeFile,
                (const std::string &filepath, const std::vector<std::uint8_t> &data), (override));
    MOCK_METHOD(ChannelResponse, onTimelineUpdated, (), (override));
};

#endif