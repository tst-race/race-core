
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

#include "ShimsJava_RaceLog.h"

#include <RaceLog.h>

#include "JavaShimUtils.h"

JNIEXPORT void JNICALL Java_ShimsJava_RaceLog_logDebug(JNIEnv *e, jclass, jstring jPluginName,
                                                       jstring jMessage, jstring jStackTrace) {
    std::string pluginName = JavaShimUtils::jstring2string(e, jPluginName);
    std::string message = JavaShimUtils::jstring2string(e, jMessage);
    std::string stackTrace = JavaShimUtils::jstring2string(e, jStackTrace);
    RaceLog::logDebug(pluginName, message, stackTrace);
}

JNIEXPORT void JNICALL Java_ShimsJava_RaceLog_logInfo(JNIEnv *e, jclass, jstring jPluginName,
                                                      jstring jMessage, jstring jStackTrace) {
    std::string pluginName = JavaShimUtils::jstring2string(e, jPluginName);
    std::string message = JavaShimUtils::jstring2string(e, jMessage);
    std::string stackTrace = JavaShimUtils::jstring2string(e, jStackTrace);
    RaceLog::logInfo(pluginName, message, stackTrace);
}

JNIEXPORT void JNICALL Java_ShimsJava_RaceLog_logWarning(JNIEnv *e, jclass, jstring jPluginName,
                                                         jstring jMessage, jstring jStackTrace) {
    std::string pluginName = JavaShimUtils::jstring2string(e, jPluginName);
    std::string message = JavaShimUtils::jstring2string(e, jMessage);
    std::string stackTrace = JavaShimUtils::jstring2string(e, jStackTrace);
    RaceLog::logWarning(pluginName, message, stackTrace);
}

JNIEXPORT void JNICALL Java_ShimsJava_RaceLog_logError(JNIEnv *e, jclass, jstring jPluginName,
                                                       jstring jMessage, jstring jStackTrace) {
    std::string pluginName = JavaShimUtils::jstring2string(e, jPluginName);
    std::string message = JavaShimUtils::jstring2string(e, jMessage);
    std::string stackTrace = JavaShimUtils::jstring2string(e, jStackTrace);
    RaceLog::logError(pluginName, message, stackTrace);
}