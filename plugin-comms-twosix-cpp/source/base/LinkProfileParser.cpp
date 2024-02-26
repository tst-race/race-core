
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

#include "LinkProfileParser.h"

#include <iostream>
#include <nlohmann/json.hpp>

#include "../PluginCommsTwoSixCpp.h"
#include "../config/LinkConfig.h"
#include "../direct/DirectLinkProfileParser.h"
#include "../utils/log.h"
#include "../whiteboard/TwosixWhiteboardLinkProfileParser.h"
#include "Link.h"

using json = nlohmann::json;

const std::string twosixWhiteboardServiceName = "twosix-whiteboard";

LinkProfileParser::LinkProfileParser(const std::string &linkProfile) {
    json linkProfileJson;
    try {
        linkProfileJson = json::parse(linkProfile);

        // optional
        send_period_length = linkProfileJson.value("send_period_length", 0.0);
        send_period_amount = linkProfileJson.value("send_period_amount", 0ul);
        sleep_period_length = linkProfileJson.value("sleep_period_length", 0.0);
        send_drop_rate = linkProfileJson.value("send_drop_rate", 0.0);
        receive_drop_rate = linkProfileJson.value("receive_drop_rate", 0.0);
        send_corrupt_rate = linkProfileJson.value("send_corrupt_rate", 0.0);
        receive_corrupt_rate = linkProfileJson.value("receive_corrupt_rate", 0.0);
        send_corrupt_amount = linkProfileJson.value("send_corrupt_amount", 0ul);
        receive_corrupt_amount = linkProfileJson.value("receive_corrupt_amount", 0ul);
        trace_corrupt_size_limit = linkProfileJson.value("trace_corrupt_size_limit", 0ul);
    } catch (std::exception &error) {
        logError("LinkProfileParser: failed to parse link profile: " + std::string(error.what()));
        logError("LinkProfileParser: invalid link profile: " + linkProfile);
        throw std::invalid_argument("invalid link profile");
    }
}

std::unique_ptr<LinkProfileParser> LinkProfileParser::parse(const std::string &linkProfile) {
    json linkProfileJson;
    try {
        linkProfileJson = json::parse(linkProfile);

        // optional, but must be of correct type if they exist
        bool multicast = linkProfileJson.value("multicast", false);
        std::string serviceName = linkProfileJson.value("service_name", "");

        if (!multicast) {
            return std::make_unique<DirectLinkProfileParser>(linkProfile);
        } else if (serviceName == twosixWhiteboardServiceName) {
            return std::make_unique<TwosixWhiteboardLinkProfileParser>(linkProfile);
        } else {
            logError("LinkProfileParser: unknown service name: " + serviceName);
            return nullptr;
        }
    } catch (json::exception &error) {
        logError("LinkProfileParser: failed to parse link profile: " + std::string(error.what()));
        logError("LinkProfileParser: invalid link profile: " + linkProfile);
    } catch (std::exception &error) {
        logError("LinkProfileParser: invalid link profile: " + linkProfile);
    }

    return nullptr;
}

// dummy implementation to allow construction of Link for testing
std::shared_ptr<Link> LinkProfileParser::createLink(IRaceSdkComms *, PluginCommsTwoSixCpp *,
                                                    Channel *, const LinkConfig &,
                                                    const std::string &) {
    throw std::logic_error("Not Implemented");
}
