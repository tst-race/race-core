
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

#pragma once

#include <IRacePluginArtifactManager.h>
#include <IRaceSdkArtifactManager.h>

#include <unordered_map>

class PluginArtifactManagerTwoSixCppLocal : public IRacePluginArtifactManager {
public:
    explicit PluginArtifactManagerTwoSixCppLocal(IRaceSdkArtifactManager *sdk);

    virtual PluginResponse init(const PluginConfig &pluginConfig) override;

    virtual PluginResponse acquireArtifact(const std::string &destPath,
                                           const std::string &fileName) override;

    virtual PluginResponse onUserInputReceived(RaceHandle handle, bool answered,
                                               const std::string &response) override;

    virtual PluginResponse onUserAcknowledgementReceived(RaceHandle handle) override;

    virtual PluginResponse receiveAmpMessage(const std::string &message) override;

protected:
    std::unordered_map<std::string, std::string> artifactMap;
    std::string appArtifactName;
    IRaceSdkArtifactManager *raceSdk;
};
