
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

#include "helper.h"

#include <stdexcept>

#include "../utils/log.h"

LinkType confighelper::linkTypeStringToEnum(const std::string &linkType) {
    if (linkType == "send") {
        return LT_SEND;
    } else if (linkType == "receive") {
        return LT_RECV;
    } else if (linkType == "bidirectional") {
        return LT_BIDI;
    }

    throw std::invalid_argument("Invalid link type: " + linkType);
}

LinkPropertySet confighelper::parseLinkPropertySet(const nlohmann::json &proplist,
                                                   std::string propname) {
    LinkPropertySet propSet;
    if (proplist.find(propname) != proplist.end()) {
        propSet.bandwidth_bps = proplist[propname].value("bandwidth_bps", -1);
        propSet.latency_ms = proplist[propname].value("latency_ms", -1);
        propSet.loss = proplist[propname].value("loss", -1.);
    }

    return propSet;
}

LinkPropertyPair confighelper::parseLinkPropertyPair(const nlohmann::json &proplist,
                                                     std::string propname) {
    LinkPropertyPair propPair;
    if (proplist.find(propname) != proplist.end()) {
        propPair.send = parseLinkPropertySet(proplist[propname], "send");
    }
    if (proplist.find(propname) != proplist.end()) {
        propPair.receive = parseLinkPropertySet(proplist[propname], "receive");
    }

    return propPair;
}

LinkConfig confighelper::parseLink(const nlohmann::json &link, const std::string &activePersona) {
    LinkConfig currentLink;
    try {
        const auto &utilizedBy = link.value("utilizedBy", std::vector<std::string>());
        if (std::find(utilizedBy.begin(), utilizedBy.end(), activePersona) == utilizedBy.end()) {
            throw std::invalid_argument("link profile not intended for this persona");
        }

        currentLink.linkProfile = link.at("profile").get<nlohmann::json::string_t>();

        // Set all the personas that this link can connect to.
        for (const std::string &persona : link.value("connectedTo", std::vector<std::string>())) {
            currentLink.personas.emplace_back(persona);
        }

        nlohmann::json properties_json = link.at("properties");
        // Set the properties link type.
        currentLink.linkProps.linkType = linkTypeStringToEnum(properties_json.value("type", ""));

        // Set the transmissionType.
        if (properties_json.find("multicast") != properties_json.end() &&
            properties_json.value("multicast", false)) {
            currentLink.linkProps.transmissionType = TT_MULTICAST;
            currentLink.linkProps.connectionType =
                CT_INDIRECT;  // our multicast channel is indirect
            currentLink.linkProps.sendType = ST_STORED_ASYNC;
        } else {
            currentLink.linkProps.transmissionType = TT_UNICAST;
            currentLink.linkProps.connectionType = CT_DIRECT;  // our unicast channel is direct
            currentLink.linkProps.sendType = ST_EPHEM_SYNC;
        }

        if (currentLink.personas.size() == 0) {
            logError("Found a link with no personas, ignoring");
            throw std::invalid_argument("link with no personas");
        }

        currentLink.linkProps.reliable = properties_json.value("reliable", false);
        currentLink.linkProps.duration_s = properties_json.value("duration_s", -1);
        currentLink.linkProps.period_s = properties_json.value("period_s", -1);
        currentLink.linkProps.mtu = properties_json.value("mtu", -1);

        currentLink.linkProps.worst = parseLinkPropertyPair(properties_json, "worst");
        currentLink.linkProps.expected = parseLinkPropertyPair(properties_json, "expected");
        currentLink.linkProps.best = parseLinkPropertyPair(properties_json, "best");

        currentLink.linkProps.supported_hints =
            properties_json.value("supported_hints", std::vector<std::string>());
    } catch (nlohmann::json::exception &e) {
        logWarning("JSON parsing exception when parsing link profile " + std::string(e.what()));
        throw std::invalid_argument("invalid link profile (JSON parse error) " +
                                    std::string(e.what()));
    }

    return currentLink;
}
