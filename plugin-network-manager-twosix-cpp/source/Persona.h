
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

#ifndef __PERSONA_H_
#define __PERSONA_H_

#include <string>
#include <vector>

#define UNSPECIFIED_PERSONA ""

enum PersonaType {
    P_UNDEF = -1,
    P_CLIENT = 0,   // client
    P_SERVER = 1,   // server
    P_REGISTRY = 2  // registry
};

class Persona {
public:
    Persona();
    void setAesKey(const std::vector<uint8_t> &aesKey);
    void setAesKeyFile(const std::string &aesKeyFile);
    void setDisplayName(const std::string &displayName);
    void setRaceUuid(const std::string &raceUuid);
    void setPersonaType(PersonaType personaType);
    std::vector<uint8_t> getAesKey() const;
    std::string getAesKeyFile() const;
    std::string getDisplayName() const;
    std::string getRaceUuid() const;
    PersonaType getPersonaType() const;

    bool operator==(const Persona &other) const;
    bool operator!=(const Persona &other) const;
    bool operator<(const Persona &other) const;

private:
    std::vector<uint8_t> aesKey;
    std::string aesKeyFile;
    std::string displayName;
    std::string raceUuid;
    PersonaType personaType;
};

#endif
