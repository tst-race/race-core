
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

#ifndef __SOURCE_RACE_LINKS_H__
#define __SOURCE_RACE_LINKS_H__

#include <LinkProperties.h>  // LinkID, LinkProperties

#include <atomic>         // std::atomic
#include <mutex>          // std::mutex, std::lock_guard
#include <string>         // std::string
#include <unordered_map>  // std::unordered_map
#include <unordered_set>  // std::unordered_map
#include <vector>         // std::vector

#include "PersonaForwardDeclarations.h"
#include "SdkResponse.h"

class RaceLinks {
public:
    /**
     * @brief Constructor.
     *
     */
    RaceLinks();
    virtual ~RaceLinks() = default;

    /**
     * @brief Add a link profile for the link ID.
     *
     * @param linkId the id of the link to add
     * @param personas The personas that can be reached by the link profile.
     * @return None
     */
    virtual void addLink(const std::string &linkId, const personas::PersonaSet &personas);

    /**
     * @brief Remove a link and all its connections
     *
     * @param linkId the id of the link to remove
     * @return None
     */
    virtual void removeLink(const std::string &linkId);

    /**
     * @brief Add a link profile from a previous newLinkRequest
     *
     * @param handle The RaceHandle of the previous createLink/loadLinkAddress request
     * @param linkId the id of the link to add
     * @return address of the link, if it was a load request else empty-string
     */
    virtual const std::string completeNewLinkRequest(RaceHandle handle, const std::string &linkId);

    /**
     * @brief Add a new link request handle and personas for later mapping.
     *
     * @param handle The RaceHandle of the link request
     * @param personas The personas that can be reached by the link profile.
     * @param linkAddress The address of the new link (if being loaded, else empty-string)
     * @return None
     */
    virtual void addNewLinkRequest(RaceHandle handle, const personas::PersonaSet &personas,
                                   const std::string &linkAddress);

    /**
     * @brief Remove a new link request handle
     *
     * @param handle The RaceHandle of the link request
     * @param linkId the id of the link request to remove
     * @return None
     */
    virtual void removeNewLinkRequest(RaceHandle handle, const LinkID &linkId);

    /**
     * @brief Remove a connection request that was not fulfilled
     *
     * @param connId The ConnectionID of the connection to remove
     * @return None
     */
    virtual void removeConnectionRequest(RaceHandle handle);

    /**
     * @brief Remove a Connection
     *
     * @param connId The ConnectionID of the connection to remove
     * @return None
     */
    virtual void removeConnection(const ConnectionID &connId);

    /**
     * @brief Add a connection request to be fulfilled later
     *
     * @param handle The RaceHandle associated with this request
     * @param linkId The LinkID the connection is on
     * @return None
     */
    virtual void addConnectionRequest(RaceHandle handle, const LinkID &linkId);

    /**
     * @brief Fulfill an earlier connection request
     *
     * @param handle The RaceHandle associated with the request being fulfilled
     * @param connId The ConnectionID of the new connection
     * @return None
     */
    virtual void addConnection(RaceHandle handle, const ConnectionID &connId);

    /**
     * @brief Check if a connection currently exists.
     *
     * @param connId The ConnectionID of the connection to check for
     * @return bool true if the ConnectionID is in the datastructures
     */
    virtual bool doesConnectionExist(const ConnectionID &connId);

    /**
     * @brief Check if a set of connections currently exist.
     *
     * @param connectionIds The IDs of the connections to check for existence.
     * @return std::unordered_set<ConnectionID> The connections that do not exist, or an empty set
     * if all connections exist.
     */
    virtual std::unordered_set<ConnectionID> doConnectionsExist(
        const std::unordered_set<ConnectionID> &connectionIds);

    /**
     * @brief Update the properties for an existing link.
     *
     * @param linkId The ID of the link to update.
     * @param properties The new properties for the link.
     */
    virtual void updateLinkProperties(const LinkID &linkId, const LinkProperties &properties);

    /**
     * @brief Get the name of the connection as seen by the comms plugin
     *
     * @param linkId The ID of the link.
     * @return string The name of the plugin the link is from.
     */
    static ConnectionID getPrivateConnectionID(const ConnectionID &connId);

    /**
     * @brief Get the name of the connection as seen by the SDK given
     * the name as seen by the comms plugin
     *
     * @param linkId The ID of the link.
     * @return string The name of the plugin the link is from.
     */
    static ConnectionID getPublicConnectionID(const ConnectionID &connId,
                                              const std::string &plugin);

    /**
     * @brief Get the properties for a link with a given ID.
     *
     * @param linkId The ID of the link to get properties for.
     * @return LinkProperties The properties of the link.
     */
    virtual LinkProperties getLinkProperties(const LinkID &linkId);

    /**
     * @brief Get every unique persona that can be reached by the current set of links.
     *
     * @return PersonaSet All personas that can be reached.
     */
    virtual personas::PersonaSet getAllPersonaSet();

    virtual bool doesLinkIncludeGivenPersonas(const personas::PersonaSet &connectionProfilePersonas,
                                              const personas::PersonaSet &givenPersonas);
    /**
     * @brief Get a vector of IDs for all the links that can each reach all of the given personas.
     *
     * @param personas The personas that need to be reached on the link.
     * @param linkType The type of link (send, receive, bidirectional).
     * @return std::vector<LinkID> A vector of IDs for all the links that can reach all of the given
     * personas.
     */
    virtual std::vector<LinkID> getAllLinksForPersonas(const personas::PersonaSet &personas,
                                                       const LinkType linkType);

    // TODO docs
    virtual bool setPersonasForLink(const std::string &linkId,
                                    const personas::PersonaSet &personas);

    /**
     * @brief Get a vector of personas reachable by the Link
     *
     * @param linkId The LinkID to lookup reachable personas for
     * @return personas::PersonaSet A set of Personas reachable by the Link
     */
    virtual personas::PersonaSet getAllPersonasForLink(const LinkID &linkId);

    /**
     * @brief Get the link for a connection
     *
     * @param connId The ConnectionID to find the Link for
     * @return LinkID of the link this connection uses
     */
    virtual LinkID getLinkForConnection(const ConnectionID &connId);

    /**
     * @brief Get the link id from a connection id
     *
     * @param connId The ID of the connection.
     * @return string The id of the link the connection is on
     */
    static std::string getLinkIDFromConnectionID(const ConnectionID &connId);

    /**
     * @brief Get the plugin from a link id
     *
     * @param linkId The ID of the link.
     * @return string The plugin the link is on
     */
    static std::string getPluginFromLinkID(const LinkID &linkId);

    /**
     * @brief Get the plugin from a connection id
     *
     * @param connId The ID of the connection.
     * @return string The plugin the connection is on
     */
    static std::string getPluginFromConnectionID(const ConnectionID &connId);

    /**
     * @brief Save the traceId and spanId for a given link tracing context
     *
     * @param linkId The ID of the link.
     * @param std::uint64_t traceId The traceId for the tracing context
     * @param std::uint64_t spanId The spanId for the tracing context
     */
    virtual void addTraceCtxForLink(const LinkID &linkId, std::uint64_t traceId,
                                    std::uint64_t spanId);

    /**
     * @brief Get the traceId and spanId for a given link tracing context
     *
     * @param linkId The ID of the link.
     * @return std::pair<std::uint64_t, std::uint64_t> A traceId,spanId pair
     */
    virtual std::pair<std::uint64_t, std::uint64_t> getTraceCtxForLink(const LinkID &linkId);

    /**
     * @brief Save the traceId and spanId for a given connection tracing context
     *
     * @param connId The ID of the connction.
     * @param std::uint64_t traceId The traceId for the tracing context
     * @param std::uint64_t spanId The spanId for the tracing context
     */
    virtual void addTraceCtxForConnection(const ConnectionID &connId, std::uint64_t traceId,
                                          std::uint64_t spanId);

    /**
     * @brief Get the traceId and spanId for a given connection tracing context
     *
     * @param connId The ID of the connection.
     * @return std::pair<std::uint64_t, std::uint64_t> A traceId,spanId pair
     */
    virtual std::pair<std::uint64_t, std::uint64_t> getTraceCtxForConnection(
        const ConnectionID &connId);

    /**
     * @brief Get the connection IDs for a given link
     *
     * @param linkId The string ID of the link
     * @return std::unordered_set<ConnectionID> all link connections
     */
    virtual std::unordered_set<ConnectionID> getLinkConnections(const LinkID &linkId);

    /**
     * @brief Cache the package handle for the given connection
     *        intended to be called when package will be sent before transmission status received
     *
     * @param connId The string ID of the connection
     * @param packageHandle the handle of the package to be sent
     */
    virtual void cachePackageHandle(const ConnectionID &connId, const RaceHandle &packageHandle);

    /**
     * @brief return all package handles for the given connection
     *
     * @param connId The string ID of the connection
     * @param removePendingPackages clears the list of all package handles for the given connection
     * @return std::unordered_set<RaceHandle> all package handles for the given connection
     */
    virtual std::unordered_set<RaceHandle> getCachedPackageHandles(const ConnectionID &connId);

    /**
     * @brief remove the package handle for a given connection
     *        intended to be call upon send completion/failure
     *
     * @param packageHandle the handle of the package
     */
    virtual void removeCachedPackageHandle(const RaceHandle &packageHandle);

protected:
    /**
     * @brief Internal implementation of the public doesConnectionExist function.
     *
     * @param connId The ConnectionID of the connection to check for
     * @return bool true if the connection exists, otherwise false.
     */
    bool doesConnectionExistInternal(const ConnectionID &connId);

    /**
     * @brief Check if a link type is a valid value.
     *
     * @param linkType The link type value to check.
     * @return true The link type is valid.
     * @return false The link type is invalid.
     */
    bool isValidLinkType(const LinkType linkType);

    /**
     * @brief Get the next link ID.
     *
     * @return LinkID The next link ID.
     */
    LinkID getNextLinkID(const std::string &plugin);

private:
    struct ConnectionInfo {
        std::pair<std::uint64_t, std::uint64_t> traceCtx;
        std::unordered_set<RaceHandle> packageHandles;
    };

    struct LinkInfo {
        personas::PersonaSet personas;
        LinkProperties properties;
        std::unordered_map<ConnectionID, ConnectionInfo> connIdToInfo;
        std::pair<std::uint64_t, std::uint64_t> traceCtx;
    };

    std::mutex linksLock;
    std::unordered_map<LinkID, LinkInfo> linkIdToInfo;
    std::unordered_map<LinkID, personas::PersonaSet> destroyedLinkProfiles;
    std::unordered_map<ConnectionID, LinkID> connToLink;  // time-space trade off

    std::unordered_map<RaceHandle, LinkID> handleToLink;
    std::unordered_map<RaceHandle, std::pair<personas::PersonaSet, const std::string>>
        handleToNewLink;
    std::atomic<std::uint64_t> currentLinkId;

    // time-space trade off for removing package handle from connectionPackageHandleMap
    std::unordered_map<RaceHandle, ConnectionID> packageHandleConnectionMap;
};

#endif
