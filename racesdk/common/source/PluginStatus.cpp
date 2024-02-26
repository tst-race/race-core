
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

#include "PluginStatus.h"

std::string pluginStatusToString(PluginStatus pluginStatus) {
    switch (pluginStatus) {
        case PLUGIN_UNDEF:
            return "PLUGIN_UNDEF";
        case PLUGIN_NOT_READY:
            return "PLUGIN_NOT_READY";
        case PLUGIN_READY:
            return "PLUGIN_READY";
        default:
            return "ERROR: INVALID PLUGIN STATUS: " + std::to_string(pluginStatus);
    }
}

std::ostream &operator<<(std::ostream &out, PluginStatus pluginStatus) {
    return out << pluginStatusToString(pluginStatus);
}
