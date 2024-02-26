
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

#include "BootstrapFileLinkProfileParser.h"

#include <nlohmann/json.hpp>

#include "../PluginCommsTwoSixCpp.h"
#include "../config/LinkConfig.h"
#include "../utils/log.h"
#include "BootstrapFileLink.h"

using json = nlohmann::json;

BootstrapFileLinkProfileParser::BootstrapFileLinkProfileParser(const std::string &linkProfile) :
    LinkProfileParser(linkProfile) {}

std::shared_ptr<Link> BootstrapFileLinkProfileParser::createLink(IRaceSdkComms *sdk,
                                                                 PluginCommsTwoSixCpp *plugin,
                                                                 Channel *channel,
                                                                 const LinkConfig &linkConfig,
                                                                 const std::string &channelGid) {
    LinkID linkId = sdk->generateLinkId(channelGid);

    logDebug("Creating BootstrapFile Link: " + linkId);
    return std::make_shared<BootstrapFileLink>(sdk, plugin, channel, linkId, linkConfig.linkProps,
                                               *this);
}
