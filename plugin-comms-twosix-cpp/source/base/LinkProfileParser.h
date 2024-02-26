
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

#ifndef _LINK_PROFILE_PARSER_H_
#define _LINK_PROFILE_PARSER_H_

#include <IRaceSdkComms.h>

#include <memory>
#include <string>

class Link;
class PluginCommsTwoSixCpp;
class LinkConfig;
class Channel;

class LinkProfileParser {
public:
    // public for testing
    double send_period_length;
    uint32_t send_period_amount;
    double sleep_period_length;
    double send_drop_rate;
    double receive_drop_rate;
    double send_corrupt_rate;
    double receive_corrupt_rate;
    uint32_t send_corrupt_amount;
    uint32_t receive_corrupt_amount;
    uint32_t trace_corrupt_size_limit;

public:
    LinkProfileParser() :
        send_period_length(0),
        send_period_amount(0),
        sleep_period_length(0),
        send_drop_rate(0),
        receive_drop_rate(0),
        send_corrupt_rate(0),
        receive_corrupt_rate(0),
        send_corrupt_amount(0),
        receive_corrupt_amount(0),
        trace_corrupt_size_limit(0) {}

    explicit LinkProfileParser(const std::string &linkProfile);

    virtual ~LinkProfileParser() = default;
    static std::unique_ptr<LinkProfileParser> parse(const std::string &linkProfile);
    virtual std::shared_ptr<Link> createLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin,
                                             Channel *channel, const LinkConfig &linkConfig,
                                             const std::string &channelGid);
};

#endif
