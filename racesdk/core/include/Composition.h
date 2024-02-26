
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

#ifndef __COMPOSITION_H_
#define __COMPOSITION_H_

#include <nlohmann/json.hpp>

#include "PluginDef.h"
#include "RaceEnums.h"

class Composition {
public:
    Composition(const std::string &id, const std::string &transport, const std::string &usermodel,
                const std::vector<std::string> &encodings, RaceEnums::NodeType nodeType,
                const std::string &platform, const std::string &architecture);
    Composition();
    std::string description() const;

public:
    std::string id;
    std::string transport;
    std::string usermodel;
    std::vector<std::string> encodings;

    RaceEnums::NodeType nodeType;
    std::string platform;
    std::string architecture;

    std::vector<PluginDef> plugins;
};

void to_json(nlohmann::json &j, const Composition &composition);
void from_json(const nlohmann::json &j, Composition &composition);

#endif
