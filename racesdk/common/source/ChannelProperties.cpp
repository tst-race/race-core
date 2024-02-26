
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

#include "ChannelProperties.h"

#include <sstream>
#include <stdexcept>

static std::string stringVectorToString(const std::vector<std::string> &vec) {
    std::string s = "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) {
            s += ", ";
        }
        s += vec[i];
    }

    s += "]";
    return s;
}

ChannelProperties::ChannelProperties() :
    channelStatus(CHANNEL_UNDEF),
    linkDirection(LD_UNDEF),
    transmissionType(TT_UNDEF),
    connectionType(CT_UNDEF),
    sendType(ST_UNDEF),
    multiAddressable(false),
    reliable(false),
    bootstrap(false),
    isFlushable(false),
    duration_s(-1),
    period_s(-1),
    mtu(-1),
    maxLinks(-1),
    creatorsPerLoader(-1),
    loadersPerCreator(-1),
    maxSendsPerInterval(-1),
    secondsPerInterval(-1),
    intervalEndTime(0),
    sendsRemainingInInterval(-1) {}

std::string channelPropertiesToString(const ChannelProperties &props) {
    std::stringstream ss;

    ss << "ChannelProperties {";
    ss << "channelGid: " << props.channelGid << ", ";
    ss << "channelStatus: " << channelStatusToString(props.channelStatus) << ", ";
    ss << "linkDirection: " << linkDirectionToString(props.linkDirection) << ", ";
    ss << "transmissionType: " << transmissionTypeToString(props.transmissionType) << ", ";
    ss << "connectionType: " << connectionTypeToString(props.connectionType) << ", ";
    ss << "sendType: " << sendTypeToString(props.sendType) << ", ";
    ss << "multiAddressable: " << props.multiAddressable << ", ";
    ss << "reliable: " << props.reliable << ", ";
    ss << "bootstrap: " << props.bootstrap << ", ";
    ss << "isFlushable: " << props.isFlushable << ", ";
    ss << "duration_s: " << props.duration_s << ", ";
    ss << "period_s: " << props.period_s << ", ";
    ss << "mtu: " << props.mtu << ", ";
    ss << "creatorExpected: " << linkPropertyPairToString(props.creatorExpected) << ", ";
    ss << "loaderExpected: " << linkPropertyPairToString(props.loaderExpected) << ", ";
    ss << "supported_hints: " << stringVectorToString(props.supported_hints) << ", ";
    ss << "maxLinks: " << props.maxLinks << ", ";
    ss << "creatorsPerLoader: " << props.creatorsPerLoader << ", ";
    ss << "loadersPerCreator: " << props.loadersPerCreator << ", ";
    ss << "roles: [";
    for (const ChannelRole &role : props.roles) {
        ss << channelRoleToString(role) << ", ";
    }
    ss << "]}, ";
    ss << "currentRole: " << channelRoleToString(props.currentRole) << ", ";
    ss << "maxSendsPerInterval: " << props.maxSendsPerInterval << ", ";
    ss << "secondsPerInterval: " << props.secondsPerInterval << ", ";
    ss << "intervalEndTime: " << props.intervalEndTime << ", ";
    ss << "sendsRemainingInInterval: " << props.sendsRemainingInInterval << "} ";
    return ss.str();
}

std::string linkDirectionToString(LinkDirection linkDirection) {
    switch (linkDirection) {
        case LD_UNDEF:
            return "LD_UNDEF";
        case LD_CREATOR_TO_LOADER:
            return "LD_CREATOR_TO_LOADER";
        case LD_LOADER_TO_CREATOR:
            return "LD_LOADER_TO_CREATOR";
        case LD_BIDI:
            return "LD_BIDI";
        default:
            return "ERROR: INVALID LINK DIRECTION: " + std::to_string(linkDirection);
    }
}

std::ostream &operator<<(std::ostream &out, LinkDirection linkDirection) {
    return out << linkDirectionToString(linkDirection);
}

LinkDirection linkDirectionFromString(const std::string &linkDirectionString) {
    if (linkDirectionString == "LD_UNDEF") {
        return LD_UNDEF;
    } else if (linkDirectionString == "LD_CREATOR_TO_LOADER") {
        return LD_CREATOR_TO_LOADER;
    } else if (linkDirectionString == "LD_LOADER_TO_CREATOR") {
        return LD_LOADER_TO_CREATOR;
    } else if (linkDirectionString == "LD_BIDI") {
        return LD_BIDI;
    } else {
        throw std::invalid_argument("Invalid argument to linkDirectionFromString: " +
                                    linkDirectionString);
    }
}

bool channelStaticPropertiesEqual(const ChannelProperties &a, const ChannelProperties &b) {
    return (a.channelGid == b.channelGid && a.linkDirection == b.linkDirection &&
            a.transmissionType == b.transmissionType && a.connectionType == b.connectionType &&
            a.sendType == b.sendType && a.multiAddressable == b.multiAddressable &&
            a.reliable == b.reliable && a.bootstrap == b.bootstrap &&
            a.isFlushable == b.isFlushable && a.duration_s == b.duration_s &&
            a.period_s == b.period_s && a.supported_hints == b.supported_hints && a.mtu == b.mtu &&
            a.creatorExpected == b.creatorExpected && a.loaderExpected == b.loaderExpected &&
            a.maxLinks == b.maxLinks && a.creatorsPerLoader == b.creatorsPerLoader &&
            a.loadersPerCreator == b.loadersPerCreator && a.roles == b.roles);
}