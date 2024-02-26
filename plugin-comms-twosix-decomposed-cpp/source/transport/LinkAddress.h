
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

#ifndef __COMMS_TWOSIX_TRANSPORT_LINK_PROFILE_H__
#define __COMMS_TWOSIX_TRANSPORT_LINK_PROFILE_H__

#include <nlohmann/json.hpp>
#include <string>

struct LinkAddress {
    // Required
    std::string hashtag;
    // Optional
    std::string hostname{"twosix-whiteboard"};
    int port{5000};
    int maxTries{120};
    double timestamp{-1.0};
};

// Enable automatic conversion to/from json
void to_json(nlohmann::json &destJson, const LinkAddress &srcLinkAddress);
void from_json(const nlohmann::json &srcJson, LinkAddress &destLinkAddress);

#endif  // __COMMS_TWOSIX_TRANSPORT_LINK_PROFILE_H__