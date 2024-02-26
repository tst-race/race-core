
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

#include "ComponentTypes.h"

#include <sstream>

std::string actionToString(const Action &action) {
    return "Action{ id:" + std::to_string(action.actionId) +
           //    ", timestamp: " + std::to_string(action.timestamp) +
           "}";
}

std::ostream &operator<<(std::ostream &out, const Action &action) {
    return out << actionToString(action);
}

std::string componentManagerStatusToString(ComponentManagerStatus status) {
    switch (status) {
        case CM_OK:
            return "CM_OK";
        case CM_ERROR:
            return "CM_ERROR";
        default:
            return "ERROR: INVALID COMPONENT STATUS" + std::to_string(status);
    }
}

std::ostream &operator<<(std::ostream &out, ComponentManagerStatus status) {
    return out << componentManagerStatusToString(status);
}

std::string channelResponseToString(ChannelResponse channelResponse) {
    return "{ status: " + componentManagerStatusToString(channelResponse.status) +
           ", handle: " + std::to_string(channelResponse.handle) + " }";
}

std::ostream &operator<<(std::ostream &out, ChannelResponse channelResponse) {
    return out << channelResponseToString(channelResponse);
}

std::string componentStatusToString(ComponentStatus componentStatus) {
    switch (componentStatus) {
        case COMPONENT_UNDEF:
            return "COMPONENT_UNDEF";
        case COMPONENT_OK:
            return "COMPONENT_OK";
        case COMPONENT_ERROR:
            return "COMPONENT_ERROR";
        case COMPONENT_FATAL:
            return "COMPONENT_FATAL";
        default:
            return "ERROR: INVALID COMPONENT STATUS" + std::to_string(componentStatus);
    }
}

std::ostream &operator<<(std::ostream &out, ComponentStatus componentStatus) {
    return out << componentStatusToString(componentStatus);
}

std::string componentStateToString(ComponentState componentState) {
    switch (componentState) {
        case COMPONENT_STATE_INIT:
            return "COMPONENT_STATE_INIT";
        case COMPONENT_STATE_STARTED:
            return "COMPONENT_STATE_STARTED";
        case COMPONENT_STATE_FAILED:
            return "COMPONENT_STATE_FAILED";
        default:
            return "ERROR: INVALID COMPONENT STATUS" + std::to_string(componentState);
    }
}

std::ostream &operator<<(std::ostream &out, ComponentState componentState) {
    return out << componentStateToString(componentState);
}

bool operator==(const EncodingParameters &lhs, const EncodingParameters &rhs) {
    return lhs.linkId == rhs.linkId && lhs.type == rhs.type &&
           lhs.encodePackage == rhs.encodePackage && lhs.json == rhs.json;
}

std::string encodingParametersToString(const EncodingParameters &encodingParameters) {
    return "EncodingParameters{ linkId: " + encodingParameters.linkId +
           ", type: " + encodingParameters.type +
           ", encodePackage: " + std::to_string(encodingParameters.encodePackage) +
           ", json: " + encodingParameters.json + "}";
}

std::ostream &operator<<(std::ostream &out, const EncodingParameters &encodingParameters) {
    return out << encodingParametersToString(encodingParameters);
}

std::string eventToString(const Event & /* event */) {
    return "Event{}";
}

std::ostream &operator<<(std::ostream &out, const Event &event) {
    return out << eventToString(event);
}

std::string linkParametersToString(const LinkParameters & /* linkParameters */) {
    return "LinkParameters{}";
}

std::ostream &operator<<(std::ostream &out, const LinkParameters &linkParameters) {
    return out << linkParametersToString(linkParameters);
}

std::string encodingPropertiesToString(const EncodingProperties &encodingProperties) {
    return "EncodingProperties{ encodingTime: " + std::to_string(encodingProperties.encodingTime) +
           ", type: " + encodingProperties.type + "}";
}

std::ostream &operator<<(std::ostream &out, const EncodingProperties &encodingProperties) {
    return out << encodingPropertiesToString(encodingProperties);
}

std::string specificEncodingPropertiesToString(
    const SpecificEncodingProperties &encodingProperties) {
    return "EncodingProperties{ maxBytes: " + std::to_string(encodingProperties.maxBytes) + "}";
}

std::ostream &operator<<(std::ostream &out, const SpecificEncodingProperties &encodingProperties) {
    return out << specificEncodingPropertiesToString(encodingProperties);
}

std::string transportPropertiesToString(const TransportProperties &transportProperties) {
    std::stringstream ss;
    ss << "{";
    for (auto &map_iter : transportProperties.supportedActions) {
        ss << map_iter.first << ":"
           << "{";
        for (auto &vec_iter : map_iter.second) {
            ss << vec_iter << ", ";
        }
        ss << "}, ";
    }
    ss << "}";
    return "TransportProperties{ supportedActions: " + ss.str() + "}";
}

std::ostream &operator<<(std::ostream &out, const TransportProperties &transportProperties) {
    return out << transportPropertiesToString(transportProperties);
}

std::string userModelPropertiesToString(const UserModelProperties &userModelProperties) {
    return "UserModelProperties{ timelineLength: " +
           std::to_string(userModelProperties.timelineLength) +
           ",  timelineFetchPeriod: " + std::to_string(userModelProperties.timelineFetchPeriod) +
           "}";
}

std::ostream &operator<<(std::ostream &out, const UserModelProperties &userModelProperties) {
    return out << userModelPropertiesToString(userModelProperties);
}
