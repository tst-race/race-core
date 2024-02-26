
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

#include "ChannelRole.h"

#include <sstream>
#include <stdexcept>

std::string linkSideToString(LinkSide linkSide) {
    switch (linkSide) {
        case LS_UNDEF:
            return "LS_UNDEF";
        case LS_CREATOR:
            return "LS_CREATOR";
        case LS_LOADER:
            return "LS_LOADER";
        case LS_BOTH:
            return "LS_BOTH";
        default:
            return "ERROR: Invalid Channel Role: " + std::to_string(linkSide);
    }
}

LinkSide linkSideFromString(const std::string &linkSideString) {
    if (linkSideString == "LS_UNDEF") {
        return LS_UNDEF;
    } else if (linkSideString == "LS_CREATOR") {
        return LS_CREATOR;
    } else if (linkSideString == "LS_LOADER") {
        return LS_LOADER;
    } else if (linkSideString == "LS_BOTH") {
        return LS_BOTH;
    } else {
        throw std::invalid_argument("Invalid argument to linkSideFromString: " + linkSideString);
    }
}

std::ostream &operator<<(std::ostream &out, LinkSide linkSide) {
    return out << linkSideToString(linkSide);
}

std::string channelRoleToString(const ChannelRole &channelRole) {
    std::stringstream ss;
    ss << "{";
    ss << "roleName: " << channelRole.roleName << ", ";

    ss << "mechanicalTags: [";
    for (const std::string &tag : channelRole.mechanicalTags) {
        ss << tag << ", ";
    }
    ss << "], ";
    ss << "behavioralTags: [";
    for (const std::string &tag : channelRole.behavioralTags) {
        ss << tag << ", ";
    }
    ss << "], ";
    ss << "linkSide: " << linkSideToString(channelRole.linkSide) << "}";

    return ss.str();
}