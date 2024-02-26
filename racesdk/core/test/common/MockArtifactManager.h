
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

#ifndef __MOCK_ARTIFACT_MANAGER_H__
#define __MOCK_ARTIFACT_MANAGER_H__

#include <gmock/gmock.h>

#include "../../source/ArtifactManager.h"
#include "race_printers.h"

class MockArtifactManager : public ArtifactManager {
public:
    MockArtifactManager() : ArtifactManager({}) {}

    MOCK_METHOD(bool, init, (const AppConfig &appConfig), (override));
    MOCK_METHOD(bool, acquirePlugin,
                (const std::string &destPath, const std::string &pluginName,
                 const std::string &platform, const std::string &nodeType,
                 const std::string &architecture),
                (override));
    MOCK_METHOD(std::vector<std::string>, getIds, (), (const override));
};

#endif