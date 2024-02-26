
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

#ifndef __RACE_CONFIG_H_
#define __RACE_CONFIG_H_

#include <AppConfig.h>
#include <ChannelProperties.h>
#include <RaceLog.h>

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

#include "Composition.h"
#include "PluginDef.h"

class RaceConfig {
public:
    RaceConfig();
    explicit RaceConfig(const AppConfig &appConfig,
                        const std::vector<std::uint8_t> &raceJsonContents);

    std::string androidPythonPath;
    std::unordered_map<RaceEnums::PluginType, std::vector<PluginDef>> plugins;
    std::vector<ChannelProperties> channels;
    std::vector<Composition> compositions;
    std::vector<std::string> initialEnabledChannels;
    std::unordered_map<std::string, std::vector<std::string>> environmentTags;
    bool isPluginFetchOnStartEnabled;
    bool isVoaEnabled;
    size_t wrapperQueueMaxSize;
    size_t wrapperTotalMaxSize;
    RaceLog::LogLevel logLevel;
    RaceLog::LogLevel logLevelStdout;
    bool logRaceConfig;
    bool logNMConfig;
    bool logCommsConfig;
    unsigned long msgLogLength;

    std::string env;

    void log() const;

    class race_config_parsing_exception : public std::exception {
    public:
        explicit race_config_parsing_exception(const std::string &msg_) : msg(msg_) {}
        const char *what() const noexcept override {
            return msg.c_str();
        }

    private:
        std::string msg;
    };

    std::vector<PluginDef> getNMPluginDefs() const;

    std::vector<PluginDef> getCommsPluginDefs() const;

    std::vector<PluginDef> getArtifactManagerPluginDefs() const;

protected:
    void initializeFromConfig(const AppConfig &appConfig, const std::string &raceJsonContents);
    std::string readConfigFile(const std::string &raceConfigPath);
    void parseConfigString(const std::string &config, const AppConfig &appConfig);
    RaceLog::LogLevel stringToLogLevel(std::string logLevel);
    bool to_bool(std::string str);
    std::string bool_to_string(bool b);
    void validatePluginDefs();

    ChannelProperties parseChannelProperties(const nlohmann::json &channelPropertiesJson) const;
    bool parseLinkPropertyPair(const nlohmann::json &propsJson, const std::string &fieldName,
                               LinkPropertyPair &pair, const std::string &channelGid) const;
    bool parseLinkPropertySet(const nlohmann::json &propsJson, const std::string &fieldName,
                              LinkPropertySet &set, const std::string &channelGid,
                              const std::string &pairField) const;
    bool parseRoles(const nlohmann::json &propsJson, std::vector<ChannelRole> &roles,
                    const std::string &fieldName, const std::string &channelGid) const;
};

#endif
