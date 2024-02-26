
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

#ifndef __CONFIG_PERSONAS_H_
#define __CONFIG_PERSONAS_H_

#include <IRaceSdkNM.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "Persona.h"

class ConfigPersonas {
public:
    bool init(IRaceSdkNM &sdk, const std::string &configPath);
    bool write(IRaceSdkNM &sdk, const std::string &configPath);
    size_t numPersonas() const;
    Persona getPersona(size_t index);
    void addPersona(const Persona &persona);

private:
    std::vector<Persona> personas;
};

#endif
