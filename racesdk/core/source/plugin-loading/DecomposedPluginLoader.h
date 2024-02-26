
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

#ifndef __DECOMPOSED_PLUGIN_LOADER_H__
#define __DECOMPOSED_PLUGIN_LOADER_H__

#include <vector>

#include "ComponentPlugin.h"
#include "IComponentPlugin.h"
#include "PluginLoader.h"
#include "PythonComponentPlugin.h"
#include "RaceEnums.h"

class DecomposedPluginLoader {
public:
    explicit DecomposedPluginLoader(const std::string &pluginPath);
    void loadComponents(std::vector<PluginDef> componentPlugins);
    IPluginLoader::PluginList<CommsWrapper> compose(std::vector<Composition> compositions,
                                                    RaceSdk &sdk);

protected:
    void loadComponentsForPlugin(const PluginDef &pluginToLoad);

public:
    std::string pluginPath;
    std::vector<std::shared_ptr<IComponentPlugin>> plugins;
    std::unordered_map<std::string, IComponentPlugin *> transports;
    std::unordered_map<std::string, IComponentPlugin *> usermodels;
    std::unordered_map<std::string, IComponentPlugin *> encodings;
};

#endif
