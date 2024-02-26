
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

#include "Log.h"

#include "RaceLog.h"

#ifndef NETWORK_MANAGER_PLUGIN_LOGGING_NAME
#define NETWORK_MANAGER_PLUGIN_LOGGING_NAME "PluginNMTwoSixCpp"
#endif

void logDebug(const std::string &message) {
    RaceLog::logDebug(NETWORK_MANAGER_PLUGIN_LOGGING_NAME, message, "");
}
void logInfo(const std::string &message) {
    RaceLog::logInfo(NETWORK_MANAGER_PLUGIN_LOGGING_NAME, message, "");
}
void logWarning(const std::string &message) {
    RaceLog::logWarning(NETWORK_MANAGER_PLUGIN_LOGGING_NAME, message, "");
}
void logError(const std::string &message) {
    RaceLog::logError(NETWORK_MANAGER_PLUGIN_LOGGING_NAME, message, "");
}

constexpr std::size_t MAX_MSG_LEN = 256;

void logMessage(const std::string &prefix, const std::string &message) {
    if (message.size() <= MAX_MSG_LEN) {
        logDebug(prefix + message);
    } else {
        logDebug(prefix + message.substr(0, MAX_MSG_LEN - 3) + "...");
    }
}
