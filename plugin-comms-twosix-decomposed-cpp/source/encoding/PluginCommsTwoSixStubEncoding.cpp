
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

#include <IEncodingComponent.h>
#include <string.h>  // strcmp

#include "PluginCommsTwoSixBase64Encoding.h"
#include "PluginCommsTwoSixNoopEncoding.h"
#include "log.h"

#ifndef TESTBUILD
IEncodingComponent *createEncoding(const std::string &encoding, IEncodingSdk *sdk,
                                   const std::string &roleName, const PluginConfig &pluginConfig) {
    TRACE_FUNCTION(encoding, roleName, pluginConfig.pluginDirectory);

    if (sdk == nullptr) {
        RaceLog::logError(logPrefix, "`sdk` parameter is set to NULL.", "");
        return nullptr;
    }

    if (encoding == PluginCommsTwoSixNoopEncoding::name) {
        return new PluginCommsTwoSixNoopEncoding(sdk);
    } else if (encoding == PluginCommsTwoSixBase64Encoding::name) {
        return new PluginCommsTwoSixBase64Encoding(sdk);
    } else {
        RaceLog::logError(logPrefix, "invalid encoding type: " + std::string(encoding), "");
        return nullptr;
    }
}
void destroyEncoding(IEncodingComponent *component) {
    TRACE_FUNCTION();
    delete component;
}

const RaceVersionInfo raceVersion = RACE_VERSION;
#endif