
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

#include "PluginLoader.h"

#include <string.h>

#include <fstream>
#include <nlohmann/json.hpp>

#include "ArtifactManager.h"
#include "ArtifactManagerWrapper.h"
#include "CommsWrapper.h"
#include "Composition.h"
#include "DecomposedPluginLoader.h"
#include "DynamicLibrary.h"
#include "LoaderWrapper.h"
#include "NMWrapper.h"
#include "PluginDef.h"
#include "PythonLoaderWrapper.h"
#include "RaceSdk.h"
#include "filesystem.h"
#include "helper.h"

using json = nlohmann::json;

static const std::string loggingPrefix = "PluginLoader: ";

static std::unordered_map<RaceEnums::PluginType, std::string> pluginDirectoriesByType = {
    {RaceEnums::PluginType::PT_NM, "/usr/local/lib/race/network-manager/"},
    {RaceEnums::PluginType::PT_COMMS, "/usr/local/lib/race/comms/"},
    {RaceEnums::PluginType::PT_ARTIFACT_MANAGER, "/usr/local/lib/race/artifact-manager/"}};

/**
 * @brief Load the plugins using the provided definitions.
 *
 * @param pluginsToLoad A vector of plugins to load.
 * @param sdk The sdk associated with the plugins.
 * @return IPluginLoader::PluginList<Wrapper> The loaded plugins.
 */
template <class Wrapper>
static IPluginLoader::PluginList<Wrapper> loadPlugins(std::vector<PluginDef> pluginsToLoad,
                                                      RaceSdk &sdk) {
    helper::logDebug("PluginLoader: loadPlugins: called");
    using Plugin = LoaderWrapper<Wrapper>;
    IPluginLoader::PluginList<Wrapper> plugins;

    for (const auto &pluginToLoad : pluginsToLoad) {
        try {
            switch (pluginToLoad.fileType) {
                case RaceEnums::PluginFileType::PFT_PYTHON: {
                    const std::string fullPluginPath =
                        pluginDirectoriesByType.at(pluginToLoad.type) + pluginToLoad.filePath;
                    helper::logDebug("PluginLoader: loadPlugins: loading Python plugin: " +
                                     fullPluginPath);
                    auto plugin = std::make_unique<PythonLoaderWrapper<Wrapper>>(sdk, pluginToLoad);
                    plugins.emplace_back(std::move(plugin));
                    break;
                }
                case RaceEnums::PluginFileType::PFT_SHARED_LIB: {
                    const std::string fullPluginPath =
                        pluginDirectoriesByType.at(pluginToLoad.type) + pluginToLoad.filePath +
                        "/" + pluginToLoad.sharedLibraryPath;
                    helper::logDebug("PluginLoader: loadPlugins: loading shared library plugin: " +
                                     fullPluginPath);
                    try {
                        std::string name = pluginToLoad.filePath;
                        auto plugin = std::make_unique<Plugin>(fullPluginPath, sdk, name,
                                                               pluginToLoad.configPath);
                        plugins.emplace_back(std::move(plugin));
                    } catch (const std::exception &e) {
                        // Log and Ignore any exceptions encountered loading the plugin
                        helper::logError("loadPlugins: Exception loading plugin " + fullPluginPath +
                                         ": " + e.what());
                        throw;
                    } catch (...) {
                        // Log and Ignore any exceptions encountered loading the plugin
                        helper::logError("loadPlugins: Unknown exception loading plugin " +
                                         fullPluginPath);
                        throw;
                    }
                    break;
                }
                default: {
                    const std::string errorMessage =
                        "loadPlugins: critical error. This should not be possible. invalid "
                        "plugin file "
                        "type: " +
                        std::to_string(static_cast<std::int32_t>(pluginToLoad.fileType));
                    helper::logError(errorMessage);
                    throw std::runtime_error(errorMessage);
                }
            }
        } catch (std::exception &e) {
            helper::logError("loadPlugins: Exception loading plugin " + pluginToLoad.filePath);
        }
    }

    helper::logDebug("PluginLoader: loadPlugins: returning");
    return plugins;
}

// This class is a pass through class that can be inherited from and overridden for testing
class UnifiedPluginLoader {
public:
    UnifiedPluginLoader() {}
    IPluginLoader::PluginList<CommsWrapper> loadPlugins(std::vector<PluginDef> pluginsToLoad,
                                                        RaceSdk &sdk) {
        return ::loadPlugins<CommsWrapper>(pluginsToLoad, sdk);
    }
};

class PluginLoader : public IPluginLoader {
public:
    PluginLoader() :
        unifiedPluginLoader(std::make_unique<UnifiedPluginLoader>()),
        decomposedPluginLoader(std::make_unique<DecomposedPluginLoader>(
            pluginDirectoriesByType[RaceEnums::PluginType::PT_COMMS])) {}

    virtual PluginList<NMWrapper> loadNMPlugins(
        RaceSdk &sdk, std::vector<PluginDef> pluginsToLoad = {}) override {
        helper::logDebug("loadNMPlugins called...");
        if (pluginsToLoad.size() > 1) {
            const std::string errorMessage =
                "PluginLoader: race.json request to load multiple network manager plugins. This is "
                "not "
                "supported. "
                "Please check your configuration and run again";
            helper::logError(errorMessage);
            throw std::runtime_error(errorMessage);
        }

        if (sdk.getRaceConfig().isPluginFetchOnStartEnabled &&
            sdk.getArtifactManager() != nullptr) {
            helper::logDebug("Fetching networkManager plugins from race.json via artifact manager");
            for (PluginDef pluginToLoad : pluginsToLoad) {
                std::string destPath =
                    sdk.getAppConfig().pluginArtifactsBaseDir + "/network-manager/";
                std::string nodeType = sdk.getAppConfig().nodeTypeString();
                bool success = sdk.getArtifactManager()->acquirePlugin(
                    destPath, pluginToLoad.filePath, sdk.getAppConfig().platform, nodeType,
                    sdk.getAppConfig().architecture);
                if (!success) {
                    helper::logWarning("Failed to fetch plugin " + pluginToLoad.filePath);
                }
            }
        }

        if (pluginsToLoad.size() < 1) {
            const std::string errorMessage =
                "PluginLoader: local network manager plugins not found (or ignored due to race "
                "config) and no "
                "Network Manager Plugins found remotely";
            helper::logError(errorMessage);
            throw std::runtime_error(errorMessage);
        }

        auto results = loadPlugins<NMWrapper>(pluginsToLoad, sdk);
        helper::logDebug("loadNMPlugins finished...");
        return results;
    }

    virtual PluginList<CommsWrapper> loadCommsPlugins(
        RaceSdk &sdk, std::vector<PluginDef> pluginsToLoad = {},
        std::vector<Composition> compositions = {}) override {
        helper::logDebug("loadCommsPlugins called...");

        if (sdk.getRaceConfig().isPluginFetchOnStartEnabled &&
            sdk.getArtifactManager() != nullptr) {
            helper::logDebug("Fetching comms plugins from race.json via artifact manager");
            for (PluginDef pluginToLoad : pluginsToLoad) {
                std::string destPath = sdk.getAppConfig().pluginArtifactsBaseDir + "/comms/";
                std::string nodeType = sdk.getAppConfig().nodeTypeString();
                bool success = sdk.getArtifactManager()->acquirePlugin(
                    destPath, pluginToLoad.filePath, sdk.getAppConfig().platform, nodeType,
                    sdk.getAppConfig().architecture);
                if (!success) {
                    helper::logWarning("Failed to fetch plugin " + pluginToLoad.filePath);
                }
            }
        }

        if (pluginsToLoad.size() < 1) {
            const std::string errorMessage =
                "PluginLoader: local comms plugins not found (or ignored due to race config) and "
                "no "
                "Comms Plugins found remotely";
            helper::logError(errorMessage);
            throw std::runtime_error(errorMessage);
        }

        std::vector<PluginDef> unified_plugins;
        std::vector<PluginDef> decomposed_plugins;
        for (PluginDef pluginToLoad : pluginsToLoad) {
            if (pluginToLoad.is_unified_comms_plugin()) {
                unified_plugins.push_back(pluginToLoad);
            }
            if (pluginToLoad.is_decomposed_comms_plugin()) {
                decomposed_plugins.push_back(pluginToLoad);
            }
        }

        auto unifiedCommsWrappers = unifiedPluginLoader->loadPlugins(unified_plugins, sdk);
        decomposedPluginLoader->loadComponents(decomposed_plugins);
        auto commsWrappers = decomposedPluginLoader->compose(compositions, sdk);
        for (auto &commsWrapper : unifiedCommsWrappers) {
            commsWrappers.emplace_back(std::move(commsWrapper));
        }
        helper::logDebug("loadCommsPlugins finished...");
        return commsWrappers;
    }

    virtual PluginList<ArtifactManagerWrapper> loadArtifactManagerPlugins(
        RaceSdk &sdk, std::vector<PluginDef> pluginsToLoad = {}) override {
        helper::logDebug("loadArtifactManagerPlugins called...");

        auto results = loadPlugins<ArtifactManagerWrapper>(pluginsToLoad, sdk);
        helper::logDebug("loadArtifactManagerPlugins finished...");
        return results;
    }

public:
    std::unique_ptr<UnifiedPluginLoader> unifiedPluginLoader;
    std::unique_ptr<DecomposedPluginLoader> decomposedPluginLoader;
};

IPluginLoader &IPluginLoader::factoryDefault(const std::string &pluginArtifactsBaseDir) noexcept {
    pluginDirectoriesByType[RaceEnums::PluginType::PT_NM] =
        pluginArtifactsBaseDir + "/network-manager/";
    pluginDirectoriesByType[RaceEnums::PluginType::PT_COMMS] = pluginArtifactsBaseDir + "/comms/";
    pluginDirectoriesByType[RaceEnums::PluginType::PT_ARTIFACT_MANAGER] =
        pluginArtifactsBaseDir + "/artifact-manager/";
    static PluginLoader pluginLoader;
    return pluginLoader;
}
