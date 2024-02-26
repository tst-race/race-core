
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

#include "DirectLinkProfileParser.h"

#include <nlohmann/json.hpp>

#include "../PluginCommsTwoSixCpp.h"
#include "../config/LinkConfig.h"
#include "../utils/log.h"
#include "DirectLink.h"

using json = nlohmann::json;

DirectLinkProfileParser::DirectLinkProfileParser(const std::string &linkProfile) :
    LinkProfileParser(linkProfile) {
    json linkProfileJson;
    try {
        linkProfileJson = json::parse(linkProfile);

        // required
        hostname = linkProfileJson.at("hostname").get<std::string>();
        port = linkProfileJson.at("port").get<int>();
    } catch (std::exception &error) {
        logError("DirectLinkProfileParser: failed to parse link profile: " +
                 std::string(error.what()));
        logError("DirectLinkProfileParser: invalid link profile: " + linkProfile);
        throw std::invalid_argument("invalid link profile");
    }
}

std::shared_ptr<Link> DirectLinkProfileParser::createLink(IRaceSdkComms *sdk,
                                                          PluginCommsTwoSixCpp *plugin,
                                                          Channel *channel,
                                                          const LinkConfig &linkConfig,
                                                          const std::string &channelGid) {
    LinkID linkId = sdk->generateLinkId(channelGid);

    logDebug("Creating Direct Link: " + linkId);
    return std::make_shared<DirectLink>(sdk, plugin, channel, linkId, linkConfig.linkProps, *this);
}
