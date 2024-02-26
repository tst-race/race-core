
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

#pragma once

#include <deque>
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "ComponentTypes.h"
#include "EncPkg.h"
#include "LinkProperties.h"  // ConnectionID, LinkID

namespace CMTypes {
enum CmInternalStatus { OK, ERROR, FATAL };

std::string cmInternalStatusToString(CmInternalStatus status);
std::ostream &operator<<(std::ostream &out, CmInternalStatus status);

// powers of 2 so they can by or'd together for checking if current state matches
enum State {
    INITIALIZING = 1 << 0,
    UNACTIVATED = 1 << 1,
    CREATING_COMPONENTS = 1 << 2,
    WAITING_FOR_COMPONENTS = 1 << 3,
    ACTIVATED = 1 << 4,
    SHUTTING_DOWN = 1 << 5,
    FAILED = 1 << 6,
};

std::string stateToString(State state);
std::ostream &operator<<(std::ostream &out, State state);

template <int>
struct HandleStruct {
    RaceHandle handle;

    bool operator==(const HandleStruct &other) const {
        return handle == other.handle;
    }

    HandleStruct &operator++() {
        ++handle;
        return *this;
    }

    std::string to_string() const {
        return std::to_string(handle);
    }
};

template <int N>
inline bool operator==(const HandleStruct<N> &lhs, const HandleStruct<N> &rhs) {
    return lhs.handle == rhs.handle;
}
template <int N>
inline bool operator!=(const HandleStruct<N> &lhs, const HandleStruct<N> &rhs) {
    return !operator==(lhs, rhs);
}
template <int N>
inline bool operator<(const HandleStruct<N> &lhs, const HandleStruct<N> &rhs) {
    return lhs.handle < rhs.handle;
}
template <int N>
inline bool operator>(const HandleStruct<N> &lhs, const HandleStruct<N> &rhs) {
    return operator<(rhs, lhs);
}
template <int N>
inline bool operator<=(const HandleStruct<N> &lhs, const HandleStruct<N> &rhs) {
    return !operator>(lhs, rhs);
}
template <int N>
inline bool operator>=(const HandleStruct<N> &lhs, const HandleStruct<N> &rhs) {
    return !operator<(lhs, rhs);
}

template <int N>
std::ostream &operator<<(std::ostream &out, const HandleStruct<N> &handleStruct) {
    return out << handleStruct.handle;
}

using ComponentWrapperHandle = HandleStruct<0>;
using ChannelSdkHandle = HandleStruct<1>;
using LinkSdkHandle = HandleStruct<2>;
using ConnectionSdkHandle = HandleStruct<3>;
using UserSdkHandle = HandleStruct<4>;
using PackageSdkHandle = HandleStruct<5>;
using EncodingHandle = HandleStruct<6>;
using DecodingHandle = HandleStruct<7>;
using UserComponentHandle = HandleStruct<8>;
using PackageFragmentHandle = HandleStruct<9>;

enum class EncodingState {
    UNENCODED,
    ENCODING,
    ENQUEUED,
    DONE,
};

std::string encodingStateToString(EncodingState encodingState);
std::ostream &operator<<(std::ostream &out, EncodingState encodingState);

/**
 * EncodingMode
 *
 * SINGLE indicates a package is not fragmented across multiple actions or batch so that multiple
 * packages fit in a single action
 *
 * BATCH does not fragment across multiple packages but does allow
 * multiple packages to fit in a single action. The bytes encoded are of the form (<len><package>)*.
 *
 * FRAGMENT_SINGLE_PRODUCER does fragment across a multiple packages and allows batching within a
 * single package, but only allows receiving packages from a single producer. This is because there
 * is no way to identify the sender and so fragment may get intermingled. The bytes encoded are of
 * the form <fragment id><flags>(<len><package>)*.
 *
 * FRAGMENT_MULTIPLE_PRODUCER is like the single
 * producer version but also includes a producer identifier allowing it avoid intermingling
 * fragments from different producers. The bytes encoded are of the form
 * <producer id><fragment id><flags>(<len><package>)*.
 */
enum class EncodingMode {
    SINGLE,
    BATCH,
    FRAGMENT_SINGLE_PRODUCER,
    FRAGMENT_MULTIPLE_PRODUCER,
};

std::string encodingModeToString(EncodingMode encodingMode);
std::ostream &operator<<(std::ostream &out, EncodingMode mode);

enum EncodingFlags {
    CONTINUE_LAST_PACKAGE = 1,
    CONTINUE_NEXT_PACKAGE = 2,
};

struct ActionInfo;
struct PackageInfo;

// This struct is owned by the ActionInfo and will live until the action has been executed by the
// ComponentActionManager.
struct EncodingInfo {
    EncodingParameters params;
    SpecificEncodingProperties props;
    EncodingHandle pendingEncodeHandle;
    EncodingState state;
    // The parent ActionInfo instance
    ActionInfo *info;
};

std::ostream &operator<<(std::ostream &out, const EncodingInfo &encodingInfo);

enum class PackageFragmentState {
    UNENCODED,
    ENCODING,
    ENQUEUED,
    DONE,
    SENT,
    FAILED,
};

std::string packageStateToString(PackageFragmentState packageState);
std::ostream &operator<<(std::ostream &out, PackageFragmentState packageState);

// This struct is owned by PackageInfo and will live until the package is removed
struct PackageFragmentInfo {
    PackageFragmentHandle handle;
    PackageInfo *package;
    PackageFragmentState state;

    // This is managed by the ComponentPackage manager and will point to the action used to send
    // this fragment if it exists. The action may have been completed and removed, in which case
    // this pointer will be null.
    ActionInfo *action;

    // offset and len refer to the offset and len of the bytes of the package in this fragment
    size_t offset;
    size_t len;

    bool markForDeletion;
};

std::string packageFragmentInfoToString(const PackageFragmentInfo &packageFragmentInfo);
std::ostream &operator<<(std::ostream &out, const PackageFragmentInfo &packageFragmentInfo);

// This struct is owned by the ComponentActionManager and lives in its global action timeline/queue.
struct ActionInfo {
    Action action;
    bool wildcardLink = false;
    LinkID linkId;
    std::vector<EncodingInfo> encoding;
    // The PackageFragmentInfo lives inside packageInfo which lives on the link's packageQueue and
    // outlives this struct because ComponentPackageManager's onPackageStatusChanged removes the
    // package and is only processed after the ComponentActionManager has executed and removed the
    // action.
    std::vector<PackageFragmentInfo *> fragments;
    bool toBeRemoved;
};

std::string actionInfoToString(const ActionInfo &actionInfo);
std::ostream &operator<<(std::ostream &out, const ActionInfo &actionInfo);

struct ActionCompare {
    bool operator()(const Action &a, const Action &b) const {
        return a.timestamp < b.timestamp || (a.timestamp == b.timestamp && a.actionId < b.actionId);
    }
    bool operator()(const Action *a, const Action *b) const {
        return (*this)(*a, *b);
    }
};

struct Link;

// This struct is owned by the ComponentPackageManager and lives on a link's packageQueue.
struct PackageInfo {
    // The parent Link instance
    Link *link;
    EncPkg pkg;
    PackageSdkHandle sdkHandle;
    EncodingHandle pendingEncodeHandle;
    std::vector<std::unique_ptr<PackageFragmentInfo>> packageFragments;
};

std::ostream &operator<<(std::ostream &out, const PackageInfo &packageInfo);

// This struct is owned by the ComponentLinkManager
struct Link {
    explicit Link(const LinkID &linkId) : linkId(linkId) {}
    LinkID linkId;
    std::unordered_set<ConnectionID> connections;
    // The ActionInfo is managed by the ComponentActionManager, which will add & remove them from
    // this queue. These are pointers to the actions as they live in the action manager's global
    // action timeline/queue.
    std::deque<ActionInfo *> actionQueue;
    // The PackageInfo is managed by the ComponentPackageManager, which will add & remove them from
    // this queue. The packages live here in the link's package queue.
    // unique_ptr is used to keep the address the same, even if the item in the deque is moved.
    std::deque<std::unique_ptr<PackageInfo>> packageQueue;
    LinkProperties props;
    std::vector<uint8_t> producerId;
    uint32_t fragmentCount = 0;

    struct ProducerQueue {
        uint32_t lastFragmentReceived;
        std::vector<uint8_t> pendingBytes;
    };
    std::unordered_map<std::string, ProducerQueue> producerQueues;
};

std::ostream &operator<<(std::ostream &out, const Link &link);

// This struct is owned by the ComponentConnectionManager
struct Connection {
    Connection(const ConnectionID &connId, const LinkID &linkId) : connId(connId), linkId(linkId) {}
    ConnectionID connId;
    LinkID linkId;
};

std::ostream &operator<<(std::ostream &out, const Connection &conn);

}  // namespace CMTypes

namespace std {
template <int N>
struct hash<CMTypes::HandleStruct<N>> {
    std::size_t operator()(const CMTypes::HandleStruct<N> &handleStruct) const {
        return std::hash<RaceHandle>{}(handleStruct.handle);
    }
};
}  // namespace std
