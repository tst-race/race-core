
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

#include "PluginNMTwoSix.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>

#include "ConfigPersonas.h"
#include "ExtClrMsg.h"
#include "Log.h"
#include "RaceEnums.h"
#include "filesystem.h"
#include "helper.h"

using json = nlohmann::json;

PluginNMTwoSix::PluginNMTwoSix(IRaceSdkNM *sdk, PersonaType personaType) :
    raceSdk(sdk),
    raceUuid(raceSdk->getActivePersona()),
    myPersonaType(personaType),
    linkWizard(raceUuid, myPersonaType, this),
    useLinkWizard(true),
    linkWizardInitialized(false),
    lookbackSeconds(60.0),
    bootstrap(this),
    linkManager(this) {}

/**
 * @brief Try to open receive connections for all links for all uuids. If a link has multiple
 * personas associated with it, only one connection is opened. If a link has no personas associated
 * with it, no connection is opened
 *
 * @param uuids The list of persona UUIDs to open receive connections for
 * @return true if connections for all links for all uuids were opened, false otherwise
 */
bool PluginNMTwoSix::openRecvConns(std::vector<std::string> uuids) {
    TRACE_METHOD();
    std::lock_guard<std::mutex> lock(connectionLock);

    std::set<LinkID> linksToOpen;  // set makes sure we only open each link once

    // open all receive links to each name
    for (std::size_t i = 0; i < uuids.size(); i++) {
        if (raceUuid == uuids[i]) {
            continue;
        }
        logDebug("openRecvConns: opening connection to: " + uuids[i]);

        std::vector<LinkID> potentialLinks = raceSdk->getLinksForPersonas({uuids[i]}, LT_RECV);
        if (potentialLinks.empty()) {
            // Warning if there is no path to receive from persona, Trying to
            // open all possible connections, some might not have links
            logWarning("No links to receive on " + uuids[i]);
        }

        linksToOpen.insert(potentialLinks.begin(), potentialLinks.end());
    }

    for (auto &linkId : linksToOpen) {
        // get link hints to use
        auto props = raceSdk->getLinkProperties(linkId);
        std::vector<std::string> supported_hints = props.supported_hints;
        json linkHints = json::object();
        if (std::count(supported_hints.begin(), supported_hints.end(), "polling_interval_ms")) {
            linkHints["polling_interval_ms"] = 500;
        }

        if (std::count(supported_hints.begin(), supported_hints.end(), "after")) {
            std::chrono::duration<double> sinceEpoch =
                std::chrono::system_clock::now().time_since_epoch();
            double now = sinceEpoch.count() - lookbackSeconds;
            linkHints["after"] = now;
        }

        // Open the connection
        SdkResponse response =
            raceSdk->openConnection(props.linkType, linkId, linkHints.dump(), 0, RACE_UNLIMITED, 0);
        if (response.status != SDK_OK) {
            logError("openRecvConns failed to open LinkID: " + linkId);
            return false;
        }

        // This network manager implementation doesn't care about connection-persona mapping for
        // receive links
        openingConnectionsMap.emplace(response.handle,
                                      std::make_pair(std::vector<std::string>{}, LT_RECV));
    }

    return true;
}

std::vector<uint8_t> PluginNMTwoSix::getAesKeyForSelf() {
    auto it = uuidToPersonaMap.find(raceUuid);
    if (it == uuidToPersonaMap.end()) {
        logError("Failed to find aes key for self: " + raceUuid + ". This is not a valid state");
        throw std::logic_error("Failed to find aes key for self: " + raceUuid);
    }
    return it->second.getAesKey();
}

ExtClrMsg PluginNMTwoSix::parseMsg(const EncPkg &ePkg) {
    TRACE_METHOD();
    auto key = getAesKeyForSelf();

    std::string decryptedPkg = encryptor.decryptEncPkg(ePkg.getCipherText(), key);
    ExtClrMsg parsedMsg("", "", "", 1, 0, 0, UNSET_UUID, UNSET_RING_TTL, 0, MSG_UNDEF, {}, {});
    try {
        parsedMsg = encryptor.parseDelimitedExtMessage(decryptedPkg);
        parsedMsg.setTraceId(ePkg.getTraceId());
        parsedMsg.setSpanId(ePkg.getSpanId());
    } catch (...) {
        logDebug("failed to parse message.");
        return parsedMsg;
    }

    logDebug("MsgType: " + std::to_string(parsedMsg.getMsgType()));
    logDebug("processEncPkg Got Message:");
    logMessage("    Message: ", parsedMsg.getMsg());
    logDebug("    from: " + parsedMsg.getFrom());
    logDebug("    to: " + parsedMsg.getTo());
    logDebug("    timestamp: " + std::to_string(parsedMsg.getTime()));
    logDebug("    nonce: " + std::to_string(parsedMsg.getNonce()));

    return parsedMsg;
}

/**
 * @brief Receive notification about a change to the LinkProperties of a Link
 *
 * @param linkId The LinkdID of the link that has been updated
 * @param linkProperties The LinkProperties that were updated
 * @return PluginResponse the status of the Plugin in response to this call
 */
PluginResponse PluginNMTwoSix::onLinkPropertiesChanged(LinkID linkId,
                                                       LinkProperties /*linkProperties*/) {
    // NOTE TO PERFORMERS: this callback will be triggered to notify you that the link properties
    // have changed. Our stub implementation has no use for this information and simply logs the
    // event. However, depending on the change in properties for a link you may want to adapt, e.g.
    // adjust your routing scheme or priority. Note that the link property information will also be
    // update in subsequent calls to `getLinkProperties()`.
    TRACE_METHOD(linkId);
    return PLUGIN_OK;
}

/**
 * @brief Receive notification about a change in package status. The handle will correspond to a
 * handle returned inside an SdkResponse to a previous sendEncryptedPackage call.
 *
 * @param handle The RaceHandle of the previous sendEncryptedPackage call
 * @param status The PackageStatus of the package updated
 * @return PluginResponse the status of the Plugin in response to this call
 */
PluginResponse PluginNMTwoSix::onPackageStatusChanged(RaceHandle handle, PackageStatus status) {
    TRACE_METHOD(handle, status);

    RaceHandle resendHandle = NULL_RACE_HANDLE;

    if (status == PACKAGE_FAILED_GENERIC or status == PACKAGE_FAILED_NETWORK_ERROR or
        status == PACKAGE_FAILED_TIMEOUT) {
        auto iter = resendMap.find(handle);
        if (iter == resendMap.end()) {
            logError("onPackageStatusChanged: (handle=" + std::to_string(handle) +
                     ") Package failed but we did not have a resend entry");
        } else {
            logError("onPackageStatusChanged: (handle=" + std::to_string(handle) +
                     ") Package failed, reopening and queueing to send");
            const AddressedMsg addrMsg = iter->second;
            resendMap.erase(iter);
            resendHandle = sendFormattedMsg(addrMsg.dst, addrMsg.msg, addrMsg.traceId,
                                            addrMsg.spanId, addrMsg.linkRank + 1);
        }
    } else if (status == PACKAGE_SENT) {  // if link was unreliable this is the final status
        auto iter = resendMap.find(handle);
        if (iter != resendMap.end()) {
            const AddressedMsg addrMsg = iter->second;
            if (!addrMsg.reliable) {
                resendMap.erase(iter);
            }
        }
    } else if (status == PACKAGE_RECEIVED) {
        auto iter = resendMap.find(handle);
        if (iter != resendMap.end()) {
            resendMap.erase(iter);
        }
    } else {
        logWarning("onPackageStatusChanged: (handle=" + std::to_string(handle) +
                   ") received PACKAGE_INVALID status");
    }

    bootstrap.onPackageStatusChanged(handle, status, resendHandle);

    logInfo("onPackageStatusChanged: returned");
    return PLUGIN_OK;
}

/**
 * @brief Try to apply some hints if the link supports them
 *
 * @param properties The LinkProperties of the link
 * @return stringified JSON of the link hints to send
 */
std::string PluginNMTwoSix::tryHints(LinkProperties properties) {
    json linkHints = json::object();
    std::vector<std::string> supported_hints = properties.supported_hints;
    if (std::count(supported_hints.begin(), supported_hints.end(), "batch")) {
        linkHints["batch"] = true;
    }
    if (std::count(supported_hints.begin(), supported_hints.end(), "after")) {
        std::chrono::duration<double> sinceEpoch =
            std::chrono::system_clock::now().time_since_epoch();
        double now = sinceEpoch.count() - lookbackSeconds;
        linkHints["after"] = now;
    }
    return linkHints.dump();
}

RaceHandle PluginNMTwoSix::sendFormattedMsg(const std::string &dstUuid,
                                            const std::string &msgString,
                                            const std::uint64_t traceId,
                                            const std::uint64_t spanId) {
    return sendFormattedMsg(dstUuid, msgString, traceId, spanId, 0);
}

RaceHandle PluginNMTwoSix::sendFormattedMsg(const std::string &dstUuid,
                                            const std::string &msgString,
                                            const std::uint64_t traceId, const std::uint64_t spanId,
                                            const std::size_t linkRank = 0) {
    TRACE_METHOD(dstUuid);
    try {
        ExtClrMsg parsedMsg = encryptor.parseDelimitedExtMessage(msgString);
        logDebug("  sendMsg: msg: " + parsedMsg.getMsg());
        logDebug("           type: " + std::to_string(parsedMsg.getMsgType()));
    } catch (...) {
        logDebug("failed to parse message I am sending.");
    }

    Persona dstPersona;
    try {
        dstPersona = uuidToPersonaMap.at(dstUuid);
    } catch (std::invalid_argument &err) {
        logError("Failed to find destination UUID " + dstUuid + " in uuidToPersonaMap");
        return NULL_RACE_HANDLE;
    }

    EncPkg ePkg(traceId, spanId, encryptor.encryptClrMsg(msgString, dstPersona.getAesKey()));
    logMessageOverhead(msgString, ePkg);

    try {
        auto rankedConns = uuidToConnectionsMap.at(dstUuid);
        if (rankedConns.size() == 0) {
            throw std::out_of_range("rankedConns is empty");
        }
        uint8_t finalLinkRank = linkRank % rankedConns.size();
        ConnectionID connId = rankedConns.at(finalLinkRank).first;
        logDebug("Sending package on " + connId);
        SdkResponse response = raceSdk->sendEncryptedPackage(ePkg, connId, RACE_BATCH_ID_NULL, 0);
        if (response.status != SDK_OK) {
            logError("sendFormattedMsg failed to send: " +
                     std::to_string(static_cast<int>(response.handle)));

            if (linkRank + 1 < rankedConns.size()) {
                logInfo("retrying on next connection");
                return sendFormattedMsg(dstUuid, msgString, traceId, spanId, linkRank + 1);
            }
            return NULL_RACE_HANDLE;
        }
        // get LinkProperties to record whether this is reliable or not
        LinkProperties props = raceSdk->getLinkProperties(raceSdk->getLinkForConnection(connId));
        resendMap.emplace(response.handle, AddressedMsg{dstUuid, msgString, traceId, spanId,
                                                        props.reliable, finalLinkRank});

        return response.handle;
    } catch (const std::out_of_range &) {
        logError("No connection to send to destination: " + dstUuid);
        return NULL_RACE_HANDLE;
    }
}

bool PluginNMTwoSix::sendBootstrapPkg(const ConnectionID &connId, const std::string &dstUuid,
                                      const std::string &msgString) {
    TRACE_METHOD(dstUuid);
    try {
        ExtClrMsg parsedMsg = encryptor.parseDelimitedExtMessage(msgString);
        logDebug("  sendBootstrapMsg: msg: " + parsedMsg.getMsg());
        logDebug("              type: " + std::to_string(parsedMsg.getMsgType()));
    } catch (...) {
        logDebug("failed to parse message I am sending.");
    }

    Persona dstPersona;
    try {
        dstPersona = uuidToPersonaMap.at(dstUuid);
    } catch (std::invalid_argument &err) {
        logError("Failed to find destination UUID " + dstUuid + " in uuidToPersonaMap");
        return false;
    }

    logDebug("Sending package on " + connId);
    SdkResponse response = raceSdk->sendBootstrapPkg(
        connId, raceUuid, encryptor.encryptClrMsg(msgString, dstPersona.getAesKey()), 0);
    if (response.status != SDK_OK) {
        logError("sendFormattedMsg failed to send: ");
        return false;
    }

    // TODO: check if package was successfully sent
    return true;
}

/**
 * @brief Handle a connection open event - update internal mappings for send connections
 *
 * @param handle RaceHandle identifying the openConnection request this is associated with
 * @param connId ConnectionID of the opened connection
 * @param properties LinkProperties of the link the connection is on
 * @return PluginResponse status of the plugin (always PLUGIN_OK)
 */
PluginResponse PluginNMTwoSix::handleConnectionOpened(RaceHandle handle, const ConnectionID &connId,
                                                      const LinkProperties &properties) {
    TRACE_METHOD(handle, connId);

    auto iterConnDetails = openingConnectionsMap.find(handle);
    if (iterConnDetails == openingConnectionsMap.end()) {
        // we were not expecting this connection to be opened
        logWarning("handleConnectionOpened: Unexpected open for conn: " + connId);
        return PLUGIN_OK;
    }

    auto uuidList = iterConnDetails->second.first;
    LinkType connType = iterConnDetails->second.second;
    openingConnectionsMap.erase(iterConnDetails);

    logDebug(logPrefix + "connection opened for " + personasToString(uuidList) + " of type " +
             linkTypeToString(connType));

    if (connType == LT_RECV or connType == LT_BIDI) {
        recvConnectionSet.insert(connId);
        logDebug(logPrefix + "receive connection opened: " + connId);
    }
    if (connType == LT_SEND or connType == LT_BIDI) {  // Handle send-capable links
        if (properties.linkType == LT_SEND and uuidList.empty()) {
            logWarning("Opened LT_SEND connection but no persona was associated with it.");
        } else if (not uuidList.empty()) {
            connectionToUuidMap[connId] = uuidList;
            auto uuidStr = personasToString(uuidList);

            // Get type of node for this persona/group of personas
            PersonaType personaType = P_UNDEF;
            for (const auto &uuid : uuidList) {
                try {
                    Persona persona = uuidToPersonaMap.at(uuid);
                    if (personaType != P_UNDEF and personaType != persona.getPersonaType()) {
                        logError(
                            logPrefix +
                            "Multicast connection has recipients of mixed node types: " + connId);
                        return PLUGIN_ERROR;
                    }
                    personaType = persona.getPersonaType();
                } catch (const std::out_of_range &) {
                    logError(logPrefix + "Could not find persona for UUID: " + uuid);
                    return PLUGIN_ERROR;
                }
            }

            uuidToConnectionsMap.try_emplace(
                uuidStr, std::vector<std::pair<ConnectionID, LinkProperties>>());

            // Updates the ranked connections by-reference
            insertConnection(uuidToConnectionsMap[uuidStr], connId, properties, personaType);
            logDebug(logPrefix + "send opened: " + connId + " to " + uuidStr);
        }
        // if it is a bidirectional link but no associated persona,
        // don't bother adding it to our send connections
    }

    return PLUGIN_OK;
}

/**
 * @brief Handle a connection closed event. Update internal mappings and attempt to open new
 * connections to replace this connection.
 *
 * @param handle RaceHandle identifying the openConnection request this is associated with
 * @param connId ConnectionID of the opened connection
 * @param properties LinkProperties of the link the connection is on
 * @return PluginResponse status of the plugin
 */
PluginResponse PluginNMTwoSix::handleConnectionClosed(RaceHandle handle, const ConnectionID &connId,
                                                      const LinkID &closedLink,
                                                      const LinkProperties &properties) {
    TRACE_METHOD(handle, connId, closedLink);

    // type of this connection (SEND/RECV) more specific than link (could be BIDI)
    LinkType connType;
    std::vector<std::string> uuidList;

    LinkID link = closedLink;
    if (recvConnectionSet.count(connId) > 0) {
        // If it was a recv connection, just try re-opening
        connType = LT_RECV;
        recvConnectionSet.erase(connId);
        if (bootstrap.isBootstrapConnection(connId)) {
            logDebug(logPrefix + "bootstrap connection was closed, no further action needed");
            return PLUGIN_OK;
        }
        logDebug(logPrefix + "receive closed, reopening LinkID " + closedLink);
    } else {
        // If it was _not_ a recv connection, it must be a send: get the persona it was to and find
        // the preferred link to that persona to open
        connType = LT_SEND;
        try {
            uuidList = connectionToUuidMap.at(connId);
            connectionToUuidMap.erase(connId);

            if (bootstrap.isBootstrapConnection(connId)) {
                logDebug(logPrefix + "bootstrap connection was closed, no further action needed");
                return PLUGIN_OK;
            }

            auto uuidStr = personasToString(uuidList);
            // Remove this connId from the map
            auto &connPropsList = uuidToConnectionsMap.at(uuidStr);
            connPropsList.erase(
                std::find_if(connPropsList.begin(), connPropsList.end(),
                             [&](std::pair<ConnectionID, LinkProperties> connProps) {
                                 return connProps.first == connId;
                             }));
            // TODO: if connPropsList.size() == 0 should it be removed from uuidToConnectionsMap?

            // Link availability may have changed, so reselect in case a better one exists
            std::vector<LinkID> potentialLinks = raceSdk->getLinksForPersonas(uuidList, connType);
            // Remove links we already have connections on
            for (auto connProps : uuidToConnectionsMap[uuidStr]) {
                LinkID usedLinks = raceSdk->getLinkForConnection(connProps.first);
                auto usedIdx = std::find(potentialLinks.begin(), potentialLinks.end(), usedLinks);
                if (usedIdx != potentialLinks.end()) {
                    potentialLinks.erase(usedIdx);
                }
            }

            // Get type of node for this persona/group of personas
            PersonaType personaType = P_UNDEF;
            for (const auto &uuid : uuidList) {
                try {
                    Persona persona = uuidToPersonaMap.at(uuid);
                    if (personaType != P_UNDEF and personaType != persona.getPersonaType()) {
                        logError(
                            logPrefix +
                            "Multicast connection has recipients of mixed node types: " + connId);
                        return PLUGIN_ERROR;
                    }
                    personaType = persona.getPersonaType();
                } catch (const std::out_of_range &) {
                    logError(logPrefix + "Could not find persona for UUID: " + uuid);
                    return PLUGIN_ERROR;
                }
            }

            link = getPreferredLinkIdForSendingToPersona(potentialLinks, personaType);
            if (link == "") {
                logWarning(logPrefix + "Could not find a replacement link to reach " + uuidStr);
                return PLUGIN_OK;
            }
        } catch (const std::out_of_range &) {
            logWarning(logPrefix + "Could not find UUID for closed connection: " + connId);
            return PLUGIN_OK;
        }
    }

    logDebug(logPrefix + "opening connection on LinkID: " + link + " to replace " + closedLink);
    // Open the link selected by the above processes
    std::string linkHints = tryHints(properties);
    SdkResponse response = raceSdk->openConnection(connType, link, linkHints, 0, RACE_UNLIMITED, 0);
    if (response.status != SDK_OK) {
        logError(logPrefix + "failed to open LinkID: " + link + " to provide a connection to " +
                 personasToString(uuidList));
    }
    openingConnectionsMap.emplace(response.handle, std::make_pair(uuidList, connType));
    return PLUGIN_OK;
}

/**
 * @brief Receive notification about a change in the status of a connection. The handle will
 * correspond to a handle returned inside an SdkResponse to a previous openConnection or
 * closeConnection call.
 *
 * @param handle The RaceHandle of the original openConnection or closeConnection call, if that
 * is what caused the change. Otherwise, 0
 * @param status The ConnectionStatus of the connection updated
 * @return PluginResponse the status of the Plugin in response to this call
 */
PluginResponse PluginNMTwoSix::onConnectionStatusChanged(RaceHandle handle, ConnectionID connId,
                                                         ConnectionStatus status, LinkID linkId,
                                                         LinkProperties properties) {
    TRACE_METHOD(handle, connId, status);

    PluginResponse response = PLUGIN_OK;
    if (status == CONNECTION_OPEN) {
        response = handleConnectionOpened(handle, connId, properties);

        if (useLinkWizard) {
            for (auto uuid : raceSdk->getPersonasForLink(linkId)) {
                linkWizard.tryQueuedRequests(uuid);
            }
        }
    } else if (status == CONNECTION_CLOSED) {
        response = handleConnectionClosed(handle, connId, linkId, properties);
    } else if (status == CONNECTION_AVAILABLE) {
        logDebug("connection available for connection: " + connId);
        // Handle connection becoming available if necessary
    } else if (status == CONNECTION_UNAVAILABLE) {
        logDebug("connection unavailable for connection: " + connId);
        // Handle connection becoming unavailable if necessary
    }

    if (bootstrap.onConnectionStatusChanged(handle, connId, status, linkId, properties) ==
        PLUGIN_FATAL) {
        return PLUGIN_FATAL;
    }

    if (openingConnectionsMap.empty()) {
        if (!useLinkWizard or linkWizard.numOutstandingRequests() == 0) {
            if (hasNecessaryConnections()) {
                raceSdk->onPluginStatusChanged(PLUGIN_READY);
                raceSdk->displayInfoToUser("network manager is ready", RaceEnums::UD_TOAST);
            } else {
                logDebug(logPrefix + "not ready, waiting for necessary connections");
            }
        } else {
            logDebug(logPrefix + "not ready, waiting for " +
                     std::to_string(linkWizard.numOutstandingRequests()) +
                     " outstanding link wizard requests");
        }
    } else {
        logDebug(logPrefix + "not ready, waiting for " +
                 std::to_string(openingConnectionsMap.size()) + " connections to open");
    }

    return response;
}

/**
 * @brief Notify network manager about a change to the status of a channel
 *
 * @param handle The handle for this callback
 * @param channelGid The name of the channel that changed status
 * @param status The new ChannelStatus of the channel
 * @param properties The ChannelProperties of the channel
 * @return PluginResponse the status of the Plugin in response to this call
 */
PluginResponse PluginNMTwoSix::onChannelStatusChanged(RaceHandle handle, std::string channelGid,
                                                      ChannelStatus status,
                                                      ChannelProperties /*properties*/) {
    TRACE_METHOD(handle, channelGid, status);

    linkManager.onChannelStatusChanged(handle, channelGid, status);

    if (useLinkWizard) {
        linkWizard.handleChannelStatusUpdate(handle, channelGid, status);
    }
    return PLUGIN_OK;
}

/**
 * @brief Notify network manager about a change to the status of a link
 *
 * @param handle The handle for this callback
 * @param channelGid The name of the link that changed status
 * @param status The new LinkStatus of the link
 * @param properties The LinkProperties of the link
 * @return PluginResponse the status of the Plugin in response to this call
 */
PluginResponse PluginNMTwoSix::onLinkStatusChanged(RaceHandle handle, LinkID linkId,
                                                   LinkStatus status, LinkProperties properties) {
    TRACE_METHOD(handle, linkId, status);
    if (useLinkWizard) {
        linkWizard.handleLinkStatusUpdate(handle, linkId, status, properties);
    }

    bootstrap.onLinkStatusChanged(handle, linkId, status, properties);

    if (status == LINK_CREATED || status == LINK_LOADED) {
        openConnectionsForLink(linkId, properties);
    }

    linkManager.onLinkStatusChanged(handle, linkId, status, properties);

    return PLUGIN_OK;
}

void PluginNMTwoSix::onStaticLinksCreated() {
    TRACE_METHOD();
    if (useLinkWizard) {
        if (not linkWizardInitialized) {
            linkWizard.setReadyToRespond(true);
            linkWizardInitialized = true;
        } else {
            linkWizard.resendSupportedChannels();
        }
        invokeLinkWizard(uuidsToSendTo);
    }
}

/**
 * @brief Receive notification about a change to the Links associated with a Persona
 *
 * @param recipientPersona The Persona that has changed link associations
 * @param linkType The LinkType of the links (send, recv, bidi)
 * @param links The list of links that are now associated with this persona
 * @return PluginResponse the status of the Plugin in response to this call
 */
PluginResponse PluginNMTwoSix::onPersonaLinksChanged(std::string recipientPersona,
                                                     LinkType linkType,
                                                     std::vector<LinkID> /*links*/) {
    TRACE_METHOD(recipientPersona, linkType);
    return PLUGIN_OK;
}

PluginResponse PluginNMTwoSix::onUserInputReceived(RaceHandle /*handle*/, bool /*answered*/,
                                                   const std::string & /*response*/) {
    TRACE_METHOD();
    return PLUGIN_OK;
}

PluginResponse PluginNMTwoSix::onUserAcknowledgementReceived(RaceHandle) {
    TRACE_METHOD();
    return PLUGIN_OK;
}

PluginResponse PluginNMTwoSix::notifyEpoch(const std::string &data) {
    TRACE_METHOD(data);
    return PLUGIN_OK;
}

/**
 * @brief Close all receive connections
 *
 * @return void
 */
void PluginNMTwoSix::closeRecvConns() {
    TRACE_METHOD();

    std::lock_guard<std::mutex> lock(connectionLock);
    for (auto &connId : recvConnectionSet) {
        logDebug("Closing Connection: " + connId);
        raceSdk->closeConnection(connId, 0);
    }
    recvConnectionSet.clear();
}

/**
 * @brief Load list of RACE personas from a config file
 *
 * @param configFilePath String path to the config file
 * @return void
 */
void PluginNMTwoSix::loadPersonas(const std::string &configFilePath) {
    TRACE_METHOD(configFilePath);

    ConfigPersonas personasConfig;
    if (!personasConfig.init(*raceSdk, configFilePath)) {
        logError("failed to parse network manager personas config file.");
        throw std::logic_error("failed to parse network manager personas config file.");
    }

    for (size_t index = 0; index < personasConfig.numPersonas(); ++index) {
        Persona currentPersona = personasConfig.getPersona(index);
        uuidToPersonaMap[currentPersona.getRaceUuid()] = currentPersona;
    }

    // Configuring Persona
    raceUuid = raceSdk->getActivePersona();
    if (uuidToPersonaMap.find(raceUuid) == uuidToPersonaMap.end()) {
        // Create persona for this node
        Persona currentPersona;
        currentPersona.setRaceUuid(raceUuid);
        currentPersona.setDisplayName(raceUuid);
        currentPersona.setPersonaType(myPersonaType);
        currentPersona.setAesKeyFile(raceUuid + ".aes");

        std::independent_bits_engine<std::default_random_engine, 8, uint8_t> byte_generator;
        std::vector<uint8_t> key(32);  // 256 bit aes key
        std::generate(key.begin(), key.end(), byte_generator);
        currentPersona.setAesKey(key);

        std::string keyPath = configFilePath + "/" + currentPersona.getAesKeyFile();
        SdkResponse response = raceSdk->writeFile(keyPath, key);
        if (response.status != SDK_OK) {
            logError("Failed to write AES key file: " + keyPath + ": " +
                     sdkStatusToString(response.status));
        }

        personasConfig.addPersona(currentPersona);
        if (!personasConfig.write(*raceSdk, configFilePath)) {
            logError("failed to write network manager personas config file");
            throw std::logic_error("failed to write network manager personas config file");
        }

        uuidToPersonaMap[raceUuid] = currentPersona;
    }
}

bool PluginNMTwoSix::openConnectionsForLink(const LinkID &linkId,
                                            const LinkProperties &properties) {
    TRACE_METHOD(linkId);

    std::vector<std::string> uuidList = raceSdk->getPersonasForLink(linkId);
    if (not uuidList.empty()) {
        logInfo(logPrefix + "Opening LinkID: " + linkId + " for " + personasToString(uuidList));

        std::string linkHints = tryHints(properties);
        SdkResponse response =
            raceSdk->openConnection(properties.linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);
        if (response.status != SDK_OK) {
            logError(logPrefix + "failed to open connection on LinkID: " + linkId);
            return false;
        }

        openingConnectionsMap.emplace(response.handle,
                                      std::make_pair(uuidList, properties.linkType));
    } else {
        logInfo(logPrefix + "No personas associated with LinkID: " + linkId);
    }

    return true;
}

PluginResponse PluginNMTwoSix::shutdown() {
    TRACE_METHOD();
    return PLUGIN_OK;
}

PluginConfig PluginNMTwoSix::getConfigs() {
    return config;
}

std::string PluginNMTwoSix::getJaegerConfigPath() {
    return config.etcDirectory + "/jaeger-config.yml";
}

IRaceSdkNM *PluginNMTwoSix::getSdk() {
    return raceSdk;
}

std::string PluginNMTwoSix::getUuid() {
    return raceUuid;
}

RaceCrypto PluginNMTwoSix::getEncryptor() {
    return encryptor;
}

LinkManager *PluginNMTwoSix::getLinkManager() {
    return &linkManager;
}

/**
 * @brief Log the size of a formatted message and how much overhead is added
 *
 * @param formattedMessage String of the message to send
 * @param package EncPkg wrapped messag configFilePath String path to the config filee
 * @return void
 */
void PluginNMTwoSix::logMessageOverhead(const std::string &formattedMessage,
                                        const EncPkg &package) {
    const size_t messageSizeInBytes = encryptor.getMsgLength(formattedMessage);
    const size_t packageSizeInBytes = package.getRawData().size();
    const size_t overhead = packageSizeInBytes - messageSizeInBytes;

    std::stringstream messageToLog;
    messageToLog << "clear message size: " << messageSizeInBytes << " bytes. ";
    messageToLog << "encrypted package size: " << packageSizeInBytes << " bytes. ";
    messageToLog << "overhead: " << overhead << " bytes.";

    logInfo(messageToLog.str());
}
