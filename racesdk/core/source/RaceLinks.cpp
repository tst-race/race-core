
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

#include "RaceLinks.h"

#include <atomic>         // std::atomic
#include <stdexcept>      // std::invalid_argument
#include <unordered_set>  // std::unordered_set

#include "helper.h"

RaceLinks::RaceLinks() : currentLinkId(0) {}

void RaceLinks::addLink(const std::string &linkId, const personas::PersonaSet &personas) {
    std::lock_guard<std::mutex> lock{linksLock};
    if (linkIdToInfo.find(linkId) != linkIdToInfo.end()) {
        throw std::invalid_argument("Link ID \"" + linkId + "\" already exists");
    }
    // Map the link ID to the link profile and reachable personas.
    linkIdToInfo[linkId].personas = personas::PersonaSet(personas.begin(), personas.end());
    linkIdToInfo[linkId].connIdToInfo.clear();
}

void RaceLinks::removeLink(const std::string &linkId) {
    std::lock_guard<std::mutex> lock{linksLock};
    auto linkInfoPairIt = linkIdToInfo.find(linkId);
    if (linkInfoPairIt != linkIdToInfo.end()) {
        if (linkInfoPairIt->second.personas.size() > 0) {
            destroyedLinkProfiles[linkId] = linkInfoPairIt->second.personas;
            linkInfoPairIt->second.personas.clear();
        }

        for (auto connIdInfoPair : linkInfoPairIt->second.connIdToInfo) {
            connToLink.erase(connIdInfoPair.first);
        }
        linkIdToInfo.erase(linkId);
    }
}

const std::string RaceLinks::completeNewLinkRequest(RaceHandle handle, const std::string &linkId) {
    std::lock_guard<std::mutex> lock{linksLock};
    if (!handleToNewLink.count(handle)) {
        throw std::invalid_argument(
            "Handle " + std::to_string(handle) +
            " not found in map. Did this handle correspond to a createLink/loadLinkAddress call?");
    }
    const auto [personas, address] = handleToNewLink[handle];
    linkIdToInfo[linkId].personas = personas;
    linkIdToInfo[linkId].connIdToInfo.clear();
    return address;
}

void RaceLinks::addNewLinkRequest(RaceHandle handle, const personas::PersonaSet &personas,
                                  const std::string &linkAddress) {
    std::lock_guard<std::mutex> lock{linksLock};
    handleToNewLink.insert({handle, {personas, linkAddress}});
}

void RaceLinks::removeNewLinkRequest(RaceHandle handle, const std::string &linkId) {
    std::lock_guard<std::mutex> lock{linksLock};
    if (handleToNewLink.count(handle) > 0) {
        destroyedLinkProfiles[linkId] = handleToNewLink[handle].first;
    }
    handleToNewLink.erase(handle);
}

void RaceLinks::addConnectionRequest(RaceHandle handle, const LinkID &linkId) {
    std::lock_guard<std::mutex> lock{linksLock};
    handleToLink[handle] = linkId;  // This may overwrite an old handle, do we care?
}

void RaceLinks::addConnection(RaceHandle handle, const ConnectionID &connId) {
    std::lock_guard<std::mutex> lock{linksLock};
    auto handleLinkIter = handleToLink.find(handle);
    if (handleLinkIter == handleToLink.end()) {
        throw std::invalid_argument("Handle mapping does not exist: " + std::to_string(handle));
    }
    std::string linkId = handleLinkIter->second;
    handleToLink.erase(handleLinkIter);

    if (linkIdToInfo.find(linkId) == linkIdToInfo.end()) {
        throw std::invalid_argument("Link ID does not exist: " + linkId);
    }
    if (linkIdToInfo[linkId].connIdToInfo.find(connId) != linkIdToInfo[linkId].connIdToInfo.end() ||
        connToLink.find(connId) != connToLink.end()) {
        throw std::invalid_argument("Connection ID is already present: " + connId);
    }
    connToLink[connId] = linkId;
    linkIdToInfo[linkId].connIdToInfo[connId];
}

void RaceLinks::removeConnectionRequest(RaceHandle handle) {
    std::lock_guard<std::mutex> lock{linksLock};
    handleToLink.erase(handle);
}

void RaceLinks::removeConnection(const ConnectionID &connId) {
    std::lock_guard<std::mutex> lock{linksLock};
    connToLink.erase(connId);
    LinkID linkId = getLinkIDFromConnectionID(connId);
    if (linkIdToInfo.count(linkId) > 0) {
        linkIdToInfo[linkId].connIdToInfo.erase(connId);
    }
}

bool RaceLinks::doesConnectionExist(const ConnectionID &connId) {
    std::lock_guard<std::mutex> lock{linksLock};
    return doesConnectionExistInternal(connId);
}

std::unordered_set<ConnectionID> RaceLinks::doConnectionsExist(
    const std::unordered_set<ConnectionID> &connectionIds) {
    std::unordered_set<ConnectionID> closedConnections;
    std::lock_guard<std::mutex> lock{linksLock};
    for (const auto &connectionId : connectionIds) {
        if (!doesConnectionExistInternal(connectionId)) {
            closedConnections.insert(connectionId);
        }
    }
    return closedConnections;
}

void RaceLinks::updateLinkProperties(const LinkID &linkId, const LinkProperties &properties) {
    if (!isValidLinkType(properties.linkType)) {
        throw std::invalid_argument("invalid link type: " +
                                    std::to_string(static_cast<std::int32_t>(properties.linkType)));
    }
    std::lock_guard<std::mutex> lock{linksLock};
    // Check if the link ID has been added, throwing an exception if not.
    linkIdToInfo.at(linkId);
    linkIdToInfo[linkId].properties = properties;
}

LinkProperties RaceLinks::getLinkProperties(const LinkID &linkId) {
    std::lock_guard<std::mutex> lock{linksLock};
    return linkIdToInfo.at(linkId).properties;
}

bool RaceLinks::setPersonasForLink(const std::string &linkId,
                                   const personas::PersonaSet &personas) {
    std::lock_guard<std::mutex> lock{linksLock};
    if (!linkIdToInfo.count(linkId)) {
        return false;
    }
    linkIdToInfo[linkId].personas = personas;
    return true;
}

personas::PersonaSet RaceLinks::getAllPersonaSet() {
    std::lock_guard<std::mutex> lock{linksLock};
    std::unordered_set<std::string> uniquePersonas;
    for (auto &linkIdInfoPair : linkIdToInfo) {
        const auto &personasForLink = linkIdInfoPair.second.personas;
        std::copy(personasForLink.begin(), personasForLink.end(),
                  std::inserter(uniquePersonas, uniquePersonas.end()));
    }

    return {uniquePersonas.begin(), uniquePersonas.end()};
}

bool RaceLinks::doesLinkIncludeGivenPersonas(const personas::PersonaSet &linkProfilePersonas,
                                             const personas::PersonaSet &givenPersonas) {
    for (const auto &givenPersona : givenPersonas) {
        if (linkProfilePersonas.find(givenPersona) == linkProfilePersonas.end()) {
            return false;
        }
    }

    return true;
}

std::vector<LinkID> RaceLinks::getAllLinksForPersonas(const personas::PersonaSet &personas,
                                                      const LinkType linkType) {
    std::vector<LinkID> links;
    std::lock_guard<std::mutex> lock{linksLock};
    for (const auto &linkIdInfoPair : linkIdToInfo) {
        // Check if the connection profle can reach all the personas.
        if (doesLinkIncludeGivenPersonas(linkIdInfoPair.second.personas, personas)) {
            const auto &linkId = linkIdInfoPair.first;
            // If the LinkID exists and is of the desired type then add it to the result.
            const LinkType linkTypeForLinkId = linkIdInfoPair.second.properties.linkType;
            if (linkTypeForLinkId == LT_BIDI || linkTypeForLinkId == linkType) {
                links.push_back(linkId);
            }
        }
    }
    return links;
}

personas::PersonaSet RaceLinks::getAllPersonasForLink(const LinkID &linkId) {
    std::lock_guard<std::mutex> lock{linksLock};
    try {
        return linkIdToInfo.at(linkId).personas;
    } catch (const std::out_of_range &) {
        try {
            return destroyedLinkProfiles.at(linkId);
        } catch (const std::out_of_range &) {
            return {};
        }
    }
}

LinkID RaceLinks::getLinkForConnection(const ConnectionID &connId) {
    std::lock_guard<std::mutex> lock{linksLock};
    return getLinkIDFromConnectionID(connId);
}

bool RaceLinks::doesConnectionExistInternal(const ConnectionID &connId) {
    return (connToLink.count(connId) > 0);
}

bool RaceLinks::isValidLinkType(const LinkType linkType) {
    return linkType == LT_SEND || linkType == LT_RECV || linkType == LT_BIDI;
}

LinkID RaceLinks::getNextLinkID(const std::string &plugin) {
    return plugin + "/LinkID_" + std::to_string(currentLinkId++);
}

std::string RaceLinks::getLinkIDFromConnectionID(const ConnectionID &connId) {
    std::size_t ind = connId.find('/');  // plugin
    ind = connId.find('/', ind + 1);     // channel GID
    ind = connId.find('/', ind + 1);     // Link ID
    if (ind == std::string::npos) {
        throw std::invalid_argument("connId does not include a LinkID: " + connId);
    }
    return connId.substr(0, ind);
}

std::string RaceLinks::getPluginFromLinkID(const LinkID &linkId) {
    std::size_t ind = linkId.find('/');
    if (ind == std::string::npos) {
        throw std::invalid_argument("LinkID does not include a plugin name: " + linkId);
    }
    return linkId.substr(0, ind);
}

std::string RaceLinks::getPluginFromConnectionID(const ConnectionID &connId) {
    // A LinkID is a prefix of ConnectionID, so this function works on either
    return getPluginFromLinkID(connId);
}

void RaceLinks::addTraceCtxForLink(const LinkID &linkId, std::uint64_t traceId,
                                   std::uint64_t spanId) {
    std::lock_guard<std::mutex> lock{linksLock};
    auto linkIdInfoIt = linkIdToInfo.find(linkId);
    if (linkIdInfoIt != linkIdToInfo.end()) {
        linkIdInfoIt->second.traceCtx = std::make_pair(traceId, spanId);
    } else {
        helper::logDebug("addTraceCtxForLink invalid link " + linkId);
    }
}

std::pair<std::uint64_t, std::uint64_t> RaceLinks::getTraceCtxForLink(const LinkID &linkId) {
    std::lock_guard<std::mutex> lock{linksLock};
    try {
        return linkIdToInfo.at(linkId).traceCtx;
    } catch (const std::out_of_range &) {
        return {};
    }
}

void RaceLinks::addTraceCtxForConnection(const ConnectionID &connId, std::uint64_t traceId,
                                         std::uint64_t spanId) {
    std::lock_guard<std::mutex> lock{linksLock};
    auto connLinkPairIt = connToLink.find(connId);
    if (connLinkPairIt == connToLink.end()) {
        helper::logDebug("addTraceCtxForConnection for invalid connection: " + connId);
    } else {
        LinkID &linkId = connLinkPairIt->second;
        auto linkIdInfoIt = linkIdToInfo.find(linkId);
        if (linkIdInfoIt != linkIdToInfo.end()) {
            linkIdInfoIt->second.connIdToInfo[connId].traceCtx = std::make_pair(traceId, spanId);
        }
    }
}

std::pair<std::uint64_t, std::uint64_t> RaceLinks::getTraceCtxForConnection(
    const ConnectionID &connId) {
    std::lock_guard<std::mutex> lock{linksLock};
    try {
        auto connLinkPairIt = connToLink.find(connId);
        if (connLinkPairIt == connToLink.end()) {
            return {};
        }
        return linkIdToInfo.at(connLinkPairIt->second).connIdToInfo.at(connId).traceCtx;
    } catch (const std::out_of_range &) {
        return {};
    }
}

std::unordered_set<ConnectionID> RaceLinks::getLinkConnections(const LinkID &linkId) {
    std::lock_guard<std::mutex> lock{linksLock};
    if (linkIdToInfo.find(linkId) == linkIdToInfo.end()) {
        return std::unordered_set<ConnectionID>();
    } else {
        std::unordered_set<ConnectionID> connIds;
        for (auto linkInfoPair : linkIdToInfo[linkId].connIdToInfo) {
            connIds.insert(linkInfoPair.first);
        }
        return connIds;
    }
}

void RaceLinks::cachePackageHandle(const ConnectionID &connId, const RaceHandle &packageHandle) {
    std::lock_guard<std::mutex> lock{linksLock};
    auto connLinkPairIt = connToLink.find(connId);
    if (connLinkPairIt == connToLink.end()) {
        helper::logInfo("attempt to cache package handle for uncached connection: " + connId);
    } else if (packageHandle == NULL_RACE_HANDLE) {
        helper::logInfo("attempt to cache package with null handle ");
    } else if (linkIdToInfo.find(connLinkPairIt->second) == linkIdToInfo.end()) {
        helper::logInfo("attempt to cache package handle for non-existent link: " +
                        connLinkPairIt->second);
    } else if (linkIdToInfo[connLinkPairIt->second].connIdToInfo.find(connId) ==
               linkIdToInfo[connLinkPairIt->second].connIdToInfo.end()) {
        helper::logInfo("attempt to cache package handle for non-existent connection: " + connId);
    } else {
        linkIdToInfo[connLinkPairIt->second].connIdToInfo[connId].packageHandles.insert(
            packageHandle);
        packageHandleConnectionMap[packageHandle] = connId;
    }
}

std::unordered_set<RaceHandle> RaceLinks::getCachedPackageHandles(const ConnectionID &connId) {
    std::lock_guard<std::mutex> lock{linksLock};
    auto connLinkPairIt = connToLink.find(connId);
    if (connLinkPairIt == connToLink.end()) {
        helper::logDebug("attempt to get package handle for non-existent connection: " + connId);
    } else if (linkIdToInfo.find(connLinkPairIt->second) == linkIdToInfo.end()) {
        helper::logInfo("attempt to get package handles for non-existent link: " +
                        connLinkPairIt->second);
    } else if (linkIdToInfo[connLinkPairIt->second].connIdToInfo.find(connId) ==
               linkIdToInfo[connLinkPairIt->second].connIdToInfo.end()) {
        helper::logInfo("attempt to get package handles for non-existent connection: " + connId);
    } else {
        return linkIdToInfo[connLinkPairIt->second].connIdToInfo[connId].packageHandles;
    }
    return {};
}

void RaceLinks::removeCachedPackageHandle(const RaceHandle &packageHandle) {
    std::lock_guard<std::mutex> lock{linksLock};
    auto handleConnPairIt = packageHandleConnectionMap.find(packageHandle);
    if (handleConnPairIt == packageHandleConnectionMap.end()) {
        helper::logDebug("attempt to remove uncached package handle: " +
                         std::to_string(packageHandle));
    } else if (packageHandle == NULL_RACE_HANDLE) {
        helper::logInfo("attempt to remove null package handle");
    } else {
        auto connLinkPairIt = connToLink.find(handleConnPairIt->second);
        if (connLinkPairIt == connToLink.end()) {
            helper::logDebug("attempt to get package handle for non-existent connection: " +
                             handleConnPairIt->second);
        } else if (linkIdToInfo.find(connLinkPairIt->second) == linkIdToInfo.end()) {
            helper::logDebug("attempt to remove uncached linkID " + handleConnPairIt->second);
        } else if (linkIdToInfo[connLinkPairIt->second].connIdToInfo.find(connLinkPairIt->first) ==
                   linkIdToInfo[connLinkPairIt->second].connIdToInfo.end()) {
            helper::logInfo("attempt to get package handles for non-existent connection: " +
                            connLinkPairIt->first);
        } else {
            linkIdToInfo[connLinkPairIt->second]
                .connIdToInfo[connLinkPairIt->first]
                .packageHandles.erase(packageHandle);
            packageHandleConnectionMap.erase(packageHandle);
        }
    }
}
