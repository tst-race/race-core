
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

#ifndef _DIRECT_LINK_PROFILE_PARSER_H_
#define _DIRECT_LINK_PROFILE_PARSER_H_

#include <string>

#include "../base/LinkProfileParser.h"

class DirectLinkProfileParser : public LinkProfileParser {
public:
    // public for testing
    std::string hostname;
    int port;

public:
    // default constructor for testing
    DirectLinkProfileParser() = default;

    explicit DirectLinkProfileParser(const std::string &linkProfile);
    ~DirectLinkProfileParser() = default;
    virtual std::shared_ptr<Link> createLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin,
                                             Channel *channel, const LinkConfig &linkConfig,
                                             const std::string &channelGid) override;
};

#endif
