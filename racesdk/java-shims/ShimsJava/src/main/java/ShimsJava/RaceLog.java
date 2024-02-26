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

// For API documentation please see the equivalent C++ header:
// racesdk/common/include/RaceLog.h

package ShimsJava;

public class RaceLog {

    public static native void logDebug(String pluginName, String message, String stackTrace);

    public static native void logInfo(String pluginName, String message, String stackTrace);

    public static native void logWarning(String pluginName, String message, String stackTrace);

    public static native void logError(String pluginName, String message, String stackTrace);
}
