
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

#include "LinkProfile.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

void to_json(nlohmann::json &destJson, const LinkProfile &srcProfile) {
    destJson = json{
        {"description", srcProfile.description},
        {"personas", srcProfile.personas},
        {"role", srcProfile.role},
    };

    if (not srcProfile.address_list.empty()) {
        destJson["address_list"] = srcProfile.address_list;
    } else {
        destJson["address"] = srcProfile.address;
    }
}

void from_json(const nlohmann::json &srcJson, LinkProfile &destProfile) {
    // address & address_list are optional
    destProfile.address = srcJson.value("address", destProfile.address);
    destProfile.address_list = srcJson.value("address_list", destProfile.address_list);
    // rest are required
    srcJson.at("description").get_to(destProfile.description);
    srcJson.at("personas").get_to(destProfile.personas);
    srcJson.at("role").get_to(destProfile.role);
}
