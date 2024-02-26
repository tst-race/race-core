
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

#include "race_log.h"

#include <RaceLog.h>

static void handle_nulls(const char *&pluginName, const char *&message, const char *&stackTrace) {
    if (pluginName == nullptr) {
        RaceLog::logError("C Shim", "NULL passed to log function for pluginName", "");
        pluginName = "";
    }
    if (message == nullptr) {
        RaceLog::logError("C Shim", "NULL passed to log function for message", "");
        message = "";
    }
    if (stackTrace == nullptr) {
        RaceLog::logError("C Shim", "NULL passed to log function for stackTrace", "");
        stackTrace = "";
    }
}

void race_log_debug(const char *pluginName, const char *message, const char *stackTrace) {
    handle_nulls(pluginName, message, stackTrace);
    RaceLog::logDebug(pluginName, message, stackTrace);
}

void race_log_info(const char *pluginName, const char *message, const char *stackTrace) {
    handle_nulls(pluginName, message, stackTrace);
    RaceLog::logInfo(pluginName, message, stackTrace);
}

void race_log_warning(const char *pluginName, const char *message, const char *stackTrace) {
    handle_nulls(pluginName, message, stackTrace);
    RaceLog::logWarning(pluginName, message, stackTrace);
}

void race_log_error(const char *pluginName, const char *message, const char *stackTrace) {
    handle_nulls(pluginName, message, stackTrace);
    RaceLog::logError(pluginName, message, stackTrace);
}
