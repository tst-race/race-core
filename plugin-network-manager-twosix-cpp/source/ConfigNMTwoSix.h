
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

#ifndef __CONFIG_NETWORK_MANAGER_TWOSIX_H_
#define __CONFIG_NETWORK_MANAGER_TWOSIX_H_

#include <IRaceSdkNM.h>

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using PersonaSet = std::unordered_set<std::string>;
using PersonaVector = std::vector<std::string>;

/** Base configuration for clients and servers */
struct ConfigNMTwoSixBase {
    std::unordered_map<std::string, std::string> channelRoles;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> expectedLinks;
    bool useLinkWizard{true};
    uint64_t bootstrapHandle{0};
    std::string bootstrapIntroducer;
    double lookbackSeconds{60.0};
    PersonaSet otherConnections;
};

/** Expected multicast link configuration */
struct ExpectedMulticastLink {
    PersonaVector personas;
    std::string channelGid;
    std::string linkSide;
};

// Enable automatic conversion to/from json
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ExpectedMulticastLink, personas, channelGid, linkSide);

/** Client-specific configuration */
struct ConfigNMTwoSixClient : public ConfigNMTwoSixBase {
    PersonaVector entranceCommittee;
    PersonaVector exitCommittee;
    std::vector<ExpectedMulticastLink> expectedMulticastLinks;
    size_t maxSeenMessages{10000};
};

// Enable automatic conversion to/from json
void to_json(nlohmann::json &destJson, const ConfigNMTwoSixClient &srcConfig);
void from_json(const nlohmann::json &srcJson, ConfigNMTwoSixClient &destConfig);

/**
 * @brief Read and parse the config file using the SDK storage API to populate the given
 *        destination client configuration object
 *
 * @param sdk RACE SDK
 * @param destConfig Destination client configuration instance
 * @return true if successful in reading and populating the client configuration
 */
bool loadClientConfig(IRaceSdkNM &sdk, ConfigNMTwoSixClient &destConfig);

/**
 * @brief Write the given source client configuration using the SDK storage API
 *
 * @param sdk RACE SDK
 * @param srcConfig Source client configuration instance
 * @return true if successful in writing the client configuration
 */
bool writeClientConfig(IRaceSdkNM &sdk, const ConfigNMTwoSixClient &srcConfig);

/** Server ring configuration entry */
struct RingEntry {
    std::int32_t length;
    std::string next;
};

using RingVector = std::vector<RingEntry>;

// Enable automatic conversion to/from json
void to_json(nlohmann::json &destJson, const RingEntry &srcRing);
void from_json(const nlohmann::json &srcJson, RingEntry &destRing);

/** Server-specific configuration */
struct ConfigNMTwoSixServer : public ConfigNMTwoSixBase {
    PersonaSet exitClients;
    PersonaSet committeeClients;
    std::string committeeName;
    std::unordered_map<std::string, PersonaVector> reachableCommittees;
    size_t maxStaleUuids{1000000};
    size_t maxFloodedUuids{1000000};
    size_t floodingFactor{2};
    RingVector rings;
};

// Enable automatic conversion to/from json
void to_json(nlohmann::json &destJson, const ConfigNMTwoSixServer &srcConfig);
void from_json(const nlohmann::json &srcJson, ConfigNMTwoSixServer &destConfig);

/**
 * @brief Read and parse the config file using the SDK storage API to populate the given
 *        destination server configuration object.
 *
 * @param sdk RACE SDK
 * @param destConfig Destination server configuration instance
 * @return true if successful in reading and populating the server configuration
 */
bool loadServerConfig(IRaceSdkNM &sdk, ConfigNMTwoSixServer &destConfig);

/**
 * @brief Write the given source server configuration using the SDK storage API
 *
 * @param sdk RACE SDK
 * @param srcConfig Source server configuration instance
 * @return true if successful in writing the server configuration
 */
bool writeServerConfig(IRaceSdkNM &sdk, const ConfigNMTwoSixServer &srcConfig);

#endif
