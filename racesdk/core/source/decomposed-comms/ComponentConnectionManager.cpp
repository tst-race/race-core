
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

#include "ComponentConnectionManager.h"

#include "ComponentManager.h"

using namespace CMTypes;

ComponentConnectionManager::ComponentConnectionManager(ComponentManagerInternal &manager) :
    manager(manager) {}

CMTypes::CmInternalStatus ComponentConnectionManager::openConnection(
    ComponentWrapperHandle /* postId */, ConnectionSdkHandle handle, LinkType /* linkType */,
    const LinkID &linkId, const std::string & /* linkHints */, int32_t /* sendTimeout */) {
    auto connId = manager.sdk.generateConnectionId(linkId);
    auto connection = std::make_unique<CMTypes::Connection>(connId, linkId);
    auto link = manager.getLink(linkId);
    connections[connId] = std::move(connection);
    link->connections.insert(connId);
    manager.sdk.onConnectionStatusChanged(handle.handle, connId, CONNECTION_OPEN, link->props,
                                          RACE_BLOCKING);
    return OK;
}

CMTypes::CmInternalStatus ComponentConnectionManager::closeConnection(
    ComponentWrapperHandle /* postId */, ConnectionSdkHandle handle, const ConnectionID &connId) {
    std::string logPrefix = "ComponentConnectionManager::closeConnection: ";
    auto it = connections.find(connId);
    if (it != connections.end()) {
        auto connection = it->second.get();
        auto link = manager.getLink(connection->linkId);
        link->connections.erase(connection->connId);
        manager.sdk.onConnectionStatusChanged(handle.handle, connId, CONNECTION_CLOSED, link->props,
                                              RACE_BLOCKING);
        connections.erase(it);
    } else {
        helper::logError(logPrefix + "Request to close non-existent connection: " + connId);
        return ERROR;
    }
    return OK;
}

void ComponentConnectionManager::teardown() {
    TRACE_METHOD();
    // TODO: tell sdk these are closed
    connections.clear();
}

void ComponentConnectionManager::setup() {
    TRACE_METHOD();
}