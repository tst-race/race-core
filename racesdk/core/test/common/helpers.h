
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

#ifndef __TEST_COMMON_HELPERS_H__
#define __TEST_COMMON_HELPERS_H__

#include <AppConfig.h>
#include <LinkProperties.h>
#include <RaceConfig.h>

#include "../../source/filesystem.h"

inline void replace_directory(std::string path) {
    fs::remove_all(path);
    fs::create_directories(path);
}

// delete all the directories and recreate them to prevent files from old tests interfering
inline void createAppDirectories(const AppConfig &config) {
    replace_directory(config.appDir);
    replace_directory(config.baseConfigPath);
    replace_directory(config.etcDirectory);
    replace_directory(config.bootstrapFilesDirectory);
    replace_directory(config.bootstrapCacheDirectory);
    replace_directory(config.tmpDirectory);
    replace_directory(config.logDirectory);
    replace_directory(config.voaConfigPath);
}

inline AppConfig createDefaultAppConfig() {
    AppConfig config;

    // variables
    config.nodeType = RaceEnums::NodeType::NT_CLIENT;
    config.persona = "test persona";
    config.sdkFilePath = "sdk";

    // directories
    config.appDir = "/tmp/test-files/appDir";
    // Configs
    config.configTarPath = "/tmp/test-files/configs.tar";
    config.baseConfigPath = "/tmp/test-files/baseConfigPath";
    // Testing specific files (user-responses.json, jaeger-config.json, voa.json)
    config.etcDirectory = "/tmp/test-files/etc";
    config.jaegerConfigPath = "";
    config.userResponsesFilePath = "/tmp/test-files/etc/userResponsesFilePath";
    config.voaConfigPath = "/tmp/test-files/voaConfigPath";
    // Bootstrap Directories
    config.bootstrapFilesDirectory = "/tmp/test-files/bootstrapFilesDirectory";
    config.bootstrapCacheDirectory = "/tmp/test-files/bootstrapCacheDirectory";

    config.tmpDirectory = "/tmp/test-files/tmpDirectory";
    config.logDirectory = "/tmp/test-files/logDirectory";
    config.logFilePath = "/tmp/test-files/logFilePath";

    return config;
}

inline RaceConfig createDefaultRaceConfig() {
    RaceConfig config;
    config.androidPythonPath = "";
    // config.plugins;
    // config.enabledChannels;
    config.isPluginFetchOnStartEnabled = true;
    config.isVoaEnabled = true;
    config.wrapperQueueMaxSize = 1000000;
    config.wrapperTotalMaxSize = 1000000000;
    config.logLevel = RaceLog::LL_DEBUG;
    config.logRaceConfig = false;
    config.logNMConfig = false;
    config.logCommsConfig = false;
    config.msgLogLength = 256;

    config.environmentTags = {{"", {}}};

    return config;
}

// Get a default set of link properties with values not set as *_UNDEF.
inline LinkProperties getDefaultLinkProperties() {
    LinkProperties props;
    props.linkType = LT_SEND;
    props.transmissionType = TT_UNICAST;
    props.connectionType = CT_DIRECT;
    props.sendType = ST_STORED_ASYNC;
    return props;
}

#endif
