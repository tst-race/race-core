
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

#include "Composition.h"

#include <exception>
#include <sstream>

#include "helper.h"

Composition::Composition(const std::string &id, const std::string &transport,
                         const std::string &usermodel, const std::vector<std::string> &encodings,
                         RaceEnums::NodeType nodeType, const std::string &platform,
                         const std::string &architecture) :
    id(id),
    transport(transport),
    usermodel(usermodel),
    encodings(encodings),
    nodeType(nodeType),
    platform(platform),
    architecture(architecture) {}
Composition::Composition() : nodeType(RaceEnums::NT_UNDEF) {}

std::string Composition::description() const {
    std::stringstream ss;
    ss << "Composite comms: " << id;
    ss << ", transport: " << transport;
    ss << ", usermodel: " << usermodel;
    ss << ", encodings: " << nlohmann::json{encodings};
    ss << ", from plugins {";
    for (auto &plugin : plugins) {
        ss << plugin.filePath << ", ";
    }
    ss << "}";
    return ss.str();
}

void to_json(nlohmann::json &j, const Composition &composition) {
    j = nlohmann::json{{"id", composition.id},
                       {"transport", composition.transport},
                       {"usermodel", composition.usermodel},
                       {"encodings", composition.encodings},
                       {"node_type", composition.nodeType},
                       {"platform", composition.platform},
                       {"architecture", composition.architecture}};
}

void from_json(const nlohmann::json &j, Composition &composition) {
    composition.id = j.at("id");
    composition.transport = j.at("transport");
    composition.usermodel = j.at("usermodel");
    composition.encodings = j.at("encodings").get<std::vector<std::string>>();

    composition.nodeType = RaceEnums::stringToNodeType(j.at("node_type"));
    composition.platform = j.at("platform");
    composition.architecture = j.at("architecture");
}
