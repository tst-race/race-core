
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

#ifndef __TEST_INTEGRATION_SOURCE_MOCK_PLUGIN_LOADER_H__
#define __TEST_INTEGRATION_SOURCE_MOCK_PLUGIN_LOADER_H__

#include <memory>

#include "../../include/PluginLoader.h"
#include "../../source/ArtifactManagerWrapper.h"
#include "../../source/CommsWrapper.h"
#include "../../source/NMWrapper.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockPluginLoader : public IPluginLoader {
public:
    MockPluginLoader(std::vector<std::shared_ptr<IRacePluginNM>> _networkManagerPlugins,
                     std::vector<std::shared_ptr<IRacePluginComms>> _commsPlugins,
                     std::vector<std::shared_ptr<IRacePluginArtifactManager>> _artifactMgrPlugins) :
        networkManagerPlugins(std::move(_networkManagerPlugins)),
        commsPlugins(std::move(_commsPlugins)),
        artifactMgrPlugins(std::move(_artifactMgrPlugins)) {}

    virtual PluginList<NMWrapper> loadNMPlugins(
        RaceSdk &sdk, std::vector<PluginDef> /*pluginDefs*/ = {}) override {
        PluginList<NMWrapper> loaded;
        for (size_t i = 0; i < networkManagerPlugins.size(); ++i) {
            loaded.emplace_back(std::make_unique<NMWrapper>(networkManagerPlugins[i],
                                                            "MockNM-" + std::to_string(i),
                                                            "Mock Network Manager Testing", sdk));
        }
        return loaded;
    }

    virtual PluginList<CommsWrapper> loadCommsPlugins(
        RaceSdk &sdk, std::vector<PluginDef> /*pluginDefs*/ = {},
        std::vector<Composition> /* compositions */ = {}) override {
        PluginList<CommsWrapper> loaded;
        for (size_t i = 0; i < commsPlugins.size(); ++i) {
            loaded.emplace_back(std::make_unique<CommsWrapper>(
                commsPlugins[i], "MockComms-" + std::to_string(i), "Mock Comms Testing", sdk));
        }
        return loaded;
    }

    virtual PluginList<ArtifactManagerWrapper> loadArtifactManagerPlugins(
        RaceSdk &sdk, std::vector<PluginDef> /*pluginDefs*/ = {}) override {
        PluginList<ArtifactManagerWrapper> loaded;
        for (size_t i = 0; i < artifactMgrPlugins.size(); ++i) {
            loaded.emplace_back(std::make_unique<ArtifactManagerWrapper>(
                artifactMgrPlugins[i], "MockArtifactManager-" + std::to_string(i),
                "Mock ArtifactManager Testing", sdk));
        }
        return loaded;
    }

private:
    std::vector<std::shared_ptr<IRacePluginNM>> networkManagerPlugins;
    std::vector<std::shared_ptr<IRacePluginComms>> commsPlugins;
    std::vector<std::shared_ptr<IRacePluginArtifactManager>> artifactMgrPlugins;
};

#endif
