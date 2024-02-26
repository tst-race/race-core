
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

#include "ComponentReceivePackageManager.h"

#include "ComponentManager.h"

using namespace CMTypes;

// reads a value of type T (must be a POD type) from a buffer and updates offset accordingly
template <typename T>
T readFromBuffer(const std::vector<uint8_t> &buffer, size_t &offset) {
    if (offset + sizeof(T) > buffer.size()) {
        throw std::out_of_range("Tried to read beyond buffer: offset: " + std::to_string(offset) +
                                ", sizeof(T): " + std::to_string(sizeof(T)) +
                                ", buffer.size(): " + std::to_string(buffer.size()));
    }

    T value;
    memcpy(&value, buffer.data() + offset, sizeof(value));
    offset += sizeof(value);
    return value;
}

ComponentReceivePackageManager::ComponentReceivePackageManager(ComponentManagerInternal &manager) :
    manager(manager) {}

CmInternalStatus ComponentReceivePackageManager::onReceive(ComponentWrapperHandle postId,
                                                           const LinkID &linkId,
                                                           const EncodingParameters &params,
                                                           std::vector<uint8_t> &&bytes) {
    TRACE_METHOD(postId, linkId, bytes.size());

    // TODO: decode packages based on multiple encoding parameters
    auto encoding = manager.encodingComponentFromEncodingParams(params);
    if (encoding == nullptr) {
        helper::logError(logPrefix +
                         "Failed to find encoding for params. Encoding type: " + params.type);
        return ERROR;
    }
    DecodingHandle decodingHandle{++nextDecodingHandle};
    pendingDecodings[decodingHandle] = linkId;
    encoding->decodeBytes(decodingHandle, params, bytes);

    return OK;
}

CmInternalStatus ComponentReceivePackageManager::onBytesDecoded(ComponentWrapperHandle postId,
                                                                DecodingHandle handle,
                                                                std::vector<uint8_t> &&bytes,
                                                                EncodingStatus status) {
    TRACE_METHOD(postId, handle, bytes.size(), status);

    if (bytes.empty()) {
        // Expected result of decoding cover traffic
        return OK;
    }

    // TODO: handle packages encoded with multiple encodings. This works if they are each
    // separate EncPkgs, but if stuff ever gets fragmented across multiple encode calls, we need
    // a way to recombine them.
    try {
        auto linkId = pendingDecodings.at(handle);
        pendingDecodings.erase(handle);
        auto link = manager.getLink(linkId);
        auto connSet = link->connections;
        std::vector<std::string> connVec = {connSet.begin(), connSet.end()};

        if (manager.mode == EncodingMode::SINGLE) {
            return receiveSingle(std::move(bytes), std::move(connVec));
        } else if (manager.mode == EncodingMode::BATCH) {
            return receiveBatch(std::move(bytes), std::move(connVec));
        } else if (manager.mode == EncodingMode::FRAGMENT_SINGLE_PRODUCER) {
            return receiveFragmentSingleProducer(link, std::move(bytes), std::move(connVec));
        } else if (manager.mode == EncodingMode::FRAGMENT_MULTIPLE_PRODUCER) {
            return receiveFragmentMultipleProducer(link, std::move(bytes), std::move(connVec));
        } else {
            throw std::runtime_error("Not implemented");
        }
    } catch (std::exception &e) {
        helper::logError(logPrefix + "Exception: " + e.what());
        return ERROR;
    }
}

std::vector<uint8_t> ComponentReceivePackageManager::readFragment(
    const std::vector<uint8_t> &buffer, size_t &offset) {
    uint32_t len = readFromBuffer<uint32_t>(buffer, offset);

    if (offset + len > buffer.size()) {
        throw std::out_of_range("Tried to read beyond buffer: offset: " + std::to_string(offset) +
                                ", len: " + std::to_string(len) +
                                ", buffer.size(): " + std::to_string(buffer.size()));
    }

    offset += len;
    return std::vector<uint8_t>(buffer.data() + offset - len, buffer.data() + offset);
}

CmInternalStatus ComponentReceivePackageManager::receiveSingle(std::vector<uint8_t> &&bytes,
                                                               std::vector<std::string> &&connVec) {
    TRACE_METHOD(bytes.size(), connVec.size());

    EncPkg pkg{std::move(bytes)};
    manager.sdk.receiveEncPkg(pkg, connVec, RACE_BLOCKING);
    return OK;
}

CmInternalStatus ComponentReceivePackageManager::receiveBatch(std::vector<uint8_t> &&bytes,
                                                              std::vector<std::string> &&connVec) {
    TRACE_METHOD(bytes.size(), connVec.size());

    size_t offset = 0;
    while (offset < bytes.size()) {
        auto pkgBytes = readFragment(bytes, offset);
        EncPkg pkg(pkgBytes);
        manager.sdk.receiveEncPkg(pkg, connVec, RACE_BLOCKING);
    }

    return OK;
}

CmInternalStatus ComponentReceivePackageManager::receiveFragmentSingleProducer(
    Link *link, std::vector<uint8_t> &&bytes, std::vector<std::string> &&connVec) {
    TRACE_METHOD(bytes.size(), connVec.size());

    return receiveFragmentProducer("", 0, link, std::move(bytes), std::move(connVec));
}

CmInternalStatus ComponentReceivePackageManager::receiveFragmentMultipleProducer(
    Link *link, std::vector<uint8_t> &&bytes, std::vector<std::string> &&connVec) {
    TRACE_METHOD(bytes.size(), connVec.size());

    size_t offset = 0;
    auto producer = readFromBuffer<std::array<uint8_t, 16>>(bytes, offset);

    std::string producerString(producer.begin(), producer.end());
    return receiveFragmentProducer(producerString, offset, link, std::move(bytes),
                                   std::move(connVec));
}

CmInternalStatus ComponentReceivePackageManager::receiveFragmentProducer(
    const std::string &producer, size_t offset, Link *link, std::vector<uint8_t> &&bytes,
    std::vector<std::string> &&connVec) {
    TRACE_METHOD(offset, link->linkId, bytes.size(), connVec.size());

    uint32_t fragmentId = readFromBuffer<uint32_t>(bytes, offset);

    auto fragmentQueue = &link->producerQueues[producer];
    if (fragmentId != fragmentQueue->lastFragmentReceived + 1) {
        // Currently there's no support for out of order packages
        // if we get packages out of order, drop old packages
        fragmentQueue->pendingBytes.clear();
    }
    fragmentQueue->lastFragmentReceived = fragmentId;

    uint8_t flags = readFromBuffer<uint8_t>(bytes, offset);
    if (!(flags & CONTINUE_LAST_PACKAGE) && !fragmentQueue->pendingBytes.empty()) {
        // Just in case the previous package was marked as continued and this one wasn't, clear the
        // buffer.
        helper::logDebug(logPrefix + "Clearing pending bytes from previous fragment");
        fragmentQueue->pendingBytes.clear();
    }

    bool firstFragment = true;
    while (offset < bytes.size()) {
        auto pkgBytes = readFragment(bytes, offset);

        if (firstFragment && (flags & CONTINUE_LAST_PACKAGE) &&
            fragmentQueue->pendingBytes.empty()) {
            firstFragment = false;
            // A fragment was lost before this so we can't reconstruct the package. No point in
            // keeping this data around.
            helper::logDebug(logPrefix +
                             "Dropping fragment because previous fragments are missing");
            continue;
        }
        firstFragment = false;

        fragmentQueue->pendingBytes.insert(fragmentQueue->pendingBytes.end(), pkgBytes.begin(),
                                           pkgBytes.end());

        if ((flags & CONTINUE_NEXT_PACKAGE) && offset >= bytes.size()) {
            // This wasn't the end of the package. There should be more fragments of the package in
            // the future.
            helper::logDebug("Package continues in next fragment");
            continue;
        }

        EncPkg pkg(std::move(fragmentQueue->pendingBytes));
        fragmentQueue->pendingBytes.clear();
        manager.sdk.receiveEncPkg(pkg, connVec, RACE_BLOCKING);
    }

    return OK;
}

void ComponentReceivePackageManager::teardown() {
    TRACE_METHOD();

    pendingDecodings.clear();
}

void ComponentReceivePackageManager::setup() {
    TRACE_METHOD();
}

static std::ostream &operator<<(
    std::ostream &out,
    const std::unordered_map<CMTypes::DecodingHandle, LinkID> &pendingDecodings) {
    std::map orderedDecodings{pendingDecodings.begin(), pendingDecodings.end()};
    out << "{";
    for (auto &iter : orderedDecodings) {
        out << iter.first << ":" << iter.second << ", ";
    }
    out << "}";
    return out;
}

std::ostream &operator<<(std::ostream &out, const ComponentReceivePackageManager &manager) {
    return out << "ReceivePackageManager{"
               << "nextDecodingHandle:" << manager.nextDecodingHandle
               << ", pendingDecodings: " << manager.pendingDecodings << "}";
}