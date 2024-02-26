
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

#ifndef __PLUGIN_CONFIG_H_
#define __PLUGIN_CONFIG_H_

#include <string>

/**
 * Various config variables passed by the sdk to the plugins
 *
 * These variables are dynamic and depend on runtime conditions, e.g. platform
 */
class PluginConfig {
public:
    std::string etcDirectory;
    std::string loggingDirectory;
    std::string auxDataDirectory;
    std::string tmpDirectory;
    std::string pluginDirectory;
};

inline bool operator==(const PluginConfig &lhs, const PluginConfig &rhs) {
    return lhs.etcDirectory == rhs.etcDirectory && lhs.loggingDirectory == rhs.loggingDirectory &&
           lhs.auxDataDirectory == rhs.auxDataDirectory && lhs.tmpDirectory == rhs.tmpDirectory &&
           lhs.pluginDirectory == rhs.pluginDirectory;
}

inline bool operator!=(const PluginConfig &lhs, const PluginConfig &rhs) {
    return !operator==(lhs, rhs);
}

#endif
