
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

#ifndef __RACE_PLUGIN_DEF_H_
#define __RACE_PLUGIN_DEF_H_

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>
#include <vector>

#include "RaceEnums.h"

using json = nlohmann::json;

/**
 * @brief Definition used to load a plugin.
 *
 */
class PluginDef {
public:
    std::string filePath;
    RaceEnums::PluginType type;
    RaceEnums::PluginFileType fileType;
    RaceEnums::NodeType nodeType;
    std::string sharedLibraryPath;
    std::string platform;
    std::string architecture;
    std::string pythonModule;
    std::string pythonClass;
    std::string configPath;
    std::string shardName;
    std::vector<std::string> channels;
    std::vector<std::string> usermodels;
    std::vector<std::string> transports;
    std::vector<std::string> encodings;

    /**
     * @brief Convert plugin json to a plugin definition. If the plugin json is invalid then an
     * exception will be thrown.
     *
     * @param pluginJson The plugin json.
     * @return PluginDef The plugin definition.
     */
    static PluginDef pluginJsonToPluginDef(json pluginJson);

    bool is_unified_comms_plugin();
    bool is_decomposed_comms_plugin();
};
#endif