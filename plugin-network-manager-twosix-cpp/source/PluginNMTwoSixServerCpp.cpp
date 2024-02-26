
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

#include "PluginNMTwoSixServerCpp.h"

#include <unistd.h>

#include <chrono>  // std::chrono::seconds
#include <cstring>
#include <ctime>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <random>
#include <set>
#include <stdexcept>
#include <thread>  // std::this_thread::sleep_for

#include "ConfigPersonas.h"
#include "ExtClrMsg.h"
#include "JsonIO.h"
#include "Log.h"
#include "RaceCrypto.h"

#define FULL_FLOODING 0

using json = nlohmann::json;

PluginNMTwoSixServerCpp::PluginNMTwoSixServerCpp(IRaceSdkNM *sdk) : PluginNMTwoSix(sdk, P_SERVER) {
    TRACE_METHOD();
}

PluginNMTwoSixServerCpp::~PluginNMTwoSixServerCpp() {
    TRACE_METHOD();
}

/**
 * @brief Initialize the plugin. Set the RaceSdk object and other prep work
 *        to begin allowing calls from core and other plugins.
 *
 * @param etcDirectory Directory containing testing specific config files
 * @return PluginResponse the status of the Plugin in response to this call
 */
PluginResponse PluginNMTwoSixServerCpp::init(const PluginConfig &pluginConfig) {
    TRACE_METHOD();
    logInfo("etcDirectory: " + pluginConfig.etcDirectory);
    logInfo("loggingDirectory: " + pluginConfig.loggingDirectory);
    logInfo("auxDataDirectory: " + pluginConfig.auxDataDirectory);
    logInfo("tmpDirectory: " + pluginConfig.tmpDirectory);
    logInfo("pluginDirectory: " + pluginConfig.pluginDirectory);

    config = pluginConfig;

    loadPersonas("personas");
    loadConfigs();

    linkManager.init(serverConfig.channelRoles);

    if (useLinkWizard) {
        linkWizard.init();
    }

    raceSdk->onPluginStatusChanged(PLUGIN_NOT_READY);
    return PLUGIN_OK;
}

/**
 * @brief Example function for performer use
 */
void PluginNMTwoSixServerCpp::exampleEncPkgWithoutPrecursor() {
    std::string cipherText = "cipher text";

    // network managers should now only create EncPkgs with the EncPkgs(traceId, spanId, ciphertext)
    // constructor. Utilizing the EncPkg(rawData) constructor will interpret the first 16 bytes as
    // the trace/span Id as these are always sent. If you wish to send EncPkgs that are not derived
    // from a clear message (e.g. heartbeat messages), you should either set the trace/spand id to
    // 0 (indicating no parent span) or utilize a trace/span ID that your plugin may be leveraging.
    EncPkg pkg(0, 0, {cipherText.begin(), cipherText.end()});

    // No difference for send logic
    for (std::size_t i = 0; i < serverConfig.rings.size(); i++) {
        std::vector<LinkID> potentialLinks =
            raceSdk->getLinksForPersonas({serverConfig.rings[i].next}, LT_SEND);
        if (potentialLinks.empty()) {
            // Except if no link is found
            logError("No links to send to " + serverConfig.rings[i].next);
            throw std::length_error("No links to send to " + serverConfig.rings[i].next);
        }

        std::string linkHints = "{}";
        std::vector<std::string> supported_hints =
            raceSdk->getLinkProperties(potentialLinks.front()).supported_hints;
        if (std::count(supported_hints.begin(), supported_hints.end(), "batch")) {
            linkHints = "{\"batch\": true}";
        }
        raceSdk->openConnection(LT_SEND, potentialLinks.front(), linkHints, 0, RACE_UNLIMITED, 0);

        // The code below should occur inside the onConnectionStatusChanged method which will
        // provide a ConnectionID to use
        ConnectionID connId = "";

        raceSdk->sendEncryptedPackage(pkg, connId, RACE_BATCH_ID_NULL, 0);
        raceSdk->closeConnection(connId, 0);
    }
}

/**
 * @brief Use the LinkWizard to request additional links if insufficient links exist
 * for the number of desired connections.
 *
 * @param personas The map of UUID -> Persona to open links to send to.
 * @return false if an error was received from the SDK on openConnection call, otherwise true
 */
bool PluginNMTwoSixServerCpp::invokeLinkWizard(std::unordered_map<std::string, Persona> personas) {
    TRACE_METHOD();

    auto availableChannels = raceSdk->getSupportedChannels();

    std::lock_guard<std::mutex> lock(connectionLock);

    for (auto [uuid, persona] : personas) {
        if (raceUuid == uuid) {
            continue;
        }

        linkWizard.addPersona(persona);

        for (auto &[channelGid, linkSideStr] : serverConfig.expectedLinks[uuid]) {
            LinkSide linkSide = linkSideFromString(linkSideStr);
            if (not linkManager.hasLink({uuid}, LT_SEND, channelGid, linkSide)) {
                if (availableChannels.count(channelGid) != 0) {
                    logInfo(logPrefix + "Invoking the LinkWizard for uuid: " + uuid +
                            ", channel: " + channelGid + ", link side: " + linkSideStr);
                    linkWizard.tryObtainUnicastLink(persona, LT_SEND, channelGid, linkSide);
                } else {
                    logWarning(logPrefix + "Unable to invoke LinkWizard for uuid: " + uuid +
                               ", channel: " + channelGid + " because channel is not available");
                }
            }
        }
    }

    return true;
}

std::vector<std::string> PluginNMTwoSixServerCpp::getExpectedChannels(const std::string &uuid) {
    TRACE_METHOD();

    std::set<std::string> channels;
    for (auto &[channelGid, linkSideStr] : serverConfig.expectedLinks[uuid]) {
        channels.insert(channelGid);
    }

    return {channels.begin(), channels.end()};
}

/**
 * @brief Implementation requirement - throws exception because Servers are not meant to receive
 * ClrMsg's
 *
 * @param handle The RaceHandle for this call
 * @param msg The clear text message to process.
 * @return PluginResponse the status of the Plugin in response to this call
 */
PluginResponse PluginNMTwoSixServerCpp::processClrMsg(RaceHandle /*handle*/, const ClrMsg &msg) {
    TRACE_METHOD();
    logMessage("    Message: :", msg.getMsg());
    logDebug("    from: " + msg.getFrom());
    logDebug("    to: " + msg.getTo());
    logDebug("    timestamp: " + std::to_string(msg.getTime()));
    logDebug("    nonce: " + std::to_string(msg.getNonce()));

    logError("processClrMsg not callable for servers");
    throw std::logic_error("processClrMsg not callable for servers");
    return PLUGIN_ERROR;
}

/**
 * @brief Given an encrypted package, do everything necessary to either display it to the user,
 * forward it (if this is a server), or just read it (if this message was intended for the network
 * manager module).
 *
 * @param handle The RaceHandle for this call
 * @param recEncPkg The encrypted package to process.
 * @param connIDs List of connection IDs that the package may have come in on. Since multiple
 * logical connection IDs can be used for the same physical connection, all logical connection
 * IDs are provided. Note that only one of the connection IDs will actually be associated with
 * this package.
 * @return PluginResponse the status of the Plugin in response to this call
 */
PluginResponse PluginNMTwoSixServerCpp::processEncPkg(
    RaceHandle /*handle*/, const EncPkg &recEncPkg, const std::vector<std::string> & /*connIDs*/) {
    TRACE_METHOD();

    // Try and decrypt and parse the message. If it does not parse, return and do nothing
    ExtClrMsg parsedMsg = parseMsg(recEncPkg);
    if (parsedMsg.getMsg() == "") {
        logInfo("Package Not Decrypted (Not for Me)");
        return PLUGIN_OK;
    }

    // If it's for this node and from a known sender, process the message
    if (parsedMsg.getTo() == raceUuid) {
        if (uuidToPersonaMap.count(parsedMsg.getFrom()) == 0) {
            logWarning("Received message for unknown UUID: " + parsedMsg.getFrom());
            return PLUGIN_OK;
        }
        Persona sender = uuidToPersonaMap[parsedMsg.getFrom()];
        if (parsedMsg.getMsgType() == MSG_LINKS) {
            if (useLinkWizard) {
                linkWizard.processLinkMsg(sender, parsedMsg);
            }
        } else if (parsedMsg.getMsgType() == MSG_BOOTSTRAPPING) {
            bootstrap.onBootstrapMessage(parsedMsg);
        } else {
            // If the messaged was parsed successfully, it should be presented to the app
            raceSdk->presentCleartextMessage(parsedMsg);
        }
        return PLUGIN_OK;
    }

    // If it's not for me, forward it to other nodes
    routeMsg(parsedMsg);
    return PLUGIN_OK;
}

/**
 * @brief Read the configuration files and instantiate data.
 *
 * @param configFilePath The directory path to where config files are located.
 */
void PluginNMTwoSixServerCpp::loadConfigs() {
    TRACE_METHOD();

    if (!loadServerConfig(*raceSdk, serverConfig)) {
        throw std::logic_error("failed to parse network manager config file.");
    }

    // Build the uuidsToSendTo map
    for (auto uuid : serverConfig.exitClients) {
        uuidsToSendTo[uuid] = uuidToPersonaMap.at(uuid);
    }
    for (auto uuid : serverConfig.committeeClients) {
        uuidsToSendTo[uuid] = uuidToPersonaMap.at(uuid);
    }
    for (auto committeeToUuidList : serverConfig.reachableCommittees) {
        for (auto uuid : committeeToUuidList.second) {
            uuidsToSendTo[uuid] = uuidToPersonaMap.at(uuid);
        }
    }
    for (auto ring : serverConfig.rings) {
        uuidsToSendTo[ring.next] = uuidToPersonaMap.at(ring.next);
    }
    for (auto uuid : serverConfig.otherConnections) {
        uuidsToSendTo[uuid] = uuidToPersonaMap.at(uuid);
    }

    // Set base config values
    useLinkWizard = serverConfig.useLinkWizard;
    lookbackSeconds = serverConfig.lookbackSeconds;

    // Log parsed configuration
    nlohmann::json jsonConfig = serverConfig;
    logDebug(logPrefix + "server config: " + jsonConfig.dump(4));
}

/**
 * @brief Get the preferred link (based on transmission type) for sending to a type of persona,
 *        i.e. client or server.
 *
 * @param potentialLinks The potential links for connecting to the persona.
 * @param recipientPersonaType The type of persona being sent to.
 * @return LinkID The ID of the preferred link, or empty string on error.
 */
LinkID PluginNMTwoSixServerCpp::getPreferredLinkIdForSendingToPersona(
    const std::vector<LinkID> &potentialLinks, const PersonaType recipientPersonaType) {
    TRACE_METHOD();
    LinkProperties bestProps;
    LinkID bestLinkId;
    auto rankLinkPropertiesForPersonaType = std::bind(rankLinkProperties, std::placeholders::_1,
                                                      std::placeholders::_2, recipientPersonaType);
    for (LinkID linkId : potentialLinks) {
        LinkProperties props = raceSdk->getLinkProperties(linkId);
        if (rankLinkPropertiesForPersonaType(props, bestProps)) {
            bestLinkId = linkId;
            bestProps = props;
        }
    }
    if (bestProps.connectionType == CT_UNDEF) {
        logDebug(logPrefix + "No CT_INDIRECT or CT_DIRECT links found");
        bestLinkId = "";
    }
    logDebug(logPrefix + "returned link ID: " + bestLinkId);
    return bestLinkId;
}

bool PluginNMTwoSixServerCpp::hasNecessaryConnections() {
    bool ringConnections = true;
    for (const RingEntry &ring : serverConfig.rings) {
        if (uuidToConnectionsMap.count(ring.next) == 0 or
            uuidToConnectionsMap[ring.next].size() == 0) {
            logDebug("hasNecessaryConnections: no connections to " + ring.next);
            ringConnections = false;
            break;
        }
    }
    return ringConnections;
}

/**
 * @brief Insert and re-sort a connection in the list of send connections for a UUID. Chooses based
 * on rankConnProps ordering.
 *
 * @param rankedConnections Sorted list of connections to modify-by-reference
 * @param newConn ConnectionID of the new connection
 * @param newProps Linkproperties of the new connection
 * @param recipientPersonaType PersonaType of the destination of the connection
 * @return void
 */
void PluginNMTwoSixServerCpp::insertConnection(
    std::vector<std::pair<ConnectionID, LinkProperties>> &rankedConnections,
    const ConnectionID &newConn, const LinkProperties &newProps,
    const PersonaType recipientPersonaType) {
    rankedConnections.emplace_back(std::pair<ConnectionID, LinkProperties>{newConn, newProps});
    auto rankConnPropsForPersonaType = std::bind(rankConnProps, std::placeholders::_1,
                                                 std::placeholders::_2, recipientPersonaType);
    std::sort(rankedConnections.begin(), rankedConnections.end(), rankConnPropsForPersonaType);
}

/**
 * @brief Comparator for two pairs of ConnectID, LinkProperties pairs. Chooses based on
 * rankLinkProperties of their LinkProperties and recipientPersonaType
 *
 * @param pair1 pair<ConnectionID, LinkProperties> of first connection
 * @param pair2 pair<ConnectionID, LinkProperties> of second connection
 * @param recipientPersonaType PersonaType of the destination of the connection
 * @return true if pair1 is higher-priority than pair2, false otherwise
 */
bool PluginNMTwoSixServerCpp::rankConnProps(const std::pair<ConnectionID, LinkProperties> &pair1,
                                            const std::pair<ConnectionID, LinkProperties> &pair2,
                                            const PersonaType recipientPersonaType) {
    return rankLinkProperties(pair1.second, pair2.second, recipientPersonaType);
}

/**
 * @brief Comparator for two LinkProperties.
 * For client destinations it prefers CT_INDIRECT, then not CT_UNDEF, then expected send bandwidth.
 * For server destinations, it prefers not CT_UNDEF, then expected send bandwidth.
 *
 * @param prop1 LinkProperties of first connection
 * @param prop2 LinkProperties of second connection
 * @param recipientPersonaType PersonaType of the destination of the connection
 * @return true if prop1 is higher-priority than prop2, false otherwise
 */
bool PluginNMTwoSixServerCpp::rankLinkProperties(const LinkProperties &prop1,
                                                 const LinkProperties &prop2,
                                                 const PersonaType recipientPersonaType) {
    if (prop1.connectionType == CT_UNDEF) {
        return false;
    }
    if (prop2.connectionType == CT_UNDEF) {
        return true;
    }
    if (recipientPersonaType == P_CLIENT) {
        // prefer indirect for clients
        if (prop1.connectionType == CT_INDIRECT and prop2.connectionType != CT_INDIRECT) {
            return true;
        }
        if (prop2.connectionType == CT_INDIRECT and prop1.connectionType != CT_INDIRECT) {
            return false;
        }
    }
    return prop1.expected.send.bandwidth_bps > prop2.expected.send.bandwidth_bps;
}

/**
 * @brief Adds the UUID to the staleUuids structure; trims 10% of the oldest UUIDs if the size
 * exceeeds maxStaleUuids
 *
 * @param uuid The MsgUuid uuid to add to staleUuids
 */
void PluginNMTwoSixServerCpp::addStaleUuid(MsgUuid uuid) {
    TRACE_METHOD(uuid);
    if (staleUuids.size() > serverConfig.maxStaleUuids) {
        logDebug("    trimming staleUuids from " + std::to_string(staleUuids.size()));
        std::int32_t trimEnd = (serverConfig.maxStaleUuids / 10) + 1;
        auto end = staleUuids.begin();
        std::advance(end, trimEnd);
        staleUuids.erase(staleUuids.begin(), end);
        logDebug("    trimmed staleUuids to " + std::to_string(staleUuids.size()));
    }
    if (uuid != UNSET_UUID) {
        staleUuids.push_back(uuid);
        logDebug("  addStaleUuid: returned");
    }
}

/**
 * @brief Adds the UUID to the floodedUuids structure; trims 10% of the oldest UUIDs if the size
 * exceeeds maxFloodedUuids.
 *
 * @param uuid The MsgUuid uuid to add to floodedUuids
 */
void PluginNMTwoSixServerCpp::addFloodedUuid(MsgUuid uuid) {
    TRACE_METHOD(uuid);
    if (floodedUuids.size() > serverConfig.maxFloodedUuids) {
        logDebug("    trimming floodedUuids from " + std::to_string(floodedUuids.size()));
        std::int32_t trimEnd = (serverConfig.maxStaleUuids / 10) + 1;
        auto end = floodedUuids.begin();
        std::advance(end, trimEnd);
        floodedUuids.erase(floodedUuids.begin(), end);
        logDebug("    trimmed to " + std::to_string(floodedUuids.size()));
    }
    if (uuid != UNSET_UUID) {
        floodedUuids.push_back(uuid);
        logDebug("  addFloodedUuid: returned");
    }
}

/**
 * ROUTING METHODS
 **/
/**
 * @brief Determines whether the msg should cause a new committee ring msg or be forwarded to the
 * client / other committees.
 *
 * @param msg The msg to process
 */
void PluginNMTwoSixServerCpp::routeMsg(ExtClrMsg &msg) {
    TRACE_METHOD();
    // does msg have a Ring-TTL?
    if (!msg.isRingTtlSet() and serverConfig.rings.size() > 0) {  // escape for size-1 committees
        startRingMsg(msg);
    } else {
        handleRingMsg(msg);
    }
}

/**
 * @brief Checks if the msg has already been seen by this node; if not, calls sendToRings, else
 * drops
 *
 * @param msg The ExtClrMsg to process
 */
void PluginNMTwoSixServerCpp::startRingMsg(const ExtClrMsg &msg) {
    TRACE_METHOD();
    if (staleUuids.get<unique>().count(msg.getUuid()) > 0) {
        // already saw this msg previously
        logInfo("Received additional copy of msg with uuid=" + std::to_string(msg.getUuid()));
        return;
    }

    addStaleUuid(msg.getUuid());
    sendToRings(msg);
}

/**
 * @brief Sends the msg out on each ring this node knows about, setting the ringTtl and ringIdx
 * variables appropriately for each.
 *
 * @param msg The ExtClrMsg to process
 */
void PluginNMTwoSixServerCpp::sendToRings(const ExtClrMsg &msg) {
    TRACE_METHOD();
    if (msg.isRingTtlSet()) {
        logError("Attempted to append a second Ring-TTL message, bad logic, ignoring");
        return;
    }
    int32_t idx = 0;
    for (auto ring : serverConfig.rings) {
        logDebug("      sending along ring of length " + std::to_string(ring.length) + " to " +
                 ring.next);
        ExtClrMsg ringMsg = msg.copy();
        // logDebug("Copied message, copy's traceId: " + std::to_string(ringMsg.getTraceId()));
        ringMsg.setRingTtl(ring.length - 1);  // so that it ttl=0 when it reaches me
        ringMsg.setRingIdx(idx);
        sendMsg(ring.next, ringMsg);
        idx++;
    }
}

/**
 * @brief Handles a received ring msg to either forward it along the ring (if ringTtl > 0), forward
 * to other committees, or forward to a client.
 *
 * @param msg The msg to process
 */
void PluginNMTwoSixServerCpp::handleRingMsg(ExtClrMsg &msg) {
    TRACE_METHOD(msg.getUuid(), msg.getRingTtl());
    addStaleUuid(msg.getUuid());
    // NOTE: we do not abort on a repeated uuid in a ring message to allow
    // multiple/redundant ring paths but we DO want to filter out the same msg arriving
    // from outside the committee so the uuid is added to stale
    if (msg.getRingTtl() > 0) {
        // continue the trip around the ring
        msg.decRingTtl();
        // get the nextNode entry for this right ring
        sendMsg(serverConfig.rings.at(static_cast<size_t>(msg.getRingIdx())).next, msg);
    } else if (floodedUuids.get<unique>().count(msg.getUuid()) == 0) {
        addFloodedUuid(msg.getUuid());
        std::string dst_client = msg.getTo();
        if (serverConfig.exitClients.count(dst_client) > 0) {
            logDebug("    client is in exitClients, forwarding to: " + dst_client);
            sendMsg(dst_client, msg.asClrMsg());
        }
        if (serverConfig.committeeClients.count(dst_client) > 0 and serverConfig.rings.size() > 0) {
            // someone in our committee can reach it, send it around this ring
            logDebug("    client is in committeeClients, forwarding around this ring");
            sendMsg(serverConfig.rings[static_cast<size_t>(msg.getRingIdx())].next, msg);
        } else {
            forwardToNewCommittees(msg);
        }
    } else {
        logInfo("    received end-of-ring msg we have already dealt with, ignoring.");
    }
}

/**
 * @brief Resets ringTtl and ringIdx, appends this committee to committeesVisited, and sends to some
 * committees this node knows about. If this node knows about fewer than floodingFactor committees,
 * it _also_ forwards the msg along its rings with ringTtl=0 to prompt them to forward.
 *
 * @param msg The ExtClrMsg to process
 */
void PluginNMTwoSixServerCpp::forwardToNewCommittees(ExtClrMsg &msg) {
    TRACE_METHOD();
    std::vector<std::string> visited = msg.getCommitteesVisited();
    if (std::find(visited.begin(), visited.end(), serverConfig.committeeName) == visited.end()) {
        msg.addCommitteeVisited(serverConfig.committeeName);
    }

    ExtClrMsg intercomMsg = msg.copy();
    intercomMsg.unsetRingTtl();
    intercomMsg.clearCommitteesSent();
    std::unordered_set<std::string> intercom_dsts;
    std::vector<std::string> sent = msg.getCommitteesSent();
    for (auto committee : serverConfig.reachableCommittees) {
        if ((std::find(visited.begin(), visited.end(), committee.first) != visited.end()) or
            (std::find(sent.begin(), sent.end(), committee.first) != sent.end())) {
            continue;
        }
        std::vector<std::string> reachable = committee.second;
        intercom_dsts.insert(reachable[0]);  // TODO handle multiple reachable members
        msg.addCommitteeSent(committee.first);
        if (serverConfig.floodingFactor != FULL_FLOODING and
            intercom_dsts.size() >= serverConfig.floodingFactor) {
            break;
        }
    }

    logDebug("        forwarding to " + std::to_string(intercom_dsts.size()));
    for (auto dst : intercom_dsts) {
        sendMsg(dst, intercomMsg);
    }

    // Special-case of floodingFactor <= 0 --> flood to every committee our _committee_ can reach
    if (serverConfig.floodingFactor == FULL_FLOODING or
        intercom_dsts.size() + sent.size() < serverConfig.floodingFactor) {
        // if _this_ node cannot reach enough other committees, forward on the ring
        logDebug("        sent to " + std::to_string(intercom_dsts.size() + sent.size()) +
                 " other committees but floodingFactor set to " +
                 std::to_string(serverConfig.floodingFactor) +
                 ", forwarding on ring for additional sends");
        if (serverConfig.rings.size() >
            0) {  // Just send on one ring to avoid exceeding floodingFactor
            sendMsg(serverConfig.rings[0].next, msg);
        }
    }
}

/**
 * @brief Packs an ExtClrMsg into a string to call sendMsg(..., std::string &msgString, ...) on
 *
 * @param dstUuid The UUID of the persona to send to
 * @param msg The ExtClrMsg to process
 */
void PluginNMTwoSixServerCpp::sendMsg(const std::string &dstUuid, const ExtClrMsg &msg) {
    std::string formattedMsg = encryptor.formatDelimitedMessage(msg);
    sendFormattedMsg(dstUuid, formattedMsg, msg.getTraceId(), msg.getSpanId(), BEST_LINK);
}

/**
 * @brief Packs a ClrMsg into a string to call sendFormattedMsg(..., std::string &msgString, ...) on
 *
 * @param dstUuid The UUID of the persona to send to
 * @param msg The ClrMsg to process
 */
RaceHandle PluginNMTwoSixServerCpp::sendMsg(const std::string &dstUuid, const ClrMsg &msg) {
    std::string formattedMsg = encryptor.formatDelimitedMessage(msg);
    return sendFormattedMsg(dstUuid, formattedMsg, msg.getTraceId(), msg.getSpanId(), BEST_LINK);
}

PluginResponse PluginNMTwoSixServerCpp::prepareToBootstrap(RaceHandle /*handle*/, LinkID /*linkId*/,
                                                           std::string /*configPath*/,
                                                           DeviceInfo /*deviceInfo*/) {
    logError("prepareToBootstrap: unsupported on servers");
    return PLUGIN_ERROR;
}

PluginResponse PluginNMTwoSixServerCpp::onBootstrapPkgReceived(std::string /*persona*/,
                                                               RawData /*pkg*/) {
    logError("onBootstrapPkgReceived: unsupported on servers");
    return PLUGIN_ERROR;
}

void PluginNMTwoSixServerCpp::writeConfigs() {
    TRACE_METHOD();

    serverConfig.bootstrapHandle = 0;
    serverConfig.bootstrapIntroducer = "";

    if (!writeServerConfig(*raceSdk, serverConfig)) {
        logError(logPrefix + "Failed to write network manager config file");
    }
}

void PluginNMTwoSixServerCpp::addClient(const std::string &persona, const RawData &key) {
    TRACE_METHOD(persona);

    Persona client;

    client.setAesKey(key);
    client.setAesKeyFile(persona + ".aes");
    client.setRaceUuid(persona);
    client.setPersonaType(P_CLIENT);
    client.setDisplayName(persona);

    uuidToPersonaMap[client.getRaceUuid()] = client;

    serverConfig.exitClients.emplace(persona);
    serverConfig.committeeClients.emplace(persona);
    if (!writeServerConfig(*raceSdk, serverConfig)) {
        logError(logPrefix + "Failed to write network manager config file");
    }

    std::string keyPath = "personas/" + client.getAesKeyFile();
    SdkResponse response = raceSdk->writeFile(keyPath, key);
    if (response.status != SDK_OK) {
        logError(logPrefix + "Failed to write AES key file: " + keyPath + ": " +
                 sdkStatusToString(response.status));
    }

    ConfigPersonas personasConfig;
    for (auto &entry : uuidToPersonaMap) {
        personasConfig.addPersona(entry.second);
    }

    if (!personasConfig.write(*raceSdk, "personas")) {
        logError(logPrefix + "Failed to write network manager personas config file");
    }
}

#ifndef TESTBUILD
IRacePluginNM *createPluginNM(IRaceSdkNM *sdk) {
    return new PluginNMTwoSixServerCpp(sdk);
}

void destroyPluginNM(IRacePluginNM *plugin) {
    delete static_cast<PluginNMTwoSixServerCpp *>(plugin);
}

const RaceVersionInfo raceVersion = RACE_VERSION;
const char *const racePluginId = "PluginNMTwoSixStub";
const char *const racePluginDescription =
    "Plugin Network Manager Server Stub (Two Six Labs) " BUILD_VERSION;
#endif
