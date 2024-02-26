
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

#include "log.h"

static const std::string pluginNameForLogging = "PluginCommsTwoSixDecomposedCpp";

void logDebug(const std::string &message) {
    RaceLog::logDebug(pluginNameForLogging, message, "");
}

void logInfo(const std::string &message) {
    RaceLog::logInfo(pluginNameForLogging, message, "");
}

void logWarning(const std::string &message) {
    RaceLog::logWarning(pluginNameForLogging, message, "");
}

void logError(const std::string &message) {
    RaceLog::logError(pluginNameForLogging, message, "");
}
