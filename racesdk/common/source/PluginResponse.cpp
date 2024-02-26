
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

#include "PluginResponse.h"

std::string pluginResponseToString(PluginResponse pluginResponse) {
    switch (pluginResponse) {
        case PLUGIN_INVALID:
            return "PLUGIN_INVALID";
        case PLUGIN_OK:
            return "PLUGIN_OK";
        case PLUGIN_TEMP_ERROR:
            return "PLUGIN_TEMP_ERROR";
        case PLUGIN_ERROR:
            return "PLUGIN_ERROR";
        case PLUGIN_FATAL:
            return "PLUGIN_FATAL";
        default:
            return "ERROR: INVALID PLUGIN RESPONSE: " + std::to_string(pluginResponse);
    }
}

std::ostream &operator<<(std::ostream &out, PluginResponse pluginResponse) {
    return out << pluginResponseToString(pluginResponse);
}
