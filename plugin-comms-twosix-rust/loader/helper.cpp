
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

void helper::convertLinkPropertySetCToClass(const LinkPropertySetC &input,
                                            LinkPropertySet &output) {
    output.bandwidth_bps = input.bandwidth_bps;
    output.latency_ms = input.latency_ms;
    output.loss = input.loss;
}

void helper::convertLinkPropertyPairCToClass(const LinkPropertyPairC &input,
                                             LinkPropertyPair &output) {
    convertLinkPropertySetCToClass(input.send, output.send);
    convertLinkPropertySetCToClass(input.receive, output.receive);
}

void helper::convertLinkPropertiesCToClass(const LinkPropertiesC &input, LinkProperties &output) {
    output.linkType = input.linkType;
    output.transmissionType = input.transmissionType;
    output.connectionType = input.connectionType;
    output.sendType = input.sendType;
    output.reliable = input.reliable;
    output.isFlushable = input.isFlushable;
    output.duration_s = input.duration_s;
    output.period_s = input.period_s;
    output.mtu = input.mtu;
    convertLinkPropertyPairCToClass(input.worst, output.worst);
    convertLinkPropertyPairCToClass(input.best, output.best);
    convertLinkPropertyPairCToClass(input.expected, output.expected);
    if (input.supported_hints != nullptr) {
        output.supported_hints = *static_cast<std::vector<std::string> *>(input.supported_hints);
    }
    if (input.channelGid != nullptr) {
        output.channelGid = *static_cast<std::string *>(input.channelGid);
    }
    if (input.linkAddress != nullptr) {
        output.linkAddress = *static_cast<std::string *>(input.linkAddress);
    }
}

void helper::convertChannelPropertiesCToClass(const ChannelPropertiesC &input,
                                              ChannelProperties &output) {
    output.channelStatus = input.channelStatus;
    output.linkDirection = input.linkDirection;
    output.transmissionType = input.transmissionType;
    output.connectionType = input.connectionType;
    output.sendType = input.sendType;
    output.multiAddressable = input.multiAddressable;
    output.reliable = input.reliable;
    output.bootstrap = input.bootstrap;
    output.isFlushable = input.isFlushable;
    output.duration_s = input.duration_s;
    output.period_s = input.period_s;
    output.mtu = input.mtu;
    output.maxLinks = input.maxLinks;
    output.creatorsPerLoader = input.creatorsPerLoader;
    output.loadersPerCreator = input.loadersPerCreator;
    output.maxSendsPerInterval = input.maxSendsPerInterval;
    output.secondsPerInterval = input.secondsPerInterval;
    output.intervalEndTime = input.intervalEndTime;
    output.sendsRemainingInInterval = input.sendsRemainingInInterval;
    convertChannelRoleCToClass(input.currentRole, output.currentRole);
    convertLinkPropertyPairCToClass(input.creatorExpected, output.creatorExpected);
    convertLinkPropertyPairCToClass(input.loaderExpected, output.loaderExpected);
    if (input.supported_hints != nullptr) {
        output.supported_hints = *static_cast<std::vector<std::string> *>(input.supported_hints);
    }
    if (input.channelGid != nullptr) {
        output.channelGid = *static_cast<std::string *>(input.channelGid);
    }

    output.roles = std::vector<ChannelRole>(input.rolesLen);
    for (size_t i = 0; i < input.rolesLen; i++) {
        convertChannelRoleCToClass(input.roles[i], output.roles[i]);
    }
}

void helper::convertLinkPropertySetToLinkPropertySetC(const LinkPropertySet &input,
                                                      LinkPropertySetC &output) {
    output.bandwidth_bps = input.bandwidth_bps;
    output.latency_ms = input.latency_ms;
    output.loss = input.loss;
}

void helper::convertLinkPropertyPairToLinkPropertyPairC(const LinkPropertyPair &input,
                                                        LinkPropertyPairC &output) {
    convertLinkPropertySetToLinkPropertySetC(input.send, output.send);
    convertLinkPropertySetToLinkPropertySetC(input.receive, output.receive);
}

void helper::convertChannelPropertiesToChannelPropertiesC(const ChannelProperties &input,
                                                          ChannelPropertiesC &output) {
    output.channelStatus = input.channelStatus;
    output.linkDirection = input.linkDirection;
    output.transmissionType = input.transmissionType;
    output.connectionType = input.connectionType;
    output.sendType = input.sendType;
    output.multiAddressable = input.multiAddressable;
    output.reliable = input.reliable;
    output.bootstrap = input.bootstrap;
    output.isFlushable = input.isFlushable;
    output.duration_s = input.duration_s;
    output.period_s = input.period_s;
    output.mtu = input.mtu;
    output.maxLinks = input.maxLinks;
    output.creatorsPerLoader = input.creatorsPerLoader;
    output.loadersPerCreator = input.loadersPerCreator;
    output.maxSendsPerInterval = input.maxSendsPerInterval;
    output.secondsPerInterval = input.secondsPerInterval;
    output.intervalEndTime = input.intervalEndTime;
    output.sendsRemainingInInterval = input.sendsRemainingInInterval;
    convertChannelRoleToChannelRoleC(input.currentRole, output.currentRole);
    convertLinkPropertyPairToLinkPropertyPairC(input.creatorExpected, output.creatorExpected);
    convertLinkPropertyPairToLinkPropertyPairC(input.loaderExpected, output.loaderExpected);

    output.supported_hints = new std::vector<std::string>(input.supported_hints);
    output.channelGid = new std::string(input.channelGid);

    output.rolesLen = input.roles.size();
    output.roles = new ChannelRoleC[output.rolesLen];
    for (size_t i = 0; i < output.rolesLen; i++) {
        convertChannelRoleToChannelRoleC(input.roles[i], output.roles[i]);
    }
}

void helper::convertChannelRoleCToClass(const ChannelRoleC &input, ChannelRole &output) {
    output.roleName = input.roleName;
    output.mechanicalTags = {};
    for (size_t i = 0; i < input.mechanicalTagsLen; i++) {
        output.mechanicalTags.push_back(input.mechanicalTags[i]);
    }
    output.behavioralTags = {};
    for (size_t i = 0; i < input.behavioralTagsLen; i++) {
        output.behavioralTags.push_back(input.behavioralTags[i]);
    }
    output.linkSide = input.linkSide;
}

void helper::convertChannelRoleToChannelRoleC(const ChannelRole &input, ChannelRoleC &output) {
    output.roleName = strdup(input.roleName.c_str());

    output.mechanicalTagsLen = input.mechanicalTags.size();
    output.mechanicalTags = new char *[output.mechanicalTagsLen];
    for (size_t i = 0; i < output.mechanicalTagsLen; i++) {
        output.mechanicalTags[i] = strdup(input.mechanicalTags[i].c_str());
    }

    output.behavioralTagsLen = input.behavioralTags.size();
    output.behavioralTags = new char *[output.behavioralTagsLen];
    for (size_t i = 0; i < output.behavioralTagsLen; i++) {
        output.behavioralTags[i] = strdup(input.behavioralTags[i].c_str());
    }

    output.linkSide = input.linkSide;
}
