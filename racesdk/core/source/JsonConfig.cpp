
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

#include "JsonConfig.h"

JsonConfig::JsonConfig() {}

JsonConfig::JsonConfig(const std::string &configPath) {
    initializeFromConfig(configPath);
}

void JsonConfig::initializeFromConfig(const std::string &configPath) {
    helper::logInfo("JsonConfig::initializeFromConfig initializing config from file: " +
                    configPath);

    // read Json config file
    std::string config;
    try {
        config = readConfigFile(configPath);
        parseConfigString(config);
    } catch (...) {
        helper::logWarning("JsonConfig::initializeFromConfig Failed to read config:" + configPath);
        configJson = nlohmann::json::array();
    }
}

std::string JsonConfig::readConfigFile(const std::string &configPath) {
    std::ifstream configFile(configPath);
    nlohmann::json configJsonString = nlohmann::json::parse(configFile);
    return configJsonString.dump();
}

void JsonConfig::parseConfigString(const std::string &config) {
    try {
        configJson = nlohmann::json::parse(config);
    } catch (nlohmann::json::exception &error) {
        throw race_config_parsing_exception(
            "JsonConfig::initializeFromConfig Failed to parse config : " + config +
            std::string(error.what()));
    } catch (std::exception &error) {
        throw race_config_parsing_exception(
            "JsonConfig::initializeFromConfig Failed to parse config : " + config +
            std::string(error.what()));
    }
}

// convert string to boolean but default to false
bool JsonConfig::to_bool(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    if (str.compare("true") == 0) {
        return true;
    } else if (str.compare("false") == 0) {
        return false;
    }
    throw race_config_parsing_exception(str + " is not a boolean.");
}