
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

#ifndef __I_COMPONENT_TYPES_H__
#define __I_COMPONENT_TYPES_H__

#include <string>
#include <unordered_map>
#include <vector>

#include "SdkResponse.h"  // RaceHandle

using LinkID = std::string;

using JsonString = std::string;

// A timestamp is seconds since unix epoch
using Timestamp = double;

struct Action {
    Timestamp timestamp;
    uint64_t actionId;
    JsonString json;
};

std::string actionToString(const Action &action);
std::ostream &operator<<(std::ostream &out, const Action &action);

// TODO: what is this used for?
struct ActionStatus {};

using ActionTimeline = std::vector<Action>;

enum ComponentManagerStatus {
    CM_OK,
    CM_ERROR,
};

std::string componentManagerStatusToString(ComponentManagerStatus status);
std::ostream &operator<<(std::ostream &out, ComponentManagerStatus status);

struct ChannelResponse {
    ComponentManagerStatus status;
    RaceHandle handle;
};

std::string channelResponseToString(ChannelResponse channelResponse);
std::ostream &operator<<(std::ostream &out, ChannelResponse channelResponse);

enum ComponentStatus {
    COMPONENT_UNDEF,
    COMPONENT_OK,
    COMPONENT_ERROR,
    COMPONENT_FATAL,
};

std::string componentStatusToString(ComponentStatus componentStatus);
std::ostream &operator<<(std::ostream &out, ComponentStatus componentStatus);

enum ComponentState {
    COMPONENT_STATE_INIT,
    COMPONENT_STATE_STARTED,
    COMPONENT_STATE_FAILED,
};

std::string componentStateToString(ComponentState componentState);
std::ostream &operator<<(std::ostream &out, ComponentState componentState);

using EncodingType = std::string;

struct EncodingParameters {
    LinkID linkId;
    EncodingType type;
    bool encodePackage;
    JsonString json;
};

std::string encodingParametersToString(const EncodingParameters &encodingParameters);
std::ostream &operator<<(std::ostream &out, const EncodingParameters &encodingParameters);
bool operator==(const EncodingParameters &lhs, const EncodingParameters &rhs);

struct Event {
    JsonString json;
};

std::string eventToString(const Event &event);
std::ostream &operator<<(std::ostream &out, const Event &event);

struct LinkParameters {
    JsonString json;
};

std::string linkParametersToString(const LinkParameters &linkParameters);
std::ostream &operator<<(std::ostream &out, const LinkParameters &linkParameters);

struct EncodingProperties {
    double encodingTime;
    EncodingType type;
};

std::string encodingPropertiesToString(const EncodingProperties &encodingProperties);
std::ostream &operator<<(std::ostream &out, const EncodingProperties &encodingProperties);

struct SpecificEncodingProperties {
    int32_t maxBytes;
};

std::string specificEncodingPropertiesToString(
    const SpecificEncodingProperties &encodingParameters);
std::ostream &operator<<(std::ostream &out, const SpecificEncodingProperties &encodingParameters);

struct TransportProperties {
    std::unordered_map<std::string, std::vector<EncodingType>> supportedActions;
};

std::string transportPropertiesToString(const TransportProperties &transportProperties);
std::ostream &operator<<(std::ostream &out, const TransportProperties &transportProperties);

struct UserModelProperties {
    double timelineLength = 600;
    double timelineFetchPeriod = 300;
};

std::string userModelPropertiesToString(const UserModelProperties &userModelProperties);
std::ostream &operator<<(std::ostream &out, const UserModelProperties &userModelProperties);

#endif
