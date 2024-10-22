
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

#ifndef __CONFIG_STATIC_LINKS_H__
#define __CONFIG_STATIC_LINKS_H__

#include <IRaceSdkNM.h>

#include <list>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

#include "LinkProfile.h"

namespace ConfigStaticLinks {
using LinkProfileList = std::list<LinkProfile>;
using ChannelLinkProfilesMap = std::unordered_map<std::string, LinkProfileList>;

ChannelLinkProfilesMap loadLinkProfiles(IRaceSdkNM &sdk, const std::string &configFilePath);
bool writeLinkProfiles(IRaceSdkNM &sdk, const std::string &configFilePath,
                       const ChannelLinkProfilesMap &linkProfiles);
}  // namespace ConfigStaticLinks

#endif
