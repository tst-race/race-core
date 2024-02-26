
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

#include "LinkAddress.h"

void to_json(nlohmann::json &destJson, const LinkAddress &srcLinkAddress) {
    destJson = nlohmann::json{
        // clang-format off
        {"hashtag", srcLinkAddress.hashtag},
        {"hostname", srcLinkAddress.hostname},
        {"port", srcLinkAddress.port},
        {"maxTries", srcLinkAddress.maxTries},
        {"timestamp", srcLinkAddress.timestamp},
        // clang-format on
    };
}

void from_json(const nlohmann::json &srcJson, LinkAddress &destLinkAddress) {
    // Required
    srcJson.at("hashtag").get_to(destLinkAddress.hashtag);
    // Optional
    destLinkAddress.hostname = srcJson.value("hostname", destLinkAddress.hostname);
    destLinkAddress.port = srcJson.value("port", destLinkAddress.port);
    destLinkAddress.maxTries = srcJson.value("maxTries", destLinkAddress.maxTries);
    destLinkAddress.timestamp = srcJson.value("timestamp", destLinkAddress.timestamp);
}