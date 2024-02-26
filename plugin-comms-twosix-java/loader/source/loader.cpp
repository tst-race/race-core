
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

#include <IRacePluginComms.h>
#include <RaceLog.h>

#include "PluginCommsJavaWrapper.h"

#ifndef TESTBUILD

extern "C" const RaceVersionInfo raceVersion = RACE_VERSION;
extern "C" const char *const racePluginId = "PluginCommsTwoSixJava";
extern "C" const char *const racePluginDescription = "Plugin Comms Java Exemplar (Two Six Labs) ";

// replace this signature with your plugin signature
const char *const pluginClassSignature = "race/PluginCommsTwoSixJava";
const char *const logLabel = "JavaCommsLoader";

IRacePluginComms *createPluginComms(IRaceSdkComms *sdk) {
    RaceLog::logDebug(logLabel, "Loading Java Comms Plugin", "");
    return new PluginCommsJavaWrapper(sdk, racePluginId, pluginClassSignature);
}

void destroyPluginComms(IRacePluginComms *plugin) {
    if (plugin != nullptr) {
        delete plugin;
    }
}

#endif
