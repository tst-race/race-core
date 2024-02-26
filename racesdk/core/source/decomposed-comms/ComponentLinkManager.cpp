
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

#include "ComponentLinkManager.h"

#include "ComponentManager.h"

using namespace CMTypes;

ComponentLinkManager::ComponentLinkManager(ComponentManagerInternal &manager) : manager(manager) {}

CMTypes::CmInternalStatus ComponentLinkManager::destroyLink(ComponentWrapperHandle /* postId */,
                                                            LinkSdkHandle handle,
                                                            const LinkID &linkId) {
    MAKE_LOG_PREFIX();
    try {
        manager.getTransport()->destroyLink(handle, linkId);

        return OK;
    } catch (std::exception &e) {
        helper::logError(logPrefix + "Exception: " + e.what());
        return ERROR;
    }
}

CMTypes::CmInternalStatus ComponentLinkManager::createLink(ComponentWrapperHandle /* postId */,
                                                           LinkSdkHandle handle,
                                                           const std::string & /* channelGid */) {
    LinkID linkId = manager.sdk.generateLinkId(manager.getCompositionId());
    manager.getTransport()->createLink(handle, linkId);
    // transport should eventually call onLinkStatusChanged
    return OK;
}

CMTypes::CmInternalStatus ComponentLinkManager::loadLinkAddress(
    ComponentWrapperHandle /* postId */, LinkSdkHandle handle, const std::string & /* channelGid */,
    const std::string &linkAddress) {
    LinkID linkId = manager.sdk.generateLinkId(manager.getCompositionId());
    manager.getTransport()->loadLinkAddress(handle, linkId, linkAddress);
    // transport should eventually call onLinkStatusChanged
    return OK;
}

CMTypes::CmInternalStatus ComponentLinkManager::loadLinkAddresses(
    ComponentWrapperHandle /* postId */, LinkSdkHandle handle, const std::string & /* channelGid */,
    const std::vector<std::string> &linkAddresses) {
    LinkID linkId = manager.sdk.generateLinkId(manager.getCompositionId());
    manager.getTransport()->loadLinkAddresses(handle, linkId, linkAddresses);
    // transport should eventually call onLinkStatusChanged
    return OK;
}

CMTypes::CmInternalStatus ComponentLinkManager::createLinkFromAddress(
    ComponentWrapperHandle /* postId */, LinkSdkHandle handle, const std::string & /* channelGid */,
    const std::string &linkAddress) {
    LinkID linkId = manager.sdk.generateLinkId(manager.getCompositionId());
    manager.getTransport()->createLinkFromAddress(handle, linkId, linkAddress);
    // transport should eventually call onLinkStatusChanged
    return OK;
}

CMTypes::CmInternalStatus ComponentLinkManager::onLinkStatusChanged(ComponentWrapperHandle postId,
                                                                    LinkSdkHandle handle,
                                                                    const LinkID &linkId,
                                                                    LinkStatus status,
                                                                    const LinkParameters &params) {
    auto props = manager.getTransport()->getLinkProperties(linkId);
    if (status == LINK_CREATED || status == LINK_LOADED) {
        manager.getUserModel()->addLink(linkId, params);
        auto link = std::make_unique<CMTypes::Link>(linkId);
        link->props = props;
        link->producerId = manager.sdk.getEntropy(16);
        links[linkId] = std::move(link);
        // TODO: onLinkStatusChange LINK_DESTROYED in case of failure? how to remove from transport?
        // should we just let the transport handle it instead?
    } else if (status == LINK_DESTROYED) {
        auto link = links.at(linkId).get();

        // create a copy of connections so modifying the orginal doesn't cause problems
        auto connections = link->connections;
        for (const ConnectionID &connId : connections) {
            manager.closeConnection(postId, {NULL_RACE_HANDLE}, connId);
        }

        links.erase(linkId);
        manager.getUserModel()->removeLink(linkId);
    }

    manager.sdk.onLinkStatusChanged(handle.handle, linkId, status, props, RACE_BLOCKING);
    return OK;
}

void ComponentLinkManager::teardown() {
    TRACE_METHOD();
    // TODO: tell sdk these are closed
    links.clear();
}

void ComponentLinkManager::setup() {
    TRACE_METHOD();
}