
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

#include "ConfigNMTwoSix.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "JsonIO.h"
#include "Log.h"

using json = nlohmann::json;

static const std::string CONFIG_FILE_NAME = "config.json";

// Base config

void base_to_json(json &destJson, const ConfigNMTwoSixBase &srcConfig) {
    destJson = json{
        {"expectedLinks", srcConfig.expectedLinks},
        {"channelRoles", srcConfig.channelRoles},
        {"useLinkWizard", srcConfig.useLinkWizard},
        {"lookbackSeconds", srcConfig.lookbackSeconds},
        {"otherConnections", srcConfig.otherConnections},
    };

    if (srcConfig.bootstrapHandle != 0u && !srcConfig.bootstrapIntroducer.empty()) {
        destJson["bootstrapHandle"] = srcConfig.bootstrapHandle;
        destJson["bootstrapIntroducer"] = srcConfig.bootstrapIntroducer;
    }
}

void base_from_json(const json &srcJson, ConfigNMTwoSixBase &destConfig) {
    destConfig.channelRoles = srcJson.value("channelRoles", destConfig.channelRoles);
    destConfig.expectedLinks = srcJson.value("expectedLinks", destConfig.expectedLinks);
    destConfig.useLinkWizard = srcJson.value("useLinkWizard", destConfig.useLinkWizard);
    destConfig.bootstrapHandle = srcJson.value("bootstrapHandle", destConfig.bootstrapHandle);
    destConfig.bootstrapIntroducer =
        srcJson.value("bootstrapIntroducer", destConfig.bootstrapIntroducer);
    destConfig.lookbackSeconds = srcJson.value("lookbackSeconds", destConfig.lookbackSeconds);
    destConfig.otherConnections = srcJson.value("otherConnections", destConfig.otherConnections);
}

// Client config

void to_json(json &destJson, const ConfigNMTwoSixClient &srcConfig) {
    base_to_json(destJson, srcConfig);
    destJson.update(json{
        {"entranceCommittee", srcConfig.entranceCommittee},
        {"exitCommittee", srcConfig.exitCommittee},
        {"expectedMulticastLinks", srcConfig.expectedMulticastLinks},
        {"maxSeenMessages", srcConfig.maxSeenMessages},
    });
}

void from_json(const json &srcJson, ConfigNMTwoSixClient &destConfig) {
    base_from_json(srcJson, destConfig);
    destConfig.entranceCommittee = srcJson.value("entranceCommittee", destConfig.entranceCommittee);
    destConfig.exitCommittee = srcJson.value("exitCommittee", destConfig.exitCommittee);
    destConfig.expectedMulticastLinks =
        srcJson.value("expectedMulticastLinks", destConfig.expectedMulticastLinks);
    destConfig.maxSeenMessages = srcJson.value("maxSeenMessages", destConfig.maxSeenMessages);
}

bool loadClientConfig(IRaceSdkNM &sdk, ConfigNMTwoSixClient &destConfig) {
    TRACE_FUNCTION();

    try {
        destConfig = JsonIO::loadJson(sdk, CONFIG_FILE_NAME);
        return true;
    } catch (std::exception &error) {
        logError(logPrefix + "Unable to parse config: " + error.what());
        return false;
    }
}

bool writeClientConfig(IRaceSdkNM &sdk, const ConfigNMTwoSixClient &srcConfig) {
    TRACE_FUNCTION();

    try {
        return JsonIO::writeJson(sdk, CONFIG_FILE_NAME, srcConfig);
    } catch (std::exception &error) {
        logError(logPrefix + "Failed to write config: " + error.what());
        return false;
    }
}

// Server config

void to_json(json &destJson, const RingEntry &srcRing) {
    destJson = json{
        {"length", srcRing.length},
        {"next", srcRing.next},
    };
}

void from_json(const json &srcJson, RingEntry &destRing) {
    srcJson.at("length").get_to(destRing.length);
    srcJson.at("next").get_to(destRing.next);
}

void to_json(json &destJson, const ConfigNMTwoSixServer &srcConfig) {
    base_to_json(destJson, srcConfig);
    destJson.update(json{
        {"exitClients", srcConfig.exitClients},
        {"committeeClients", srcConfig.committeeClients},
        {"committeeName", srcConfig.committeeName},
        {"reachableCommittees", srcConfig.reachableCommittees},
        {"maxStaleUuids", srcConfig.maxStaleUuids},
        {"maxFloodedUuids", srcConfig.maxFloodedUuids},
        {"floodingFactor", srcConfig.floodingFactor},
        {"rings", srcConfig.rings},
    });
}

void from_json(const json &srcJson, ConfigNMTwoSixServer &destConfig) {
    base_from_json(srcJson, destConfig);
    destConfig.exitClients = srcJson.value("exitClients", destConfig.exitClients);
    destConfig.committeeClients = srcJson.value("committeeClients", destConfig.committeeClients);
    destConfig.committeeName = srcJson.value("committeeName", destConfig.committeeName);
    destConfig.reachableCommittees =
        srcJson.value("reachableCommittees", destConfig.reachableCommittees);
    destConfig.maxStaleUuids = srcJson.value("maxStaleUuids", destConfig.maxStaleUuids);
    destConfig.maxFloodedUuids = srcJson.value("maxFloodedUuids", destConfig.maxFloodedUuids);
    destConfig.floodingFactor = srcJson.value("floodingFactor", destConfig.floodingFactor);
    destConfig.rings = srcJson.value("rings", destConfig.rings);
}

bool loadServerConfig(IRaceSdkNM &sdk, ConfigNMTwoSixServer &destConfig) {
    TRACE_FUNCTION();

    try {
        destConfig = JsonIO::loadJson(sdk, CONFIG_FILE_NAME);
        return true;
    } catch (std::exception &error) {
        logError(logPrefix + "Unable to parse config: " + error.what());
        return false;
    }
}

bool writeServerConfig(IRaceSdkNM &sdk, const ConfigNMTwoSixServer &srcConfig) {
    TRACE_FUNCTION();

    try {
        return JsonIO::writeJson(sdk, CONFIG_FILE_NAME, srcConfig);
    } catch (std::exception &error) {
        logError(logPrefix + "Failed to write config: " + error.what());
        return false;
    }
}
