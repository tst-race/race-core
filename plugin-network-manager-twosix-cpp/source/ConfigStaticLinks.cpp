
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

#include "ConfigStaticLinks.h"

#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

#include "JsonIO.h"
#include "Log.h"

using json = nlohmann::json;
using ChannelGid = std::string;
using Persona = std::string;

auto ConfigStaticLinks::loadLinkProfiles(IRaceSdkNM &sdk, const std::string &configFilePath)
    -> ChannelLinkProfilesMap {
    TRACE_FUNCTION(configFilePath);
    try {
        return JsonIO::loadJson(sdk, configFilePath);
    } catch (std::exception &e) {
        logError(logPrefix + "Failed to load link profiles: " + e.what());
        return {};
    }
}

bool ConfigStaticLinks::writeLinkProfiles(IRaceSdkNM &sdk, const std::string &configFilePath,
                                          const ChannelLinkProfilesMap &linkProfiles) {
    TRACE_FUNCTION(configFilePath);
    return JsonIO::writeJson(sdk, configFilePath, linkProfiles);
}