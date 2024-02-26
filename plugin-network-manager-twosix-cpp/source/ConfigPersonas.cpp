
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

#include "ConfigPersonas.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "JsonIO.h"
#include "Log.h"

using json = nlohmann::json;

#define AES_KEY_LENGTH 32

static const std::string displayNameKey = "displayName";
static const std::string raceUuidKey = "raceUuid";
static const std::string personaTypeKey = "personaType";
static const std::string aesKeyFileKey = "aesKeyFile";

bool isValidPersona(const json &persona) {
    return (persona.is_object() && persona.find(displayNameKey) != persona.end() &&
            persona.find(raceUuidKey) != persona.end() &&
            persona.find(aesKeyFileKey) != persona.end() &&
            persona.find(personaTypeKey) != persona.end() &&
            (persona[personaTypeKey] == "client" || persona[personaTypeKey] == "server" ||
             persona[personaTypeKey] == "registry"));
};

bool ConfigPersonas::init(IRaceSdkNM &sdk, const std::string &configPath) {
    std::string configFilePath = configPath + "/race-personas.json";
    json configJson = JsonIO::loadJson(sdk, configFilePath);

    if (!configJson.is_array()) {
        logError("invalid json config: " + configJson.dump());
        return false;
    }

    for (const auto &persona : configJson) {
        if (!isValidPersona(persona)) {
            logError("Invalid persona found in config: " + persona.dump());
            return false;
        }

        personas.emplace_back();
        Persona &currentPersona = personas.back();
        currentPersona.setDisplayName(persona[displayNameKey]);
        currentPersona.setRaceUuid(persona[raceUuidKey]);
        currentPersona.setAesKeyFile(persona[aesKeyFileKey]);

        std::string keyPath = configPath + "/" + persona.value(aesKeyFileKey, "");
        std::vector<uint8_t> aesKey = sdk.readFile(keyPath);

        if (aesKey.size() != AES_KEY_LENGTH) {
            logError("invalid aesKey for persona: " + currentPersona.getRaceUuid());
            return false;
        }

        currentPersona.setAesKey(aesKey);

        if (persona[personaTypeKey] == "client") {
            currentPersona.setPersonaType(P_CLIENT);
        } else if (persona[personaTypeKey] == "registry") {
            currentPersona.setPersonaType(P_REGISTRY);
        } else if (persona[personaTypeKey] == "server") {
            currentPersona.setPersonaType(P_SERVER);
        }
    }

    return true;
}

bool ConfigPersonas::write(IRaceSdkNM &sdk, const std::string &configPath) {
    json configJson;

    for (const auto &persona : personas) {
        json personaJson;
        personaJson[displayNameKey] = persona.getDisplayName();
        personaJson[raceUuidKey] = persona.getRaceUuid();
        if (persona.getPersonaType() == P_CLIENT) {
            personaJson[personaTypeKey] = "client";
        } else if (persona.getPersonaType() == P_REGISTRY) {
            personaJson[personaTypeKey] = "registry";
        } else if (persona.getPersonaType() == P_SERVER) {
            personaJson[personaTypeKey] = "server";
        } else {
            throw std::invalid_argument("Invalid persona type: " +
                                        std::to_string(persona.getPersonaType()));
        }
        personaJson[aesKeyFileKey] = persona.getAesKeyFile();

        configJson.push_back(personaJson);
    }

    std::string configFilePath = configPath + "/race-personas.json";
    return JsonIO::writeJson(sdk, configFilePath, configJson);
}

size_t ConfigPersonas::numPersonas() const {
    return personas.size();
}

Persona ConfigPersonas::getPersona(size_t index) {
    if (index >= personas.size()) {
        throw std::invalid_argument("persona index is out of range");
    }

    return personas[index];
}

void ConfigPersonas::addPersona(const Persona &persona) {
    personas.push_back(persona);
}
