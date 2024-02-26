
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

#include "PluginNMTwoSixClientCpp.h"

#include <unistd.h>

#include <chrono>  // std::chrono::seconds
#include <cstring>
#include <ctime>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <thread>  // std::this_thread::sleep_for

#include "ConfigNMTwoSix.h"
#include "ExtClrMsg.h"
#include "Log.h"
#include "RaceCrypto.h"
#include "filesystem.h"
#include "helper.h"

PluginNMTwoSixClientCpp::PluginNMTwoSixClientCpp(IRaceSdkNM *sdk) : PluginNMTwoSix(sdk, P_CLIENT) {
    TRACE_METHOD();
}

PluginNMTwoSixClientCpp::~PluginNMTwoSixClientCpp() {
    TRACE_METHOD();
}

PluginResponse PluginNMTwoSixClientCpp::init(const PluginConfig &pluginConfig) {
    TRACE_METHOD();
    logInfo("etcDirectory: " + pluginConfig.etcDirectory);
    logInfo("loggingDirectory: " + pluginConfig.loggingDirectory);
    logInfo("auxDataDirectory: " + pluginConfig.auxDataDirectory);
    logInfo("tmpDirectory: " + pluginConfig.tmpDirectory);
    logInfo("pluginDirectory: " + pluginConfig.pluginDirectory);

    config = pluginConfig;

    loadPersonas("personas");
    loadConfigs();

    if (!clientConfig.bootstrapIntroducer.empty()) {
        bootstrap.onBootstrapStart(clientConfig.bootstrapIntroducer, clientConfig.entranceCommittee,
                                   clientConfig.bootstrapHandle);
    }

    linkManager.init(clientConfig.channelRoles);

    if (useLinkWizard) {
        linkWizard.init();
    }

    raceSdk->onPluginStatusChanged(PLUGIN_NOT_READY);
    return PLUGIN_OK;
}

/**
 * @brief Use the LinkWizard to request additional links if insufficient links exist
 * for the number of desired connections.
 *
 * @return false if an error was received from the SDK on openConnection call, otherwise true
 */
bool PluginNMTwoSixClientCpp::invokeLinkWizard(std::unordered_map<std::string, Persona> personas) {
    TRACE_METHOD();

    auto availableChannels = raceSdk->getSupportedChannels();

    std::lock_guard<std::mutex> lock(connectionLock);
    for (auto [uuid, persona] : personas) {
        // Never create connections to other clients
        if (raceUuid == uuid or persona.getPersonaType() == P_CLIENT) {
            continue;
        }

        linkWizard.addPersona(persona);

        for (auto &[channelGid, linkSideStr] : clientConfig.expectedLinks[uuid]) {
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

    for (auto &expectedMulticastLink : clientConfig.expectedMulticastLinks) {
        std::vector<Persona> personaList;
        for (const auto &uuid : expectedMulticastLink.personas) {
            auto iter = personas.find(uuid);
            if (iter != personas.end()) {
                personaList.push_back(iter->second);
            } else {
                logWarning(logPrefix + "No persona found for uuid " + uuid +
                           " in expected multicast link");
            }
        }

        if (personaList.size() == expectedMulticastLink.personas.size()) {
            auto channelGid = expectedMulticastLink.channelGid;
            LinkSide linkSide = linkSideFromString(expectedMulticastLink.linkSide);
            if (not linkManager.hasLink(expectedMulticastLink.personas, LT_SEND, channelGid,
                                        linkSide)) {
                if (availableChannels.count(channelGid) != 0) {
                    logInfo(logPrefix + "Invoking the LinkWizard for uuids: " +
                            personasToString(expectedMulticastLink.personas) + ", channel: " +
                            channelGid + ", link side: " + expectedMulticastLink.linkSide);
                    linkWizard.tryObtainMulticastSend(personaList, LT_SEND, channelGid, linkSide);
                } else {
                    logWarning(logPrefix + "Unable to invoke LinkWizard for uuids: " +
                               personasToString(expectedMulticastLink.personas) +
                               ", channel: " + channelGid + " because channel is not available");
                }
            }
        }
    }

    return true;
}

std::vector<std::string> PluginNMTwoSixClientCpp::getExpectedChannels(const std::string &uuid) {
    TRACE_METHOD();

    std::set<std::string> channels;
    for (auto &[channelGid, linkSideStr] : clientConfig.expectedLinks[uuid]) {
        channels.insert(channelGid);
    }

    return {channels.begin(), channels.end()};
}

LinkID PluginNMTwoSixClientCpp::getPreferredLinkIdForSendingToPersona(
    const std::vector<LinkID> &potentialLinks, const PersonaType /*recipientPersonaType*/) {
    LinkProperties bestProps;
    LinkID bestLinkId = "";
    for (LinkID linkId : potentialLinks) {
        LinkProperties props = raceSdk->getLinkProperties(linkId);
        if (rankLinkProperties(props, bestProps)) {
            bestLinkId = linkId;
            bestProps = props;
        }
    }

    if (bestProps.connectionType == CT_UNDEF) {
        logDebug("getPreferredLinkIdForSendingToPersona: No CT_INDIRECT or CT_DIRECT links found");
        return "";
    }
    return bestLinkId;
}

bool PluginNMTwoSixClientCpp::hasNecessaryConnections() {
    bool sendConnection = false;
    for (auto server : clientConfig.entranceCommittee) {
        // All we need is a sending connection to any 1 entrance committee server
        logDebug("present: " + std::to_string(uuidToConnectionsMap.count(server)));
        if (uuidToConnectionsMap.count(server) > 0 and uuidToConnectionsMap[server].size() > 0) {
            sendConnection = true;
            break;
        }
    }

    logDebug("hasNecessaryConnections: send connection? " + std::to_string(sendConnection) +
             ", receive connections?" + std::to_string(recvConnectionSet.size()));

    // Just ensure we have at least 1 receive connection
    return sendConnection and recvConnectionSet.size() > 0;
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
void PluginNMTwoSixClientCpp::insertConnection(
    std::vector<std::pair<ConnectionID, LinkProperties>> &rankedConnections,
    const ConnectionID &newConn, const LinkProperties &newProps,
    const PersonaType /*recipientPersonaType*/) {
    rankedConnections.emplace_back(std::pair<ConnectionID, LinkProperties>{newConn, newProps});
    std::sort(rankedConnections.begin(), rankedConnections.end(), rankConnProps);
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
bool PluginNMTwoSixClientCpp::rankConnProps(const std::pair<ConnectionID, LinkProperties> &pair1,
                                            const std::pair<ConnectionID, LinkProperties> &pair2) {
    return rankLinkProperties(pair1.second, pair2.second);
}

/**
 * @brief Comparator for two LinkProperties - prefers CT_INDIRECT, then not CT_UNDEF, then expected
 * send bandwidth
 *
 * @param prop1 LinkProperties of first connection
 * @param prop2 LinkProperties of second connection
 * @param recipientPersonaType PersonaType of the destination of the connection
 * @return true if prop1 is higher-priority than prop2, false otherwise
 */
bool PluginNMTwoSixClientCpp::rankLinkProperties(const LinkProperties &prop1,
                                                 const LinkProperties &prop2) {
    if (prop1.connectionType == CT_UNDEF) {
        return false;
    }
    if (prop2.connectionType == CT_UNDEF) {
        return true;
    }
    if (prop1.connectionType == CT_INDIRECT and prop2.connectionType != CT_INDIRECT) {
        return true;
    }
    if (prop2.connectionType == CT_INDIRECT and prop1.connectionType != CT_INDIRECT) {
        return false;
    }

    return prop1.expected.send.bandwidth_bps > prop2.expected.send.bandwidth_bps;
}

void PluginNMTwoSixClientCpp::addSeenMessage(MsgHash hash) {
    TRACE_METHOD();
    if (seenMessages.size() > clientConfig.maxSeenMessages) {
        logDebug("    trimming seenMessages from " + std::to_string(seenMessages.size()));
        std::int32_t trimEnd = (clientConfig.maxSeenMessages / 10) + 1;
        auto end = seenMessages.begin();
        std::advance(end, trimEnd);
        seenMessages.erase(seenMessages.begin(), end);
        logDebug("    trimmed seenMessages to " + std::to_string(seenMessages.size()));
    }
    seenMessages.push_back(hash);
}

PluginResponse PluginNMTwoSixClientCpp::processClrMsg(RaceHandle handle, const ClrMsg &msg) {
    TRACE_METHOD();
    logMessage("    Message: ", msg.getMsg());
    logDebug("    from: " + msg.getFrom());
    logDebug("    to: " + msg.getTo());
    logDebug("    timestamp: " + std::to_string(msg.getTime()));
    logDebug("    nonce: " + std::to_string(msg.getNonce()));

    if (msg.getTo() == raceUuid) {
        logInfo("I am persona: " + msg.getTo() + ", no need to send on RACE network");
        raceSdk->presentCleartextMessage(msg);
        return PLUGIN_OK;
    }

    MsgHash md = encryptor.getMessageHash(msg);
    if (seenMessages.get<unique>().count(md) == 0) {
        addSeenMessage(md);
    } else {
        logError("new ClrMsg is identical to previously sent message");
        throw std::logic_error("New ClrMsg identical to previous ClrMsg");
    }

    LinkType linkType = LT_SEND;
    bool anySent = sendMulticastMsg(clientConfig.entranceCommittee, msg);
    if (not anySent) {
        for (const std::string &entranceCommitteeMember : clientConfig.entranceCommittee) {
            RaceHandle encPkgHandle = sendMsg(entranceCommitteeMember, msg);
            anySent = raceSdk->getLinksForPersonas({entranceCommitteeMember}, linkType).size() > 0;
            messageStatusTracker.addEncPkgHandleForClrMsg(encPkgHandle, handle);
        }
    }

    if (!anySent) {
        logError("No valid links to any entrance committee members found");
        throw std::length_error("No valid links to any entrance committee members found");
    }

    return PLUGIN_OK;
}

PluginResponse PluginNMTwoSixClientCpp::handleReceivedMsg(const ExtClrMsg &parsedMsg) {
    MsgHash md = encryptor.getMessageHash(parsedMsg);
    if (seenMessages.get<unique>().count(md) == 0) {
        addSeenMessage(md);
    } else {
        logInfo("Package duplicate to one already seen. Ignoring");
        return PLUGIN_OK;
    }

    // If the messaged was parsed successully, it should be presented to the client
    raceSdk->presentCleartextMessage(parsedMsg);
    return PLUGIN_OK;
}

PluginResponse PluginNMTwoSixClientCpp::processEncPkg(
    RaceHandle /*handle*/, const EncPkg &ePkg, const std::vector<std::string> & /*connIDs*/) {
    TRACE_METHOD();

    // Try and decrypt and parse the message. If it does not parse, return and do nothing
    ExtClrMsg parsedMsg = parseMsg(ePkg);
    if (parsedMsg.getMsg() == "") {
        logInfo("Package Not Decrypted (Not for Me)");
        return PLUGIN_OK;
    }

    PluginResponse response;
    if (parsedMsg.getMsgType() == MSG_LINKS) {
        if (useLinkWizard) {
            linkWizard.processLinkMsg(uuidToPersonaMap.at(parsedMsg.getFrom()), parsedMsg);
        }
        response = PLUGIN_OK;
    } else if (parsedMsg.getMsgType() == MSG_CLIENT) {
        response = handleReceivedMsg(parsedMsg);
    } else if (parsedMsg.getMsgType() == MSG_BOOTSTRAPPING) {
        response = bootstrap.onBootstrapMessage(parsedMsg);
    } else {
        logError("Message has undefined message type");
        response = PLUGIN_ERROR;
    }
    return response;
}

/**
 * @brief Read the plugin configuration file
 */
void PluginNMTwoSixClientCpp::loadConfigs() {
    TRACE_METHOD();

    if (!loadClientConfig(*raceSdk, clientConfig)) {
        throw std::logic_error("failed to parse network manager config file.");
    }

    // Build the uuidsToSendTo map
    for (auto uuid : clientConfig.entranceCommittee) {
        uuidsToSendTo[uuid] = uuidToPersonaMap.at(uuid);
    }
    for (auto uuid : clientConfig.exitCommittee) {
        uuidsToSendTo[uuid] = uuidToPersonaMap.at(uuid);
    }
    for (auto uuid : clientConfig.otherConnections) {
        uuidsToSendTo[uuid] = uuidToPersonaMap.at(uuid);
    }

    // Set base config values
    useLinkWizard = clientConfig.useLinkWizard;
    lookbackSeconds = clientConfig.lookbackSeconds;

    // Log parsed configuration
    nlohmann::json jsonConfig = clientConfig;
    logDebug(logPrefix + "client config: " + jsonConfig.dump(4));
}

bool PluginNMTwoSixClientCpp::sendMulticastMsg(const std::vector<std::string> &uuidList,
                                               const ClrMsg &msg, const std::size_t linkRank) {
    auto uuidStr = personasToString(uuidList);
    TRACE_METHOD(uuidStr, msg.getMsg());

    std::string formattedMsg = encryptor.formatDelimitedMessage(msg);

    try {
        auto rankedConns = uuidToConnectionsMap.at(uuidStr);
        if (rankedConns.size() == 0) {
            throw std::out_of_range("rankedConns is empty");
        }
        uint8_t finalLinkRank = linkRank % rankedConns.size();
        ConnectionID connId = rankedConns.at(finalLinkRank).first;
        logDebug(logPrefix + "Sending package on " + connId);

        LinkProperties props = raceSdk->getLinkProperties(raceSdk->getLinkForConnection(connId));
        uint64_t batchId = props.isFlushable ? ++nextBatchId : RACE_BATCH_ID_NULL;

        bool anyError = false;
        for (const auto &uuid : uuidList) {
            Persona persona;
            try {
                persona = uuidToPersonaMap.at(uuid);
            } catch (std::invalid_argument &err) {
                logError(logPrefix + "Failed to find destination UUID " + uuid +
                         " in uuidToPersonaMap");
                anyError = true;
                break;
            }

            EncPkg ePkg(msg.getTraceId(), msg.getSpanId(),
                        encryptor.encryptClrMsg(formattedMsg, persona.getAesKey()));
            logMessageOverhead(formattedMsg, ePkg);

            SdkResponse response = raceSdk->sendEncryptedPackage(ePkg, connId, batchId, 0);
            if (response.status != SDK_OK) {
                logError(logPrefix +
                         "Failed to send: " + std::to_string(static_cast<int>(response.handle)));
                anyError = true;
                break;
            }
            // If this package fails, it'll end up getting re-sent to this persona over a unicast
            // link rather than a multicast link
            resendMap.emplace(response.handle, AddressedMsg{uuid, formattedMsg, msg.getTraceId(),
                                                            msg.getSpanId(), props.reliable, 0});
        }

        if (props.isFlushable) {
            SdkResponse response = raceSdk->flushChannel(props.channelGid, batchId, 0);
            if (response.status != SDK_OK) {
                logError(logPrefix + "Failed to flush channel " + props.channelGid);
                anyError = true;
            }
        }

        if (anyError) {
            if (linkRank + 1 < rankedConns.size()) {
                logInfo(logPrefix + "retrying on next connection");
                return sendMulticastMsg(uuidList, msg, linkRank + 1);
            }
            return false;
        }
        return true;
    } catch (const std::out_of_range &) {
        logError(logPrefix + "No connection to send to destination: " + uuidStr);
    }

    return false;
}

/**
 * @brief Packs a ClrMsg into a string to call sendFormattedMsg(..., std::string &msgString, ...) on
 *
 * @param dstUuid The UUID of the persona to send to
 * @param msg The ClrMsg to process
 *
 * @return The RaceHandle associated with the sent encrypted package.
 */
RaceHandle PluginNMTwoSixClientCpp::sendMsg(const std::string &dstUuid, const ClrMsg &msg) {
    std::string formattedMsg = encryptor.formatDelimitedMessage(msg);
    return sendFormattedMsg(dstUuid, formattedMsg, msg.getTraceId(), msg.getSpanId(), BEST_LINK);
}

PluginResponse PluginNMTwoSixClientCpp::onPackageStatusChanged(RaceHandle handle,
                                                               PackageStatus status) {
    auto [clrMsgHandle, messageStatus] =
        messageStatusTracker.updatePackageStatusForEncPkgHandle(status, handle);
    if (messageStatus != MS_UNDEF) {
        raceSdk->onMessageStatusChanged(clrMsgHandle, messageStatus);
    }

    return PluginNMTwoSix::onPackageStatusChanged(handle, status);
}

PluginResponse PluginNMTwoSixClientCpp::prepareToBootstrap(RaceHandle handle, LinkID linkId,
                                                           std::string configPath,
                                                           DeviceInfo /*deviceInfo*/) {
    TRACE_METHOD();

    // Copy personas
    // BootstrapManager::onBootstrapFinished removes this new dir to account for multiple bootstraps
    std::string from = "personas";
    std::string to = configPath + "/personas";
    raceSdk->makeDir(to);
    for (std::string configFile : raceSdk->listDir(from)) {
        std::vector<std::uint8_t> decryptedFile = raceSdk->readFile(from + "/" + configFile);
        logDebug("prepareToBootstrap: Reading: " + from + "/" + configFile);
        try {
            std::string toFile = to + "/" + configFile;
            logDebug("Writing: " + toFile);
            SdkResponse response = raceSdk->writeFile(toFile, decryptedFile);
            if (response.status != SDK_OK) {
                throw std::runtime_error("prepareToBootstrap: failed to write file: " + toFile);
            }
        } catch (...) {
            logError("Failed to read or write: " + from + "/" + configFile);
            return PLUGIN_ERROR;
        }
    }

    bootstrap.onPrepareToBootstrap(handle, linkId, configPath, clientConfig.entranceCommittee);

    return PLUGIN_OK;
}

PluginResponse PluginNMTwoSixClientCpp::onBootstrapFinished(RaceHandle bootstrapHandle,
                                                            BootstrapState state) {
    TRACE_METHOD(bootstrapHandle, state);
    bootstrap.onBootstrapFinished(bootstrapHandle, state);
    return PLUGIN_OK;
}

PluginResponse PluginNMTwoSixClientCpp::onBootstrapPkgReceived(std::string persona, RawData pkg) {
    TRACE_METHOD();
    EncPkg ePkg(0, 0, pkg);

    ExtClrMsg parsedMsg = parseMsg(ePkg);
    if (parsedMsg.getMsg() == "") {
        logError("onBootstrapPkgReceived: Failed to decrypt");
        return PLUGIN_OK;
    }

    return bootstrap.onBootstrapPackage(persona, parsedMsg, clientConfig.entranceCommittee);
}

void PluginNMTwoSixClientCpp::writeConfigs() {
    TRACE_METHOD();

    clientConfig.bootstrapHandle = 0;
    clientConfig.bootstrapIntroducer = "";

    if (!writeClientConfig(*raceSdk, clientConfig)) {
        logError(logPrefix + "Failed to write network manager config file");
    }
}

void PluginNMTwoSixClientCpp::addClient(const std::string & /*persona*/, const RawData & /*key*/) {
    logError("onBootstrapPkgReceived: unsupported on clients");
}

#ifndef TESTBUILD
IRacePluginNM *createPluginNM(IRaceSdkNM *sdk) {
    return new PluginNMTwoSixClientCpp(sdk);
}

void destroyPluginNM(IRacePluginNM *plugin) {
    delete static_cast<PluginNMTwoSixClientCpp *>(plugin);
}

const RaceVersionInfo raceVersion = RACE_VERSION;
const char *const racePluginId = "PluginNMTwoSixStub";
const char *const racePluginDescription =
    "Plugin Network Manager Client Stub (Two Six Labs) " BUILD_VERSION;
#endif
