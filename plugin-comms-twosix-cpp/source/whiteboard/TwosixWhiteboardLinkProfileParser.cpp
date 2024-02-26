
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

#include "TwosixWhiteboardLinkProfileParser.h"

#include <nlohmann/json.hpp>
#include <regex>

#include "../PluginCommsTwoSixCpp.h"
#include "../config/LinkConfig.h"
#include "../utils/log.h"
#include "TwosixWhiteboardLink.h"

using json = nlohmann::json;

std::string TwosixWhiteboardLinkProfileParser::fixHashtag(const std::string &orig) {
    // Allow alphanumeric upper and lower case, underscores, and dashes
    std::regex e("([^a-zA-Z0-9_\\-])");
    std::string fixed = std::regex_replace(orig, e, "");
    if (fixed.compare(orig) != 0) {
        logWarning("Warning: the original hashtag \"" + orig +
                   "\" contained invalid characters, fixed to " + fixed);
    }
    return fixed;
}

TwosixWhiteboardLinkProfileParser::TwosixWhiteboardLinkProfileParser(
    const std::string &linkProfile) :
    LinkProfileParser(linkProfile) {
    json linkProfileJson;
    try {
        linkProfileJson = json::parse(linkProfile);
        logDebug("TwosixWhiteboardLinkProfileParser: link profile: " + linkProfile);

        // required
        hostname = linkProfileJson.at("hostname").get<std::string>();
        port = linkProfileJson.at("port").get<int>();

        // fixHashtag helps prevent stupid mistakes like putting / in the tag, which would break
        // accessing the whiteboard. Carried over from pixelfed. Better safe than sorry.
        hashtag = fixHashtag(linkProfileJson.at("hashtag").get<std::string>());

        // optional, but must be of correct type if it exists
        checkFrequency = linkProfileJson.value("checkFrequency", 1000);

        maxTries = linkProfileJson.value("maxTries", 120);
        logDebug("TwosixWhiteboardLinkProfileParser: maxTries: " + std::to_string(maxTries));

        timestamp = linkProfileJson.value("timestamp", -1.0);
    } catch (std::exception &error) {
        logError("TwosixWhiteboardLinkProfileParser: failed to parse link profile: " +
                 std::string(error.what()));
        logError("TwosixWhiteboardLinkProfileParser: invalid link profile: " + linkProfile);
        throw std::invalid_argument("invalid link profile");
    }
}

std::shared_ptr<Link> TwosixWhiteboardLinkProfileParser::createLink(IRaceSdkComms *sdk,
                                                                    PluginCommsTwoSixCpp *plugin,
                                                                    Channel *channel,
                                                                    const LinkConfig &linkConfig,
                                                                    const std::string &channelGid) {
    LinkID linkId = sdk->generateLinkId(channelGid);

    logDebug("Creating Twosix Whiteboard Link: " + linkId);
    return std::make_shared<TwosixWhiteboardLink>(sdk, plugin, channel, linkId,
                                                  linkConfig.linkProps, *this);
}
