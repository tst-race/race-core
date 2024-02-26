
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

#include "../include/PluginNMTestHarness.h"

#include <sstream>

#include "MessageSerializer/MessageSerializer.h"
#include "RaceLog.h"
#include "helpers.h"

#define TRACE_METHOD(...) TRACE_METHOD_BASE(PluginNMTestHarness, ##__VA_ARGS__)

PluginNMTestHarness::PluginNMTestHarness(IRaceSdkNM *sdk) : raceSdk(sdk) {
    TRACE_METHOD();
    if (sdk == nullptr) {
        const std::string errorMessage = "sdk passed to init is nullptr.";
        logError(errorMessage);
        throw std::invalid_argument(errorMessage);
    }
    activePersona = raceSdk->getActivePersona();
}

PluginNMTestHarness::~PluginNMTestHarness() {
    TRACE_METHOD();
}

PluginResponse PluginNMTestHarness::init(const PluginConfig & /*pluginConfig*/) {
    activePersona = raceSdk->getActivePersona();
    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::shutdown() {
    return PLUGIN_OK;
}

std::tuple<std::string, LinkID, ConnectionID> PluginNMTestHarness::splitRoute(
    const std::string &route) {
    std::string channelGid;
    LinkID linkId;
    ConnectionID connId;

    auto fragments = helpers::split(route, "/");
    if (fragments.size() == 4u) {
        channelGid = fragments[1];
        linkId = fragments[0] + "/" + channelGid + "/" + fragments[2];
        connId = route;
    } else if (fragments.size() == 3u) {
        channelGid = fragments[1];
        linkId = fragments[0] + "/" + channelGid + "/" + fragments[2];
    } else if (fragments.size() == 2u) {
        channelGid = fragments[1];
    } else if (fragments.size() == 1u) {
        channelGid = fragments[0];
    }

    return {channelGid, linkId, connId};
}

PluginResponse PluginNMTestHarness::processNMBypassMsg(RaceHandle /*handle*/,
                                                       const std::string &route,
                                                       const ClrMsg &msg) {
    TRACE_METHOD(route);

    auto [channelGid, linkId, connId] = splitRoute(route);

    if (channelGid.empty()) {
        logError(logPrefix + "invalid route descriptor: " + route);
        return PLUGIN_ERROR;
    }

    const size_t messageLengthLogLimit = 256;
    std::string message = msg.getMsg();
    if (message.size() > messageLengthLogLimit) {
        message.resize(messageLengthLogLimit - 3);
        message += "...";
    }
    logDebug(logPrefix + message);

    std::string messageToEncrypt;
    try {
        messageToEncrypt = MessageSerializer::serialize(msg);
    } catch (std::invalid_argument &e) {
        logError(logPrefix + "Failed to serialize clear message", e.what());
        return PLUGIN_ERROR;
    }

    EncPkg pkg(msg.getTraceId(), msg.getSpanId(),
               {messageToEncrypt.begin(), messageToEncrypt.end()});

    logMessageOverhead(msg, pkg);

    if (not connId.empty()) {
        logInfo(logPrefix + "sending message on existing connection: " + connId);
        const uint64_t batchId = 0;
        SdkResponse response = raceSdk->sendEncryptedPackage(pkg, connId, batchId, 0);
        if (response.status != SDK_OK) {
            logError(logPrefix + "sendEncryptedPackage failed on connId: " + connId);
        }
        return PLUGIN_OK;
    } else {
        return openConnAndQueueToSend({msg.getTo(), channelGid, linkId, pkg});
    }
}

PluginResponse PluginNMTestHarness::processClrMsg(RaceHandle /*handle*/, const ClrMsg & /*msg*/) {
    return PLUGIN_OK;
}

LinkID PluginNMTestHarness::getLinkForChannel(const std::string &persona,
                                              const std::string &channelGid, LinkType linkType) {
    logDebug("getLinkForChannel: looking for a link to " + persona + " on channel " + channelGid +
             " of type " + linkTypeToString(linkType));
    auto potentialLinks = raceSdk->getLinksForPersonas({persona}, linkType);
    for (auto potentialLink : potentialLinks) {
        auto linkProps = raceSdk->getLinkProperties(potentialLink);
        if (channelGid == "*" or linkProps.channelGid == channelGid) {
            logDebug("getLinkForChannel: using " + potentialLink);
            return potentialLink;
        } else {
            if (linkProps.channelGid == "") {
                logError("getLinkForChannel: channel GID not set for link: " + potentialLink);
            }
            logDebug("getLinkForChannel: skipping " + potentialLink + " with channelGid \"" +
                     linkProps.channelGid + "\"");
        }
    }
    logDebug("getLinkForChannel: no link found");
    return LinkID();
}

PluginResponse PluginNMTestHarness::openConnAndQueueToSend(const AddressedPkg &addrPkg) {
    LinkID linkToSend = addrPkg.linkId;

    if (linkToSend.empty()) {
        linkToSend = getLinkForChannel(addrPkg.dst, addrPkg.channelGid, LT_SEND);
    }

    if (linkToSend.empty()) {
        logError("Unable to determine link to send to \"" + addrPkg.dst + "\"");
        return PLUGIN_ERROR;
    }

    logInfo("openConnAndQueueToSend: opening connection on LinkID " + linkToSend + " to " +
            addrPkg.dst);
    SdkResponse response = raceSdk->openConnection(LT_SEND, linkToSend, "{}", 0, RACE_UNLIMITED, 0);
    if (response.status != SDK_OK) {
        logError("openConnAndQueueToSend: failed to open LinkID: " + linkToSend);
        return PLUGIN_ERROR;
    }
    sendMap.emplace(response.handle, addrPkg);

    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::processEncPkg(RaceHandle /*handle*/, const EncPkg &pkg,
                                                  const std::vector<std::string> &connIDs) {
    TRACE_METHOD();

    for (auto &connId : connIDs) {
        auto iter = recvConnIds.find(connId);
        if (iter != recvConnIds.end()) {
            recvConnIds.erase(iter);
            raceSdk->closeConnection(connId, 0);
        }
    }

    const RawData cipherText = pkg.getCipherText();
    const std::string cipherTextString = std::string{cipherText.begin(), cipherText.end()};
    logDebug(logPrefix + "received cipher text of length " +
             std::to_string(cipherTextString.size()));
    try {
        ClrMsg msg = MessageSerializer::deserialize(cipherTextString);
        msg.setTraceId(pkg.getTraceId());
        msg.setSpanId(pkg.getSpanId());

        // Check to see if message is for me, if so present it to racetestapp
        if (msg.getTo() == activePersona) {
            raceSdk->presentCleartextMessage(msg);
        } else {
            logDebug(logPrefix + "Received message for recipient, " + msg.getTo() + ", ignoring");
        }

    } catch (const std::invalid_argument &error) {
        logError(logPrefix +
                 "Failed to parse received encrypted package: " + std::string(error.what()));
        return PLUGIN_ERROR;
    }
    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::prepareToBootstrap(RaceHandle /*handle*/, LinkID /*linkId*/,
                                                       std::string /*configPath*/,
                                                       DeviceInfo /*deviceInfo*/) {
    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::onBootstrapPkgReceived(std::string /*persona*/,
                                                           RawData /*pkg*/) {
    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::openRecvConnection(RaceHandle /*handle*/,
                                                       const std::string &persona,
                                                       const std::string &route) {
    TRACE_METHOD(persona, route);

    auto [channelGid, linkId, connId] = splitRoute(route);

    LinkID linkToOpen = linkId;
    if (linkToOpen.empty()) {
        linkToOpen = getLinkForChannel(persona, channelGid, LT_RECV);
    }

    if (linkToOpen.empty()) {
        logError(logPrefix + "Unable to determine link to receive from \"" + persona + "\"");
        return PLUGIN_ERROR;
    }

    SdkResponse response = raceSdk->openConnection(LT_RECV, linkToOpen, "{}", 0, RACE_UNLIMITED, 0);
    if (response.status != SDK_OK) {
        logError(logPrefix + "failed to open LinkID: " + linkToOpen);
        return PLUGIN_ERROR;
    }
    pendingRecvConns.emplace(response.handle, linkToOpen);

    logInfo(logPrefix + "opening receive connection on linkId \"" + linkToOpen +
            "\" for persona \"" + persona + "\"");
    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::rpcDeactivateChannel(const std::string &channelGid) {
    TRACE_METHOD(channelGid);

    SdkResponse response = raceSdk->deactivateChannel(channelGid, RACE_BLOCKING);
    if (response.status != SDK_OK) {
        logError(logPrefix + "failed to deactivate " + channelGid);
        return PLUGIN_ERROR;
    }

    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::rpcDestroyLink(const std::string &linkId) {
    TRACE_METHOD(linkId);

    std::string channelGid;

    // Only valid forms are:
    // - PluginID/ChannelID/LinkID
    // - PluginID/ChannelID/*
    // - ChannelID/*
    auto fragments = helpers::split(linkId, "/");
    if (fragments.size() == 3u) {
        if (fragments[2] == "*") {
            channelGid = fragments[1];
        }
    } else if (fragments.size() == 2u && fragments[1] == "*") {
        channelGid = fragments[0];
    } else {
        logError(logPrefix + "invalid link ID: " + linkId);
        return PLUGIN_ERROR;
    }

    // channelGid is populated only when a wildcard link ID is used
    if (channelGid.empty()) {
        logInfo(logPrefix + "destroying link " + linkId);
        SdkResponse response = raceSdk->destroyLink(linkId, RACE_BLOCKING);
        if (response.status != SDK_OK) {
            logError(logPrefix + "failed to destroy " + linkId);
            return PLUGIN_ERROR;
        }
    } else {
        logInfo(logPrefix + "destroying all links for channel " + channelGid);
        auto channelLinks = raceSdk->getLinksForChannel(channelGid);
        for (auto &channelLinkId : channelLinks) {
            logInfo(logPrefix + "destroying link " + channelLinkId);
            SdkResponse response = raceSdk->destroyLink(channelLinkId, RACE_BLOCKING);
            if (response.status != SDK_OK) {
                logError(logPrefix + "failed to destroy " + channelLinkId);
            }
        }
    }

    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::rpcCloseConnection(const std::string &connectionId) {
    TRACE_METHOD(connectionId);

    // Only valid forms are:
    // - PluginID/ChannelID/LinkID/ConnectionID
    // This form is not yet supported, as we don't have an SDK method to get connections for a link
    // - PluginID/ChannelID/LinkID/*
    auto fragments = helpers::split(connectionId, "/");
    if (fragments.size() != 4u) {
        logError(logPrefix + "invalid connection ID: " + connectionId);
        return PLUGIN_ERROR;
    }

    if (fragments[3] == "*") {
        std::string linkId = fragments[0] + "/" + fragments[1] + "/" + fragments[2];
        logError(logPrefix + "closing all connections for link " + linkId + " is not supported");
        return PLUGIN_ERROR;
    } else {
        logInfo(logPrefix + "closing connection " + connectionId);
        SdkResponse response = raceSdk->closeConnection(connectionId, RACE_BLOCKING);
        if (response.status != SDK_OK) {
            logError(logPrefix + "failed to close " + connectionId);
            return PLUGIN_ERROR;
        }
    }

    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::onConnectionStatusChanged(RaceHandle handle,
                                                              ConnectionID connId,
                                                              ConnectionStatus status,
                                                              LinkID linkId,
                                                              LinkProperties /*properties*/) {
    TRACE_METHOD(connId, status, linkId);
    if (status == CONNECTION_OPEN) {
        auto sendIter = sendMap.find(handle);
        if (sendIter != sendMap.end()) {
            const AddressedPkg addrPkg = sendIter->second;
            sendMap.erase(sendIter);
            logInfo(logPrefix + "Sending message to \"" + addrPkg.dst + "\" on connId: " + connId);
            const uint64_t batchId = 0;
            SdkResponse response = raceSdk->sendEncryptedPackage(addrPkg.pkg, connId, batchId, 0);
            if (response.status != SDK_OK) {
                logError(logPrefix + "sendEncryptedPackage failed on connId: " + connId);
            }
            // TODO re-queue package and try again
            logInfo(logPrefix + "closing connection: " + connId);
            raceSdk->closeConnection(connId, 0);
        }

        auto recvIter = pendingRecvConns.find(handle);
        if (recvIter != pendingRecvConns.end()) {
            pendingRecvConns.erase(recvIter);
            recvConnIds.emplace(connId, linkId);
        }
    } else if (status == CONNECTION_CLOSED) {
        auto iter = sendMap.find(handle);
        if (iter != sendMap.end()) {
            const AddressedPkg addrPkg = iter->second;
            sendMap.erase(iter);
            openConnAndQueueToSend(addrPkg);
        }
    }

    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::onLinkStatusChanged(RaceHandle /*handle*/, LinkID /*connId*/,
                                                        LinkStatus /*status*/,
                                                        LinkProperties /*properties*/) {
    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::onChannelStatusChanged(RaceHandle /*handle*/,
                                                           std::string /*channelGid*/,
                                                           ChannelStatus /*status*/,
                                                           ChannelProperties /*properties*/) {
    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::onPersonaLinksChanged(std::string /*recipientPersona*/,
                                                          LinkType /*linkType*/,
                                                          std::vector<LinkID> /*links*/) {
    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::onLinkPropertiesChanged(LinkID /*linkId*/,
                                                            LinkProperties /*linkProperties*/) {
    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::onPackageStatusChanged(RaceHandle /*handle*/,
                                                           PackageStatus status) {
    TRACE_METHOD(status);
    if (status == PACKAGE_SENT) {
        logDebug(logPrefix + "SENT");
    } else if (status == PACKAGE_RECEIVED) {
        logDebug(logPrefix + "RECEIVED");
    } else {
        logDebug(logPrefix + "FAILED");
    }
    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::onUserInputReceived(RaceHandle /*handle*/, bool /*answered*/,
                                                        const std::string & /*response*/) {
    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::onUserAcknowledgementReceived(RaceHandle /*handle*/) {
    return PLUGIN_OK;
}

PluginResponse PluginNMTestHarness::notifyEpoch(const std::string &data) {
    TRACE_METHOD(data);
    return PLUGIN_OK;
}

std::string PluginNMTestHarness::getDescription() {
    return "Network Manager Test Harness " BUILD_VERSION;
}

void PluginNMTestHarness::logDebug(const std::string &message, const std::string &stackTrace) {
    RaceLog::logDebug("PluginNMTestHarness", message, stackTrace);
}

void PluginNMTestHarness::logInfo(const std::string &message, const std::string &stackTrace) {
    RaceLog::logInfo("PluginNMTestHarness", message, stackTrace);
}

void PluginNMTestHarness::logError(const std::string &message, const std::string &stackTrace) {
    RaceLog::logError("PluginNMTestHarness", message, stackTrace);
}

void PluginNMTestHarness::logMessageOverhead(const ClrMsg &message, const EncPkg &package) {
    const size_t messageSizeInBytes = message.getMsg().size();
    const size_t packageSizeInBytes = package.getRawData().size();
    const size_t overhead = packageSizeInBytes - messageSizeInBytes;

    std::stringstream logMessage;
    logMessage << "clear message size: " << messageSizeInBytes << " bytes. ";
    logMessage << "encrypted package size: " << packageSizeInBytes << " bytes. ";
    logMessage << "overhead: " << overhead << " bytes.";

    logInfo(logMessage.str());
}
