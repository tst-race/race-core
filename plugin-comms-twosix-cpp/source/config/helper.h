
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

#ifndef __SOURCE_CONFIG_LOADER_HELPER_H__
#define __SOURCE_CONFIG_LOADER_HELPER_H__

#include <LinkProperties.h>

#include <nlohmann/json.hpp>

#include "LinkConfig.h"

namespace confighelper {

bool isLinkValid(const nlohmann::json &link);
LinkType linkTypeStringToEnum(const std::string &linkType);
LinkConfig parseLink(const nlohmann::json &link, const std::string &activePersona);
LinkPropertySet parseLinkPropertySet(const nlohmann::json &linkJson, std::string set);
LinkPropertyPair parseLinkPropertyPair(const nlohmann::json &linkJson, std::string subprop);

}  // namespace confighelper

#endif
