
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

#include "ComponentPackageManager.h"

#include <algorithm>

#include "ComponentManager.h"

using namespace CMTypes;

// This is the size of the <fragment id><flags> part of a fragmented message
static const size_t FRAGMENT_SINGLE_PRODUCER_OVERHEAD = 5;

// This is the size of the <producer id><fragment id><flags> part of a fragmented message
static const size_t FRAGMENT_MULTIPLE_PRODUCER_OVERHEAD = 21;

// How many bytes are used to store the length of the package for batched/fragmented messages
static const size_t FRAGMENT_LEN_SIZE = 4;

ComponentPackageManager::ComponentPackageManager(ComponentManagerInternal &manager) :
    manager(manager) {}

bool ComponentPackageManager::isLastFragment(CMTypes::PackageFragmentInfo *fragment) {
    return fragment->offset + fragment->len >= fragment->package->pkg.getSize();
}

bool ComponentPackageManager::isPackageFinished(PackageInfo *packageInfo) {
    for (auto &fragment : packageInfo->packageFragments) {
        if (fragment->state == PackageFragmentState::FAILED) {
            // Any failure means the package is finished
            return true;
        }
    }

    if (packageInfo->packageFragments.empty()) {
        // haven't created all necessary fragments for this package
        return false;
    }

    auto lastFragment = packageInfo->packageFragments.back().get();
    if (!isLastFragment(lastFragment)) {
        // haven't created all necessary fragments for this package
        return false;
    }

    for (auto &fragment : packageInfo->packageFragments) {
        if (fragment->state != PackageFragmentState::SENT) {
            // Some fragments haven't been sent yet
            return false;
        }
    }
    return true;
}

size_t ComponentPackageManager::spaceAvailableInAction(ActionInfo *actionInfo) {
    MAKE_LOG_PREFIX();
    if (actionInfo->toBeRemoved) {
        return 0;
    }

    if (manager.mode == EncodingMode::SINGLE && !actionInfo->fragments.empty()) {
        return 0;
    }

    for (auto &encodingInfo : actionInfo->encoding) {
        if (encodingInfo.state != EncodingState::UNENCODED) {
            return 0;
        }
    }

    size_t perFragmentOverhead = (manager.mode == EncodingMode::SINGLE) ? 0 : FRAGMENT_LEN_SIZE;
    size_t perActionOverhead = 0;
    if (manager.mode == EncodingMode::FRAGMENT_SINGLE_PRODUCER) {
        perActionOverhead = FRAGMENT_SINGLE_PRODUCER_OVERHEAD;
    } else if (manager.mode == EncodingMode::FRAGMENT_MULTIPLE_PRODUCER) {
        perActionOverhead = FRAGMENT_MULTIPLE_PRODUCER_OVERHEAD;
    } else if (manager.mode != EncodingMode::SINGLE && manager.mode != EncodingMode::BATCH) {
        helper::logError(logPrefix + "Unknown mode: " + encodingModeToString(manager.mode));
        return 0;
    }

    // TODO: cache this
    size_t maxBytes = 0;
    for (auto &encodingInfo : actionInfo->encoding) {
        maxBytes += static_cast<size_t>(encodingInfo.props.maxBytes);
    }

    // TODO: cache this
    size_t filled = perActionOverhead;
    for (auto &packageFragment : actionInfo->fragments) {
        filled += perFragmentOverhead;
        filled += packageFragment->len;
    }

    // watch out for underflow
    if (maxBytes > filled + perFragmentOverhead) {
        return maxBytes - filled - perFragmentOverhead;
    } else {
        return 0;
    }
}

bool ComponentPackageManager::isActionAbleToFit(ActionInfo *actionInfo, const EncPkg &pkg) {
    size_t spaceAvailable = spaceAvailableInAction(actionInfo);
    size_t minFragmentSize = 1;
    if (manager.mode == EncodingMode::SINGLE || manager.mode == EncodingMode::BATCH) {
        if (spaceAvailable > pkg.getSize()) {
            return true;
        }
    } else {
        if (spaceAvailable > minFragmentSize) {
            return true;
        }
    }
    return false;
}

bool ComponentPackageManager::isPackageAbleToFit(Link *link, const EncPkg &pkg) {
    for (auto actionInfo : link->actionQueue) {
        bool validLink = ((actionInfo->wildcardLink && actionInfo->linkId.empty()) ||
                          actionInfo->linkId == link->linkId);
        if (validLink && isActionAbleToFit(actionInfo, pkg)) {
            return true;
        }
    }
    return false;
}

bool ComponentPackageManager::isTimeToEncode(double now, ActionInfo *actionInfo) {
    auto encodingTime = manager.getMaxEncodingTime();

    if (now + encodingTime <= actionInfo->action.timestamp) {
        return true;
    } else {
        return false;
    }
}

bool ComponentPackageManager::generateFragmentsForPackage(double now, Link *link,
                                                          PackageInfo *packageInfo) {
    TRACE_METHOD(packageInfo->sdkHandle);
    size_t offset = 0;
    if (!packageInfo->packageFragments.empty()) {
        auto fragment = packageInfo->packageFragments.back().get();
        offset = fragment->offset + fragment->len;
    }

    if (offset == packageInfo->pkg.getSize()) {
        return true;
    }

    for (auto actionInfo : link->actionQueue) {
        if ((actionInfo->linkId == link->linkId or actionInfo->linkId.empty()) &&
            isActionAbleToFit(actionInfo, packageInfo->pkg) && isTimeToEncode(now, actionInfo)) {
            size_t spaceAvailable = spaceAvailableInAction(actionInfo);
            size_t bytesToEncode = std::min(spaceAvailable, packageInfo->pkg.getSize() - offset);

            auto packageFragment = std::make_unique<PackageFragmentInfo>(
                PackageFragmentInfo{{nextFragmentHandle++},
                                    packageInfo,
                                    PackageFragmentState::UNENCODED,
                                    actionInfo,
                                    offset,
                                    bytesToEncode,
                                    false});

            offset += bytesToEncode;

            fragments[packageFragment->handle] = packageFragment.get();
            actionInfo->fragments.push_back(packageFragment.get());
            packageInfo->packageFragments.push_back(std::move(packageFragment));
            actionInfo->linkId = link->linkId;

            if (offset == packageInfo->pkg.getSize()) {
                return true;
            }
        }
    }
    return false;
}

PluginResponse ComponentPackageManager::sendPackage(ComponentWrapperHandle /* postId */, double now,
                                                    PackageSdkHandle handle,
                                                    const ConnectionID &connId, EncPkg &&pkg,
                                                    double /* timeoutTimestamp */,
                                                    uint64_t /* batchId */) {
    TRACE_METHOD(handle, connId);
    auto linkId = manager.getConnection(connId)->linkId;
    auto link = manager.getLink(linkId);

    if (!isPackageAbleToFit(link, pkg)) {
        return PLUGIN_TEMP_ERROR;
    }

    auto packageInfo = std::make_unique<PackageInfo>(
        PackageInfo{link, std::move(pkg), handle, {NULL_RACE_HANDLE}, {}});

    if (!generateFragmentsForPackage(now, link, packageInfo.get())) {
        helper::logError("Failed to generate fragments for package");
    }
    link->packageQueue.push_back(std::move(packageInfo));

    // TODO: TimeoutTimestamp? batchId?
    return PLUGIN_OK;
}

void ComponentPackageManager::encodeForAction(CMTypes::ActionInfo *actionInfo) {
    TRACE_METHOD(actionInfo->action.actionId);

    if (actionInfo->linkId.empty() && !actionInfo->fragments.empty()) {
        throw std::runtime_error("Action with fragments has no linkId");
    }

    if (manager.mode == EncodingMode::SINGLE && actionInfo->fragments.size() > 1) {
        throw std::runtime_error("Got multiple fragments in an action with mode == SINGLE");
    }

    std::vector<uint8_t> bytesToEncode;
    if (!actionInfo->fragments.empty()) {
        auto link = manager.getLink(actionInfo->linkId);
        if (manager.mode == EncodingMode::FRAGMENT_MULTIPLE_PRODUCER) {
            bytesToEncode.insert(bytesToEncode.end(), link->producerId.begin(),
                                 link->producerId.end());
        }

        if (manager.mode == EncodingMode::FRAGMENT_MULTIPLE_PRODUCER ||
            manager.mode == EncodingMode::FRAGMENT_SINGLE_PRODUCER) {
            uint8_t *lenPtr = reinterpret_cast<uint8_t *>(&link->fragmentCount);
            bytesToEncode.insert(bytesToEncode.end(), lenPtr, lenPtr + sizeof(link->fragmentCount));
            link->fragmentCount += 1;

            uint8_t flags = 0;
            if (actionInfo->fragments.front()->offset != 0) {
                flags |= EncodingFlags::CONTINUE_LAST_PACKAGE;
            }
            if (!isLastFragment(actionInfo->fragments.back())) {
                flags |= EncodingFlags::CONTINUE_NEXT_PACKAGE;
            }
            bytesToEncode.push_back(flags);
        }

        for (auto packageFragment : actionInfo->fragments) {
            auto data = packageFragment->package->pkg.getRawData();
            uint32_t offset = packageFragment->offset;
            uint32_t len = packageFragment->len;
            auto begin = data.data() + offset;
            auto end = data.data() + offset + len;
            if (manager.mode != EncodingMode::SINGLE) {
                uint8_t *lenPtr = reinterpret_cast<uint8_t *>(&len);
                bytesToEncode.insert(bytesToEncode.end(), lenPtr, lenPtr + sizeof(len));
            }

            bytesToEncode.insert(bytesToEncode.end(), begin, end);
            packageFragment->state = PackageFragmentState::ENCODING;
        }
    }

    for (auto &encodingInfo : actionInfo->encoding) {
        if (encodingInfo.state != EncodingState::UNENCODED) {
            continue;
        }

        EncodingHandle encodingHandle{++nextEncodingHandle};
        encodingInfo.pendingEncodeHandle = encodingHandle;
        encodingInfo.state = EncodingState::ENCODING;

        // get the next package(s) to encode
        for (auto packageFragment : actionInfo->fragments) {
            packageFragment->package->pendingEncodeHandle = encodingHandle;
        }

        auto encoding = manager.encodingComponentFromEncodingParams(encodingInfo.params);
        if (encoding == nullptr) {
            helper::logError(logPrefix + "Failed to find encoding for params. Encoding type: " +
                             encodingInfo.params.type);
            continue;
        }

        pendingEncodings[encodingHandle] = &encodingInfo;
        encodingInfo.params.linkId = actionInfo->linkId;
        encoding->encodeBytes(encodingHandle, encodingInfo.params, bytesToEncode);
    }
}

CMTypes::CmInternalStatus ComponentPackageManager::onLinkStatusChanged(
    CMTypes::ComponentWrapperHandle /* postId */, CMTypes::LinkSdkHandle /* handle */,
    const LinkID &linkId, LinkStatus status, const LinkParameters & /* params */) {
    TRACE_METHOD(linkId, status);
    try {
        if (status == LINK_DESTROYED) {
            auto link = manager.getLink(linkId);

            // remove packages from pending encodings
            for (auto actionInfo : link->actionQueue) {
                for (auto &encodingInfo : actionInfo->encoding) {
                    pendingEncodings.erase(encodingInfo.pendingEncodeHandle);
                    encodingInfo.state = EncodingState::UNENCODED;
                }
                actionInfo->fragments.clear();
            }

            // remove link fragments from global fragments and notify networkManager of failed
            // packages
            for (auto &packageInfo : link->packageQueue) {
                // have to inform networkManager now. It's possible this call causes a queued
                // onPackageStatusChanged call to fail due to missing fragments. It's also possible
                // that that call succeeded and this package was actually sent out.
                manager.sdk.onPackageStatusChanged(packageInfo->sdkHandle.handle,
                                                   PACKAGE_FAILED_GENERIC, RACE_BLOCKING);

                for (auto &fragment : packageInfo->packageFragments) {
                    fragments.erase(fragment->handle);
                }
            }

            link->packageQueue.clear();
        }
    } catch (std::out_of_range & /* e */) {
        // Nothing to do if the link doesn't exist
    }

    return OK;
}

bool ComponentPackageManager::isActionEncoded(CMTypes::ActionInfo *actionInfo) {
    return std::any_of(actionInfo->encoding.begin(), actionInfo->encoding.end(),
                       [](const auto &val) { return val.state == EncodingState::ENQUEUED; });
}

CMTypes::CmInternalStatus ComponentPackageManager::onBytesEncoded(
    ComponentWrapperHandle /* postId */, EncodingHandle handle, std::vector<uint8_t> &&bytes,
    EncodingStatus status) {
    TRACE_METHOD(handle, status);
    auto iter = pendingEncodings.find(handle);
    if (iter == pendingEncodings.end()) {
        helper::logInfo(
            logPrefix +
            "No pending encodings found, action may have been canceled or already executed");
        return OK;
    }

    auto pendingEncoding = iter->second;
    pendingEncodings.erase(iter);

    if (status == ENCODE_OK) {
        manager.getTransport()->enqueueContent(pendingEncoding->params,
                                               pendingEncoding->info->action, bytes);
        pendingEncoding->state = EncodingState::ENQUEUED;
        if (isActionEncoded(pendingEncoding->info)) {
            for (auto fragment : pendingEncoding->info->fragments) {
                fragment->state = PackageFragmentState::ENQUEUED;
            }
        }
    } else {
        helper::logError(logPrefix + "Encoding failed");
    }
    return OK;
}

CMTypes::CmInternalStatus ComponentPackageManager::onPackageStatusChanged(
    ComponentWrapperHandle /* postId */, PackageFragmentHandle handle, PackageStatus status) {
    TRACE_METHOD(handle, status);

    // Remove the package from the link's packageQueue
    auto fragmentIter = fragments.find(handle);
    if (fragmentIter != fragments.end()) {
        auto fragment = fragmentIter->second;
        auto pkg = fragment->package;
        auto link = pkg->link;

        if (status == PackageStatus::PACKAGE_SENT) {
            fragment->state = PackageFragmentState::SENT;
        } else if (status == PackageStatus::PACKAGE_RECEIVED) {
            // TODO
        } else {
            fragment->state = PackageFragmentState::FAILED;
        }

        fragments.erase(fragmentIter);

        if (isPackageFinished(pkg)) {
            bool reAssignAllActions = false;
            for (auto &packageFragment : pkg->packageFragments) {
                if (packageFragment->action != nullptr &&
                    packageFragment->state == PackageFragmentState::UNENCODED) {
                    reAssignAllActions = true;

                    auto action = packageFragment->action;
                    for (auto &packageFragment2 : action->fragments) {
                        packageFragment2->markForDeletion = true;
                        packageFragment2->action = nullptr;
                    }
                    action->fragments.clear();
                }

                // remove any remaining fragments
                fragments.erase(packageFragment->handle);
            }

            manager.sdk.onPackageStatusChanged(pkg->sdkHandle.handle, status, RACE_BLOCKING);
            link->packageQueue.erase(
                std::remove_if(
                    link->packageQueue.begin(), link->packageQueue.end(),
                    [&pkg](const auto &queuePkg) { return pkg->sdkHandle == queuePkg->sdkHandle; }),
                link->packageQueue.end());

            if (reAssignAllActions) {
                generateFragmentsForAllPackages();
            }
        }
    } else {
        helper::logDebug(logPrefix +
                         "Unable to find fragment with handle: " + std::to_string(handle.handle));
    }

    return OK;
}

void ComponentPackageManager::updatedActions() {
    generateFragmentsForAllPackages();
}

void ComponentPackageManager::removeMarkedFragments(CMTypes::Link *link) {
    // clean up package fragments that were marked for deletion
    for (auto &packageInfo : link->packageQueue) {
        auto begin = packageInfo->packageFragments.begin();
        auto end = packageInfo->packageFragments.end();

        for (auto it = begin; it != end; it++) {
            if ((*it)->markForDeletion) {
                for (auto it2 = it; it2 != end; it2++) {
                    fragments.erase((*it2)->handle);
                }
                packageInfo->packageFragments.erase(it, end);
                break;
            }
        }
    }
}

void ComponentPackageManager::generateFragmentsForAllPackages() {
    auto links = manager.getLinks();
    auto now = helper::currentTime();

    // remove packages
    // TODO: only remove packages if an action has been inserted or removed, or if a package has
    // been removed from any of the links
    for (auto &link : links) {
        for (auto actionInfo : link->actionQueue) {
            if (!isTimeToEncode(now, actionInfo)) {
                continue;
            }

            // don't remove packages from actions that are already being encoded for
            for (auto &encodingInfo : actionInfo->encoding) {
                if (encodingInfo.state != EncodingState::UNENCODED) {
                    continue;
                }
            }

            // Reset package state for any currently enqueued content
            for (auto &packageFragment : actionInfo->fragments) {
                packageFragment->markForDeletion = true;
                packageFragment->action = nullptr;
            }
            actionInfo->fragments.clear();
            if (actionInfo->wildcardLink) {
                actionInfo->linkId.clear();
            }
        }

        removeMarkedFragments(link);
    }

    size_t maxPackages = 0;
    for (auto &link : links) {
        maxPackages = std::max(maxPackages, link->packageQueue.size());
    }

    // assign packages to actions
    std::vector<bool> queueFull(links.size(), false);
    for (size_t i = 0; i < maxPackages; i++) {
        for (size_t j = 0; j < links.size(); j++) {
            auto link = links[j];
            if (link->packageQueue.size() <= i || queueFull[j]) {
                continue;
            }

            auto &packageInfo = link->packageQueue[i];
            if (!packageInfo->packageFragments.empty()) {
                auto frag = packageInfo->packageFragments.back().get();
                if (isLastFragment(frag)) {
                    continue;
                }
            }

            if (!generateFragmentsForPackage(now, link, packageInfo.get())) {
                queueFull[j] = true;
            }
        }
    }

    for (size_t i = 0; i < links.size(); i++) {
        if (!queueFull[i]) {
            // notify sdk that we're not blocked
            // it's okay if we weren't blocked before and do this anyway
            for (auto &connId : links[i]->connections) {
                manager.sdk.unblockQueue(connId);
            }
        }
    }
}

void ComponentPackageManager::actionDone(CMTypes::ActionInfo *actionInfo) {
    TRACE_METHOD(actionInfo->action.actionId);

    for (auto &encodingInfo : actionInfo->encoding) {
        auto iter = pendingEncodings.find(encodingInfo.pendingEncodeHandle);
        if (iter != pendingEncodings.end()) {
            helper::logWarning(
                logPrefix + "Action completed while encoding is still pending, encoding handle: " +
                encodingInfo.pendingEncodeHandle.to_string());
            pendingEncodings.erase(iter);
        }
        encodingInfo.state = EncodingState::DONE;
    }

    for (auto packageFragment : actionInfo->fragments) {
        packageFragment->state = PackageFragmentState::DONE;
        packageFragment->action = nullptr;
    }
}

std::vector<CMTypes::PackageFragmentHandle> ComponentPackageManager::getPackageHandlesForAction(
    CMTypes::ActionInfo *actionInfo) {
    TRACE_METHOD(actionInfo->action.actionId);
    std::vector<CMTypes::PackageFragmentHandle> handles;
    std::transform(actionInfo->fragments.begin(), actionInfo->fragments.end(),
                   std::back_inserter(handles), [](const auto &frag) { return frag->handle; });
    return handles;
}

void ComponentPackageManager::teardown() {
    TRACE_METHOD();
    pendingEncodings.clear();
    fragments.clear();
}

void ComponentPackageManager::setup() {
    TRACE_METHOD();
}

static std::ostream &operator<<(
    std::ostream &out,
    const std::unordered_map<CMTypes::EncodingHandle, CMTypes::EncodingInfo *> &pendingEncodings) {
    std::map orderedEncodings{pendingEncodings.begin(), pendingEncodings.end()};
    out << "{";
    for (auto &iter : orderedEncodings) {
        out << "\n        " << iter.first << ":" << *iter.second << ", ";
    }
    if (!orderedEncodings.empty()) {
        out << "\n    ";
    }
    out << "}";
    return out;
}

static std::ostream &operator<<(
    std::ostream &out,
    const std::unordered_map<CMTypes::PackageFragmentHandle, CMTypes::PackageFragmentInfo *>
        &fragments) {
    std::map orderedFragments{fragments.begin(), fragments.end()};
    out << "{";
    for (auto &iter : orderedFragments) {
        out << "\n        " << iter.first << ":" << *iter.second << ", ";
    }
    if (!orderedFragments.empty()) {
        out << "\n    ";
    }
    out << "}";
    return out;
}

std::ostream &operator<<(std::ostream &out, const ComponentPackageManager &manager) {
    return out << "PackageManager{"
               << "\n    pendingEncodings:" << manager.pendingEncodings
               << "\n    nextEncodingHandle: " << manager.nextEncodingHandle
               << "\n    fragments: " << manager.fragments << "\n}";
}
