
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

#ifndef __APP_CONFIG_H_
#define __APP_CONFIG_H_

#include <string>

#include "RaceEnums.h"

class AppConfig {
public:
    AppConfig();

    std::string persona;
    std::string appDir;
    std::string pluginArtifactsBaseDir;
    std::string platform;
    std::string architecture;
    /// Type of environment the RACE node will be run on (phone, enterprise-server, desktop, etc.)
    std::string environment;
    // Config Files
    std::string configTarPath;
    std::string baseConfigPath;
    // Testing specific files (user-responses.json, jaeger-config.json, voa.json)
    std::string etcDirectory;
    std::string jaegerConfigPath;
    std::string userResponsesFilePath;
    std::string voaConfigPath;
    // Bootstrap Directories
    std::string bootstrapFilesDirectory;
    std::string bootstrapCacheDirectory;

    std::string sdkFilePath;

    std::string tmpDirectory;
    std::string logDirectory;
    std::string logFilePath;

    std::string appPath;

    RaceEnums::NodeType nodeType;
    RaceEnums::StorageEncryptionType encryptionType;

    std::string nodeTypeString() const;

    std::string to_string() const;
};

#endif
