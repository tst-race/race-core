
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

#include "Persona.h"

Persona::Persona() : personaType(P_UNDEF) {}

void Persona::setAesKey(const std::vector<uint8_t> &_aesKey) {
    aesKey = _aesKey;
}

void Persona::setAesKeyFile(const std::string &_aesKeyFile) {
    aesKeyFile = _aesKeyFile;
}

void Persona::setDisplayName(const std::string &_displayName) {
    displayName = _displayName;
}

void Persona::setRaceUuid(const std::string &_raceUuid) {
    raceUuid = _raceUuid;
}

void Persona::setPersonaType(PersonaType _personaType) {
    personaType = _personaType;
}

std::vector<uint8_t> Persona::getAesKey() const {
    return aesKey;
}

std::string Persona::getAesKeyFile() const {
    return aesKeyFile;
}

std::string Persona::getDisplayName() const {
    return displayName;
}

std::string Persona::getRaceUuid() const {
    return raceUuid;
}

PersonaType Persona::getPersonaType() const {
    return personaType;
}

bool Persona::operator==(const Persona &other) const {
    return (raceUuid == other.raceUuid);
}

bool Persona::operator!=(const Persona &other) const {
    return !(*this == other);
}

bool Persona::operator<(const Persona &other) const {
    return (raceUuid < other.raceUuid);
}
