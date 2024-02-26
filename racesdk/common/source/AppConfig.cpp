
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

#include "../include/AppConfig.h"

#include <RaceLog.h>
#include <inttypes.h>

#include <algorithm>
#include <boost/io/ios_state.hpp>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

// TODO: have a utility library that contains utility functions like `shell`
// static std::string getArch() {
//     std::string arch = helper::shell("uname -m");
//     if (!arch.empty()) {
//         // remove the newline at the end
//         arch.resize(arch.size() - 1);
//     } else {
//         throw std::runtime_error("`uname -m` failed. This should never happen.");
//     }

//     return arch;
// }

AppConfig::AppConfig() :
    appDir("/usr/local/lib"),
#ifdef __ANDROID__
    pluginArtifactsBaseDir("/data/data/com.twosix.race/race/artifacts"),
    platform("android"),
#else
    pluginArtifactsBaseDir("/usr/local/lib/race"),
    platform("linux"),
#endif
#if defined(__x86_64__)
    architecture("x86_64"),
#else
    architecture("arm64-v8a"),
#endif
    environment(""),
#ifdef __ANDROID__
    configTarPath("/storage/self/primary/Download/race/configs.tar.gz"),
    baseConfigPath("/storage/self/primary/data/com.twosix.race/files/data/race/data/configs"),
#else
    configTarPath("/tmp/configs.tar.gz"),
    baseConfigPath("/data/configs"),
#endif

#ifdef __ANDROID__
    etcDirectory("/storage/self/primary/Download/race/etc"),
#else
    etcDirectory("/etc/race"),
#endif
    jaegerConfigPath(etcDirectory + "/jaeger-config.yml"),
    userResponsesFilePath(etcDirectory + "/user-responses.json"),
    voaConfigPath(etcDirectory + "/voa.json"),
#ifdef __ANDROID__
    bootstrapFilesDirectory(
        "/storage/self/primary/Android/data/com.twosix.race/files/data/bootstrap-files"),
    bootstrapCacheDirectory(
        "/storage/self/primary/Android/data/com.twosix.race/files/data/bootstrap-cache"),
#else
    bootstrapFilesDirectory("/data/bootstrap-files"),
    bootstrapCacheDirectory("/data/bootstrap-cache"),
#endif
    sdkFilePath("sdk"),
    tmpDirectory("/tmp"),
    logDirectory("/log"),
    logFilePath(logDirectory + "/race.log"),
    nodeType(RaceEnums::NT_UNDEF),
    encryptionType(RaceEnums::StorageEncryptionType::ENC_AES) {
#ifdef __ANDROID__
    nodeType = RaceEnums::NT_CLIENT;
#else
    nodeType = RaceEnums::stringToNodeType(getenv("RACE_NODE_TYPE"));
#endif
}

std::string AppConfig::to_string() const {
    std::stringstream ss;
    ss << "Logging AppConfig...\n";
    ss << " --- App Config Begin --- \n";

    ss << "persona: " << persona << "\n";
    ss << "appDir: " << appDir << "\n";
    ss << "platform: " << platform << "\n";
    ss << "architecture: " << architecture << "\n";
    ss << "environment: " << environment << "\n";
    ss << "configTarPath: " << configTarPath << "\n";
    ss << "baseConfigPath: " << baseConfigPath << "\n";
    ss << "etcDirectory: " << etcDirectory << "\n";
    ss << "voaConfigPath: " << voaConfigPath << "\n";
    ss << "pluginArtifactsBaseDir: " << pluginArtifactsBaseDir << "\n";
    ss << "jaegerConfigPath: " << jaegerConfigPath << "\n";
    ss << "sdkFilePath: " << sdkFilePath << "\n";
    ss << "userResponsesFilePath: " << userResponsesFilePath << "\n";
    ss << "tmpDirectory: " << tmpDirectory << "\n";
    ss << "logDirectory: " << logDirectory << "\n";
    ss << "logFilePath: " << logFilePath << "\n";
    ss << "voaConfigPath: " << voaConfigPath << "\n";
    ss << "nodeType: " << RaceEnums::nodeTypeToString(nodeType) << "\n";
    ss << "encryptionType: " << RaceEnums::storageEncryptionTypeToString(encryptionType) << "\n";

    ss << " --- App Config End --- \n";
    return ss.str();
}

std::string AppConfig::nodeTypeString() const {
    return RaceEnums::nodeTypeToString(nodeType);
}
