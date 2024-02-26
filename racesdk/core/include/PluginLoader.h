
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

#ifndef __SOURCE_PLUGIN_LOADER_H__
#define __SOURCE_PLUGIN_LOADER_H__

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "OpenTracingForwardDeclarations.h"
#include "PluginDef.h"
#include "RaceConfig.h"
#include "WrapperForwardDeclarations.h"

class RaceSdk;

inline std::ostream &operator<<(std::ostream &os, const PluginDef &pluginDef) {
    os << "{ "
       << "filePath: " << pluginDef.filePath << ", "
       << "type: " << static_cast<std::int32_t>(pluginDef.type) << ", "
       << "fileType: " << static_cast<std::int32_t>(pluginDef.fileType) << ", "
       << "nodeType: " << static_cast<std::int32_t>(pluginDef.nodeType) << ", "
       << "platform: " << pluginDef.platform << ", "
       << "pythonModule: " << pluginDef.pythonModule << ", "
       << "pythonClass: " << pluginDef.pythonClass << ", "
       << "configPath: " << pluginDef.configPath << ", "
       << "shardName: " << pluginDef.shardName << " }";

    return os;
}

/**
 * @brief Abstract base class to define interface for a plugin loader
 *
 * This class can be extended in the future once dynamic loading is implemented. Possible
 * exentsions:
 *      * list all available plugins
 *      * load plugin by name
 *      * utility to destroy plugins
 *
 */
class IPluginLoader {
public:
    template <class T>
    using PluginList = std::vector<std::unique_ptr<T>>;

    /**
     * @brief Load the artifact manager plugins from a provided list of plugin definitions.
     *
     * @param sdk Reference to the sdk that is loading the plugins.
     * @return List of artifact manager plugins
     */
    virtual PluginList<ArtifactManagerWrapper> loadArtifactManagerPlugins(
        RaceSdk &sdk, std::vector<PluginDef> configRequestedPlugins = {}) = 0;

    /**
     * @brief Load the network manager plugins from a provided list of plugin definitions. If the
     * list of plugin definitions is empty then a default location will be searched for available
     * plugins.
     *
     * @param sdk Reference to the sdk that is loading the plugins.
     * @param configRequestedPlugins A vector of plugin definitions requested in race.json.
     * @return PluginList<NMWrapper> A list of network manager plugins.
     */
    virtual PluginList<NMWrapper> loadNMPlugins(
        RaceSdk &sdk, std::vector<PluginDef> configRequestedPlugins = {}) = 0;

    /**
     * @brief Load the comms plugins from a provided list of plugin definitions. If the list of
     * plugin definitions is empty then a default location will be searched for available plugins.
     *
     * @param sdk Reference to the sdk that is loading the plugins.
     * @param configRequestedPlugins A vector of plugin definitions requested in race.json.
     * @return PluginList<CommsWrapper> A list of comms plugins.
     */
    virtual PluginList<CommsWrapper> loadCommsPlugins(
        RaceSdk &sdk, std::vector<PluginDef> pluginsToLoad = {},
        std::vector<Composition> compositions = {}) = 0;

    /**
     * @brief Static class function to return a default, concrete reference to a singleton of an
     * IPluginLoader.
     *
     * @param pluginArtifactsBaseDir base location for plugin artifacts (/usr/local/lib/race on
     * linux, /data/data/com.twosix.race/race/artifacts on android)
     * @return IPluginLoader& A default, concrete instance of IPluginLoader.
     */
    static IPluginLoader &factoryDefault(const std::string &pluginArtifactsBaseDir) noexcept;
};

#endif
