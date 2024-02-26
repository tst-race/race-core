
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

#include "ComponentManagerTypes.h"

#include <sstream>

namespace CMTypes {

std::string cmInternalStatusToString(CmInternalStatus status) {
    switch (status) {
        case CmInternalStatus::OK:
            return "OK";
        case CmInternalStatus::ERROR:
            return "ERROR";
        case CmInternalStatus::FATAL:
            return "FATAL";
        default:
            return "ERROR: INVALID COMPONENT STATUS: " + std::to_string(status);
    }
}

std::ostream &operator<<(std::ostream &out, CmInternalStatus status) {
    return out << cmInternalStatusToString(status);
}

std::string stateToString(State state) {
    switch (state) {
        case State::INITIALIZING:
            return "INITIALIZING";
        case State::UNACTIVATED:
            return "UNACTIVATED";
        case State::CREATING_COMPONENTS:
            return "CREATING_COMPONENTS";
        case State::WAITING_FOR_COMPONENTS:
            return "WAITING_FOR_COMPONENTS";
        case State::ACTIVATED:
            return "ACTIVATED";
        case State::SHUTTING_DOWN:
            return "SHUTTING_DOWN";
        case State::FAILED:
            return "FAILED";
        default:
            return "ERROR: INVALID STATE: " + std::to_string(state);
    }
}

std::ostream &operator<<(std::ostream &out, State state) {
    return out << stateToString(state);
}

std::string encodingStateToString(EncodingState encodingState) {
    switch (encodingState) {
        case EncodingState::UNENCODED:
            return "UNENCODED";
        case EncodingState::ENCODING:
            return "ENCODING";
        case EncodingState::ENQUEUED:
            return "ENQUEUED";
        case EncodingState::DONE:
            return "DONE";
        default:
            return "ERROR: INVALID ENCODING STATE: " +
                   std::to_string(static_cast<int>(encodingState));
    }
}

std::ostream &operator<<(std::ostream &out, EncodingState encodingState) {
    return out << encodingStateToString(encodingState);
}

std::string encodingModeToString(EncodingMode encodingMode) {
    switch (encodingMode) {
        case EncodingMode::SINGLE:
            return "SINGLE";
        case EncodingMode::BATCH:
            return "BATCH";
        case EncodingMode::FRAGMENT_SINGLE_PRODUCER:
            return "FRAGMENT_SINGLE_PRODUCER";
        case EncodingMode::FRAGMENT_MULTIPLE_PRODUCER:
            return "FRAGMENT_MULTIPLE_PRODUCER";
        default:
            return "ERROR: INVALID ENCODING MODE: " +
                   std::to_string(static_cast<int>(encodingMode));
    }
}

std::ostream &operator<<(std::ostream &out, EncodingMode mode) {
    return out << encodingModeToString(mode);
}

std::ostream &operator<<(std::ostream &out, const EncodingInfo &encodingInfo) {
    return out << "EncodingInfo{ params: " << encodingInfo.params
               << ", pendingEncodeHandle: " << encodingInfo.pendingEncodeHandle
               << ", state: " << encodingInfo.state << " }";
}

std::string packageStateToString(PackageFragmentState packageState) {
    switch (packageState) {
        case PackageFragmentState::UNENCODED:
            return "UNENCODED";
        case PackageFragmentState::ENCODING:
            return "ENCODING";
        case PackageFragmentState::ENQUEUED:
            return "ENQUEUED";
        case PackageFragmentState::DONE:
            return "DONE";
        case PackageFragmentState::SENT:
            return "SENT";
        case PackageFragmentState::FAILED:
            return "FAILED";
        default:
            return "ERROR: INVALID PACKAGE STATE: " +
                   std::to_string(static_cast<int>(packageState));
    }
}

std::ostream &operator<<(std::ostream &out, PackageFragmentState packageState) {
    return out << packageStateToString(packageState);
}

static std::ostream &operator<<(
    std::ostream &out,
    const std::vector<std::unique_ptr<CMTypes::PackageFragmentInfo>> &fragments) {
    out << "[";
    for (auto &fragment : fragments) {
        out << *fragment << ", ";
    }
    out << "]";
    return out;
}

std::ostream &operator<<(std::ostream &out, const PackageInfo &packageInfo) {
    return out << "PackageInfo{ linkId: " << packageInfo.link->linkId
               << ", sdkHandle: " << packageInfo.sdkHandle
               << ", pendingEncodeHandle: " << packageInfo.pendingEncodeHandle
               << ", packageFragments: " << packageInfo.packageFragments << " }";
}

std::string packageFragmentInfoToString(const PackageFragmentInfo &fragment) {
    return "PackageFragmentInfo{package: " + std::to_string(fragment.package->sdkHandle.handle) +
           ", action: " +
           (fragment.action ? std::to_string(fragment.action->action.actionId) : "nullptr") +
           ", state: " + packageStateToString(fragment.state) +
           ", offset: " + std::to_string(fragment.offset) +
           ", len: " + std::to_string(fragment.len) + "}";
}

std::ostream &operator<<(std::ostream &out, const PackageFragmentInfo &packageFragmentInfo) {
    return out << packageFragmentInfoToString(packageFragmentInfo);
}

std::string packageListToString(const std::vector<PackageFragmentInfo *> &packageFragments) {
    std::stringstream ss;
    ss << "[";
    for (auto fragment : packageFragments) {
        ss << *fragment << ", ";
    }
    ss << "]";
    return ss.str();
}

std::string actionInfoToString(const ActionInfo &actionInfo) {
    return "ActionInfo{action: " + actionToString(actionInfo.action) + ", " +
           "linkId: " + actionInfo.linkId + ", " +
           "encoding size: " + std::to_string(actionInfo.encoding.size()) + ", " +
           "fragments: " + packageListToString(actionInfo.fragments) + ", " +
           "toBeRemoved: " + std::to_string(actionInfo.toBeRemoved) + "}";
}

std::ostream &operator<<(std::ostream &out, const ActionInfo &actionInfo) {
    return out << actionInfoToString(actionInfo);
}

static std::ostream &operator<<(
    std::ostream &out, const std::deque<std::unique_ptr<CMTypes::PackageInfo>> &packageQueue) {
    out << "[";
    for (auto &pkg : packageQueue) {
        out << "\n        " << *pkg << ", ";
    }
    if (!packageQueue.empty()) {
        out << "\n    ";
    }
    out << "]";
    return out;
}

static std::ostream &operator<<(std::ostream &out,
                                const std::deque<CMTypes::ActionInfo *> &actionQueue) {
    out << "[";
    for (auto &action : actionQueue) {
        out << "\n        " << *action << ", ";
    }
    if (!actionQueue.empty()) {
        out << "\n    ";
    }
    out << "]";
    return out;
}

std::ostream &operator<<(std::ostream &out, const Link &link) {
    // for ordering
    std::set<ConnectionID> conns{link.connections.begin(), link.connections.end()};
    nlohmann::json connsJson = conns;
    return out << "Link{"
               << "\n    linkId: " << link.linkId << "\n    connections: " << connsJson
               << "\n    actionQueue: " << link.actionQueue
               << "\n    packageQueue: " << link.packageQueue << "\n}";
}

std::ostream &operator<<(std::ostream &out, const Connection &conn) {
    return out << "Connection{ connId " << conn.connId << ", linkId " << conn.linkId << " }";
}

}  // namespace CMTypes