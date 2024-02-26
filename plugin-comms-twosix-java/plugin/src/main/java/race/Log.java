
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

package race;

import ShimsJava.RaceLog;

abstract class Log {

    private static final String pluginNameForLogging = "PluginCommsTwoSixJava";

    static void logDebug(String message) {
        RaceLog.logDebug(pluginNameForLogging, message, "");
    }

    static void logInfo(String message) {
        RaceLog.logInfo(pluginNameForLogging, message, "");
    }

    static void logWarning(String message) {
        RaceLog.logWarning(pluginNameForLogging, message, "");
    }

    static void logError(String message) {
        RaceLog.logError(pluginNameForLogging, message, "");
    }
}
