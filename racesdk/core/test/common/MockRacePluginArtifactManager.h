
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

#ifndef __MOCK_RACE_PLUGIN_ARTIFACT_MANAGER_H_
#define __MOCK_RACE_PLUGIN_ARTIFACT_MANAGER_H_

#include <gmock/gmock.h>

#include "IRacePluginArtifactManager.h"
#include "race_printers.h"

class MockRacePluginArtifactManager : public IRacePluginArtifactManager {
public:
    MOCK_METHOD(PluginResponse, init, (const PluginConfig &pluginConfig), (override));
    MOCK_METHOD(PluginResponse, acquireArtifact,
                (const std::string &destPath, const std::string &fileName), (override));
    MOCK_METHOD(PluginResponse, onUserInputReceived,
                (RaceHandle handle, bool answered, const std::string &response), (override));
    MOCK_METHOD(PluginResponse, onUserAcknowledgementReceived, (RaceHandle handle), (override));
    MOCK_METHOD(PluginResponse, receiveAmpMessage, (const std::string &message), (override));
};

#endif