
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

#include "IndirectBootstrapChannel.h"

#include <climits>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "../whiteboard/TwosixWhiteboardLinkProfileParser.h"
#include "IndirectBootstrapLink.h"

const std::string IndirectBootstrapChannel::indirectBootstrapChannelGid =
    "twoSixIndirectBootstrapCpp";

IndirectBootstrapChannel::IndirectBootstrapChannel(PluginCommsTwoSixCpp &plugin) :
    IndirectChannel(plugin, indirectBootstrapChannelGid) {}

std::shared_ptr<Link> IndirectBootstrapChannel::createLink(const LinkID &linkId) {
    return createBootstrapLink(linkId, "");
}

std::shared_ptr<Link> IndirectBootstrapChannel::createBootstrapLink(const LinkID &linkId,
                                                                    const std::string &passphrase) {
    LinkProperties linkProps = linkProperties;

    // Note that bootstrap channels are not expected to be bi-directional
    // in practice. The implementation of IndirectBootstrapChannel mirrors
    // that of IndirectChannel as a simplification, and exists mainly to enable
    // bootstrap operations on devices where BootstrapChannel is not supported.
    linkProps.linkType = LT_BIDI;

    TwosixWhiteboardLinkProfileParser parser;
    parser.hostname = whiteboardHostname;
    parser.port = whiteboardPort;
    parser.hashtag = "_cpp_" + plugin.racePersona + "_" + passphrase;
    parser.checkFrequency = 1000;
    std::chrono::duration<double> sinceEpoch =
        std::chrono::high_resolution_clock::now().time_since_epoch();
    parser.timestamp = sinceEpoch.count();
    parser.maxTries = 120;

    const auto link = std::make_shared<IndirectBootstrapLink>(plugin.raceSdk, &plugin, this, linkId,
                                                              linkProps, parser, passphrase);

    logDebug("IndirectBootstrapChannel::createBootstrapLink: created link");

    return link;
}

std::shared_ptr<Link> IndirectBootstrapChannel::createLinkFromAddress(
    const LinkID &linkId, const std::string &linkAddress) {
    return loadLink(linkId, linkAddress);
}

std::shared_ptr<Link> IndirectBootstrapChannel::loadLink(const LinkID &linkId,
                                                         const std::string &linkAddress) {
    // copy default link properties and set link type
    LinkProperties linkProps = linkProperties;
    linkProps.linkType = LT_BIDI;

    TwosixWhiteboardLinkProfileParser parser(linkAddress);

    const auto link = std::make_shared<IndirectBootstrapLink>(plugin.raceSdk, &plugin, this, linkId,
                                                              linkProps, parser, "");

    return link;
}
