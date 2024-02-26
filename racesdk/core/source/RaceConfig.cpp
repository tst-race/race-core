
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

#include "../include/RaceConfig.h"

#include <inttypes.h>

#include <boost/io/ios_state.hpp>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

#include "helper.h"

RaceConfig::RaceConfig() :
    plugins({{RaceEnums::PluginType::PT_NM, {}},
             {RaceEnums::PluginType::PT_COMMS, {}},
             {RaceEnums::PluginType::PT_ARTIFACT_MANAGER, {}}}),
    isPluginFetchOnStartEnabled(false),
    isVoaEnabled(true),
    // 10MB queue size for plugins, There's no reason for this being the default, just seems fine.
    wrapperQueueMaxSize(10 * 1024 * 1024),
    // 2GB total size for all the plugin queues combined, This is >200 times the single queue limit,
    // so it shoudn't get hit.
    wrapperTotalMaxSize(2048 * 1024 * 1024ul),
    logLevel(RaceLog::LL_DEBUG),
    logLevelStdout(RaceLog::LL_WARNING),
    logRaceConfig(true),
    logNMConfig(true),
    logCommsConfig(true),
    msgLogLength(256) {}

RaceConfig::RaceConfig(const AppConfig &config, const std::vector<std::uint8_t> &raceJsonContents) :
    RaceConfig() {
    initializeFromConfig(config, std::string(raceJsonContents.begin(), raceJsonContents.end()));
}

void RaceConfig::log() const {
    std::ostream &o = RaceLog::getLogStream(RaceLog::LL_INFO);
    boost::io::ios_flags_saver flags(o, std::ios_base::dec | std::ios_base::boolalpha);
    o << " --- Race Config Begin --- \n";

    o << "isPluginFetchOnStartEnabled: " << isPluginFetchOnStartEnabled << "\n";
    o << "isVoaEnabled: " << isVoaEnabled << "\n";
    o << "wrapperQueueMaxSize: " << wrapperQueueMaxSize << "\n";
    o << "wrapperTotalMaxSize: " << wrapperTotalMaxSize << "\n";
    o << "logLevel: " << logLevel << "\n";
    o << "logRaceConfig: " << logRaceConfig << "\n";
    o << "logNMConfig: " << logNMConfig << "\n";
    o << "logCommsConfig: " << logCommsConfig << "\n";
    o << "msgLogLength: " << msgLogLength << "\n";
    o << "network manager plugins: ";
    for (auto &networkManager : getNMPluginDefs()) {
        o << networkManager.filePath << ' ';
    }
    o << "\n";

    o << "comms plugins:\n";
    for (auto &comms : getCommsPluginDefs()) {
        o << comms.filePath << '/' << comms.sharedLibraryPath << ", ";
        json array;
        array = comms.channels;
        o << "channels: " << array << ", ";
        array = comms.transports;
        o << "transports: " << array << ", ";
        array = comms.usermodels;
        o << "usermodels: " << array << ", ";
        array = comms.encodings;
        o << "encodings: " << array << "\n";
    }

    o << "artifact manager plugins: ";
    for (auto &amp : getArtifactManagerPluginDefs()) {
        o << amp.filePath << ' ';
    }
    o << "\n";

    o << "channels: "
      << "\n";
    for (auto &channel : channels) {
        o << channelPropertiesToString(channel) << "\n";
    }
    o << "\n";

    o << "compositions: "
      << "\n";
    for (auto &composition : compositions) {
        o << composition.description() << "\n";
    }
    o << "\n";

    o << "initial enabled channels: "
      << "\n";
    for (auto &channelGid : initialEnabledChannels) {
        o << channelGid << "\n";
    }
    o << "\n";

    o << " --- Race Config End --- \n";
    o << std::flush;
}

void RaceConfig::initializeFromConfig(const AppConfig &config,
                                      const std::string &raceJsonContents) {
    helper::logInfo("initializing RACE config");

    // Parse json string
    try {
        parseConfigString(raceJsonContents, config);
    } catch (race_config_parsing_exception &e) {
        helper::logWarning(e.what());
        return;
    }
}

std::string RaceConfig::readConfigFile(const std::string &raceConfigPath) {
    try {
        std::ifstream configFile(raceConfigPath);
        nlohmann::json configJson = nlohmann::json::parse(configFile);
        return configJson.dump();
    } catch (...) {
        throw race_config_parsing_exception(
            "getLogConfigs: failed to parse json. Defaulting to LogLevel = DEBUG, logging all "
            "configs, and log file path = " +
            raceConfigPath);
    }
}

void RaceConfig::parseConfigString(const std::string &configString, const AppConfig &appConfig) {
    MAKE_LOG_PREFIX();
    try {
        // the config json has strings not booleans because if you use the RiB commands to edit the
        // config file RiB writes the values as strings
        nlohmann::json configJson = nlohmann::json::parse(configString);

        androidPythonPath = configJson.at("android_python_path").get<std::string>();

        if (configJson.contains("initial_enabled_channels")) {
            initialEnabledChannels =
                configJson.at("initial_enabled_channels").get<std::vector<std::string>>();
        }

        environmentTags = configJson.at("environment_tags")
                              .get<std::unordered_map<std::string, std::vector<std::string>>>();

        // App will set env prior to calling initRaceSystem if real user input is provided.
        // If not, env will default to "" and will be obtained from user-responses.json
        env = appConfig.environment;

        // the following are optional, but must be of correct type if they exist
        isPluginFetchOnStartEnabled = to_bool(configJson.value(
            "isPluginFetchOnStartEnabled", bool_to_string(isPluginFetchOnStartEnabled)));

        isVoaEnabled = to_bool(configJson.value("isVoaEnabled", bool_to_string(isVoaEnabled)));

        wrapperQueueMaxSize =
            std::stoul(configJson.value("max_queue_size", std::to_string(wrapperQueueMaxSize)));

        wrapperTotalMaxSize =
            std::stoul(configJson.value("max_size", std::to_string(wrapperTotalMaxSize)));

        logLevel = stringToLogLevel(configJson.value("level", "DEBUG"));
        logRaceConfig = to_bool(configJson.value("log-race-config", bool_to_string(logRaceConfig)));
        logNMConfig =
            to_bool(configJson.value("log-network-manager-config", bool_to_string(logNMConfig)));
        logCommsConfig =
            to_bool(configJson.value("log-comms-config", bool_to_string(logCommsConfig)));
        msgLogLength = std::stoul(configJson.value("msg-log-length", std::to_string(msgLogLength)));

        // Parse plugin defs

        for (auto &channelPropertiesJson : configJson.at("channels")) {
            try {
                channels.push_back(parseChannelProperties(channelPropertiesJson));
            } catch (std::exception &e) {
                // if the channel properties are invalid in some way, e.g. missing a field, continue
                // on, but don't add them to the channel properties list
                helper::logError(logPrefix + std::string(e.what()));
                helper::logError(logPrefix +
                                 "channelPropertiesJson: " + channelPropertiesJson.dump());
            }
        }

        std::unordered_map<std::string, PluginDef> componentPluginMap;
        auto pluginJsons = configJson.at("plugins");
        for (auto &pluginJson : pluginJsons) {
            // Check that the plugin is intended for this node and platform. If not then skip it.
            auto pluginDef = PluginDef::pluginJsonToPluginDef(pluginJson);
            helper::logInfo("Found plugin: " + pluginDef.shardName);

            if ((pluginDef.nodeType != RaceEnums::NT_ALL &&
                 pluginDef.nodeType != appConfig.nodeType) ||
                pluginDef.platform != appConfig.platform ||
                pluginDef.architecture != appConfig.architecture) {
                helper::logInfo("Discarding plugin: " + pluginDef.shardName);
                continue;
            }

            // push onto the list of network manager/comms plugins
            plugins[pluginDef.type].push_back(pluginDef);

            for (auto &transport : pluginDef.transports) {
                auto it = componentPluginMap.find(transport);
                if (it != componentPluginMap.end()) {
                    helper::logError(logPrefix + "component " + transport +
                                     " already exists. Previous transport supplied by " +
                                     it->second.filePath + ". New transport supplied by " +
                                     pluginDef.filePath);

                    throw std::runtime_error("Multiple definitions of transport " + transport);
                }
                componentPluginMap[transport] = pluginDef;
            }

            for (auto &usermodel : pluginDef.usermodels) {
                auto it = componentPluginMap.find(usermodel);
                if (it != componentPluginMap.end()) {
                    helper::logError(logPrefix + "component " + usermodel +
                                     " already exists. Previous usermodel supplied by " +
                                     it->second.filePath + ". New usermodel supplied by " +
                                     pluginDef.filePath);

                    throw std::runtime_error("Multiple definitions of usermodel " + usermodel);
                }
                componentPluginMap[usermodel] = pluginDef;
            }

            for (auto &encoding : pluginDef.encodings) {
                auto it = componentPluginMap.find(encoding);
                if (it != componentPluginMap.end()) {
                    helper::logError(logPrefix + "component " + encoding +
                                     " already exists. Previous encoding supplied by " +
                                     it->second.filePath + ". New encoding supplied by " +
                                     pluginDef.filePath);

                    throw std::runtime_error("Multiple definitions of encoding " + encoding);
                }
                componentPluginMap[encoding] = pluginDef;
            }
        }

        auto compostionJsons = configJson.value("compositions", std::vector<json>{});
        for (auto &compostionJson : compostionJsons) {
            // Check that the plugin is intended for this node and platform. If not then skip it.
            Composition composition = compostionJson;
            helper::logInfo("Found composition: " + composition.id);

            if ((composition.nodeType != RaceEnums::NT_ALL &&
                 composition.nodeType != appConfig.nodeType) ||
                composition.platform != appConfig.platform ||
                composition.architecture != appConfig.architecture) {
                helper::logInfo("Discarding plugin: " + composition.id);
                continue;
            }

            composition.plugins.push_back(componentPluginMap.at(composition.transport));
            composition.plugins.push_back(componentPluginMap.at(composition.usermodel));
            for (auto &encoding : composition.encodings) {
                composition.plugins.push_back(componentPluginMap.at(encoding));
            }

            // push onto the list of network manager/comms plugins
            compositions.push_back(composition);
        }

        validatePluginDefs();
    } catch (nlohmann::json::exception &error) {
        std::string errorMessage =
            "RaceConfig: failed to parse race config json: " + std::string(error.what());
        helper::logError(errorMessage);
        throw race_config_parsing_exception(errorMessage);
    } catch (std::exception &error) {
        std::string errorMessage =
            "RaceConfig: failed to parse race config json: " + std::string(error.what());
        helper::logError(errorMessage);
        throw race_config_parsing_exception(errorMessage);
    }
}

RaceLog::LogLevel RaceConfig::stringToLogLevel(std::string logLevelStr) {
    if (logLevelStr.compare("DEBUG") == 0) {
        return RaceLog::LL_DEBUG;
    } else if (logLevelStr.compare("INFO") == 0) {
        return RaceLog::LL_INFO;
    } else if (logLevelStr.compare("WARNING") == 0) {
        return RaceLog::LL_WARNING;
    } else if (logLevelStr.compare("ERROR") == 0) {
        return RaceLog::LL_ERROR;
    } else {
        std::string errorMessage = "Invalid log level specified in logging.json: " + logLevelStr;
        helper::logError(errorMessage);
        throw race_config_parsing_exception(errorMessage);
    }
}

// convert string to boolean but default to false
bool RaceConfig::to_bool(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    if (str.compare("true") == 0) {
        return true;
    } else if (str.compare("false") == 0) {
        return false;
    }
    throw race_config_parsing_exception(str + " is not a boolean.");
}

std::string RaceConfig::bool_to_string(bool b) {
    return b ? "true" : "false";
}

void RaceConfig::validatePluginDefs() {
    const size_t numNMPlugins = this->getNMPluginDefs().size();
    const size_t numCommsPlugins = this->getCommsPluginDefs().size();

    std::string errorMessage;
    if (numNMPlugins > 1) {
        errorMessage +=
            "Multiple network manager plugins were specified. This is invalid. Please update your "
            "configuration and run again.\n";
    } else if (numNMPlugins == 0) {
        errorMessage +=
            "No network manager plugin specified. This is invalid. Please update your "
            "configuration and run "
            "again.\n";
    }
    if (numCommsPlugins == 0) {
        errorMessage +=
            "No comms plugins were specified. This is invalid. Please update your configuration "
            "and run again.\n";
    }

    if (!errorMessage.empty()) {
        helper::logError("validatePluginDefs: " + errorMessage);
        throw race_config_parsing_exception(errorMessage);
    }
}

std::vector<PluginDef> RaceConfig::getNMPluginDefs() const {
    return plugins.at(RaceEnums::PluginType::PT_NM);
}

std::vector<PluginDef> RaceConfig::getCommsPluginDefs() const {
    return plugins.at(RaceEnums::PluginType::PT_COMMS);
}

std::vector<PluginDef> RaceConfig::getArtifactManagerPluginDefs() const {
    return plugins.at(RaceEnums::PluginType::PT_ARTIFACT_MANAGER);
}

template <typename T>
static bool parseField(const json &config, T &dest, const std::string &fieldName,
                       const std::string &channelGid) {
    try {
        dest = config.at(fieldName).get<T>();
        return true;
    } catch (std::exception &e) {
        helper::logError("Failed to parse " + fieldName + " from channel '" + channelGid +
                         "': " + std::string(e.what()));
        return false;
    }
}

ChannelProperties RaceConfig::parseChannelProperties(const nlohmann::json &propsJson) const {
    bool success = true;

    ChannelProperties props;
    props.channelStatus = CHANNEL_UNSUPPORTED;
    props.channelGid = "<missing channelGid>";

    success &= parseField(propsJson, props.channelGid, "channelGid", props.channelGid);

    success &= parseField(propsJson, props.bootstrap, "bootstrap", props.channelGid);
    success &= parseField(propsJson, props.duration_s, "duration_s", props.channelGid);
    success &= parseField(propsJson, props.isFlushable, "isFlushable", props.channelGid);
    success &= parseField(propsJson, props.mtu, "mtu", props.channelGid);
    success &= parseField(propsJson, props.multiAddressable, "multiAddressable", props.channelGid);
    success &= parseField(propsJson, props.period_s, "period_s", props.channelGid);
    success &= parseField(propsJson, props.reliable, "reliable", props.channelGid);
    success &= parseField(propsJson, props.supported_hints, "supported_hints", props.channelGid);
    success &= parseField(propsJson, props.maxLinks, "maxLinks", props.channelGid);
    success &=
        parseField(propsJson, props.creatorsPerLoader, "creatorsPerLoader", props.channelGid);
    success &=
        parseField(propsJson, props.loadersPerCreator, "loadersPerCreator", props.channelGid);
    success &= parseLinkPropertyPair(propsJson, "creatorExpected", props.creatorExpected,
                                     props.channelGid);
    success &=
        parseLinkPropertyPair(propsJson, "loaderExpected", props.loaderExpected, props.channelGid);

    std::string tmp;

    tmp = connectionTypeToString(CT_UNDEF);
    success &= parseField(propsJson, tmp, "connectionType", props.channelGid);
    props.connectionType = connectionTypeFromString(tmp);

    tmp = linkDirectionToString(LD_UNDEF);
    success &= parseField(propsJson, tmp, "linkDirection", props.channelGid);
    props.linkDirection = linkDirectionFromString(tmp);

    tmp = sendTypeToString(ST_UNDEF);
    success &= parseField(propsJson, tmp, "sendType", props.channelGid);
    props.sendType = sendTypeFromString(tmp);

    tmp = transmissionTypeToString(TT_UNDEF);
    success &= parseField(propsJson, tmp, "transmissionType", props.channelGid);
    props.transmissionType = transmissionTypeFromString(tmp);

    success &= parseRoles(propsJson, props.roles, "roles", props.channelGid);

    success &=
        parseField(propsJson, props.maxSendsPerInterval, "maxSendsPerInterval", props.channelGid);
    success &=
        parseField(propsJson, props.secondsPerInterval, "secondsPerInterval", props.channelGid);
    success &= parseField(propsJson, props.intervalEndTime, "intervalEndTime", props.channelGid);
    success &= parseField(propsJson, props.sendsRemainingInInterval, "sendsRemainingInInterval",
                          props.channelGid);

    if (!success) {
        throw race_config_parsing_exception("Failed to parse channel '" + props.channelGid + "'");
    }

    return props;
}

bool RaceConfig::parseLinkPropertyPair(const nlohmann::json &propsJson,
                                       const std::string &fieldName, LinkPropertyPair &pair,
                                       const std::string &channelGid) const {
    try {
        bool success = true;
        json lpPairJson = propsJson.at(fieldName);
        success &= parseLinkPropertySet(lpPairJson, "send", pair.send, channelGid, fieldName);
        success &= parseLinkPropertySet(lpPairJson, "receive", pair.receive, channelGid, fieldName);
        return success;
    } catch (std::exception &e) {
        helper::logError("Failed to parse " + fieldName + " from channel '" + channelGid +
                         "': " + std::string(e.what()));
        return false;
    }
}

bool RaceConfig::parseLinkPropertySet(const nlohmann::json &propsJson, const std::string &fieldName,
                                      LinkPropertySet &set, const std::string &channelGid,
                                      const std::string &pairField) const {
    try {
        bool success = true;
        json lpSetJson = propsJson.at(fieldName);
        success &= parseField(lpSetJson, set.bandwidth_bps, "bandwidth_bps", channelGid);
        success &= parseField(lpSetJson, set.latency_ms, "latency_ms", channelGid);
        success &= parseField(lpSetJson, set.loss, "loss", channelGid);
        return success;
    } catch (std::exception &e) {
        helper::logError("Failed to parse " + fieldName + " from channel '" + channelGid +
                         "', field '" + pairField + "': " + std::string(e.what()));
        return false;
    }
}

bool RaceConfig::parseRoles(const nlohmann::json &propsJson, std::vector<ChannelRole> &roles,
                            const std::string &fieldName, const std::string &channelGid) const {
    try {
        bool success = true;
        json rolesJson = propsJson.at(fieldName);

        for (auto &roleJson : rolesJson) {
            ChannelRole role;
            success &= parseField(roleJson, role.roleName, "roleName", channelGid);
            success &= parseField(roleJson, role.mechanicalTags, "mechanicalTags", channelGid);
            success &= parseField(roleJson, role.behavioralTags, "behavioralTags", channelGid);

            std::string tmp = linkSideToString(LS_UNDEF);
            success &= parseField(roleJson, tmp, "linkSide", channelGid);
            role.linkSide = linkSideFromString(tmp);

            roles.push_back(role);
        }

        return success;
    } catch (std::exception &e) {
        helper::logError("Failed to parse " + fieldName + " from channel '" + channelGid + ": " +
                         std::string(e.what()));
        return false;
    }
}
