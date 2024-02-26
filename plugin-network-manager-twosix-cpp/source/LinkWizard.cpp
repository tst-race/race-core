
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

#include "LinkWizard.h"

#include <OpenTracingHelpers.h>  // traceIdFromContext, spanIdFromContext, createTracer

#include <algorithm>
#include <nlohmann/json.hpp>

#include "ExtClrMsg.h"
#include "Log.h"
#include "PluginNMTwoSix.h"
#include "helper.h"

using json = nlohmann::json;

/**
 * Public Methods
 */

LinkWizard::LinkWizard(const std::string &_raceUuid, PersonaType _personaType,
                       PluginNMTwoSix *_plugin) :
    plugin(_plugin),
    raceUuid(_raceUuid),
    personaType(_personaType),
    currentRequestId(0),
    readyToRespond(false) {}

void LinkWizard::init() {
    TRACE_METHOD();
    tracer = createTracer(plugin->getJaegerConfigPath(), raceUuid);
}

void LinkWizard::setReadyToRespond(bool new_ready) {
    logDebug("  ━☆ LinkWizard::setReadyToRespond: called " + std::to_string(new_ready));
    readyToRespond = new_ready;
    if (readyToRespond) {
        retryRespondSupportedChannels();
    }
    logDebug("  ━☆ LinkWizard::setReadyToRespond: returned");
}

void LinkWizard::resendSupportedChannels() {
    TRACE_METHOD();
    std::shared_ptr<opentracing::Span> span = tracer->StartSpan("resendSupportedChannels");
    span->SetTag("source", "LinkWizard");
    for (const auto &iter : uuidToSupportedChannelsMap) {
        respondSupportedChannels(
            iter.first, {traceIdFromContext(span->context()), spanIdFromContext(span->context())});
    }
}

bool LinkWizard::addPersona(const Persona &persona) {
    std::string uuid = persona.getRaceUuid();
    bool success = true;
    TRACE_METHOD(uuid);
    if (uuidToSupportedChannelsMap.count(uuid) == 0 and outstandingQueries.count(uuid) == 0) {
        success = querySupportedChannels(persona.getRaceUuid());
    }
    return success;
}

uint32_t LinkWizard::numOutstandingRequests() {
    return static_cast<uint32_t>(obtainUnicastQueue.size() + obtainMulticastSendQueue.size());
}

bool LinkWizard::processLinkMsg(const Persona &persona, const ExtClrMsg &extMsg) {
    std::string msg = extMsg.getMsg();
    TRACE_METHOD(persona.getRaceUuid(), msg);
    try {
        std::string uuid = persona.getRaceUuid();
        json msgJson = json::parse(msg);
        if (msgJson.contains("getSupportedChannels") && msgJson["getSupportedChannels"]) {
            respondSupportedChannels(uuid, std::make_pair(extMsg.getTraceId(), extMsg.getSpanId()));
        }
        if (msgJson.contains("supportedChannels")) {
            // Update map of UUID:channels
            uuidToSupportedChannelsMap[uuid] =
                msgJson["supportedChannels"].get<ChannelToLinkSideMap>();
            outstandingQueries.erase(uuid);

            std::string channelGids;
            for (const auto &iter : uuidToSupportedChannelsMap[uuid]) {
                channelGids += iter.first + "=" + linkSideToString(iter.second) + "; ";
            }
            logDebug("  ━☆ LinkWizard: updated supported channels for " + uuid +
                     " to: " + channelGids);

            tryQueuedRequests(uuid);
        }
        if (msgJson.contains("requestCreateUnicastLink")) {
            handleCreateUnicastLinkRequest(uuid, msgJson["requestCreateUnicastLink"]);
        }
        if (msgJson.contains("requestCreateMulticastRecvLink")) {
            json request = msgJson["requestCreateMulticastRecvLink"];
            handleCreateMulticastRecvLinkRequest(uuid, request["channelGid"], request["requestId"]);
        }
        if (msgJson.contains("requestLoadLinkAddress")) {
            handleLoadLinkAddressRequest(uuid, msgJson["requestLoadLinkAddress"]["channelGid"],
                                         msgJson["requestLoadLinkAddress"]["address"],
                                         msgJson["requestLoadLinkAddress"]["personas"]);
        }
        if (msgJson.contains("respondRequestedCreateMulticastRecv")) {
            handleCreateMulticastRecvResponse(
                uuid, msgJson["respondRequestedCreateMulticastRecv"]["requestId"],
                msgJson["respondRequestedCreateMulticastRecv"]["channelGid"],
                msgJson["respondRequestedCreateMulticastRecv"]["address"]);
        }
    } catch (json::exception &e) {
        // NOTE: this is a large try-catch, elements of the try may have executed before getting to
        // a json failure
        logError("  ━☆ LinkWizard::Error parsing LinkMsg JSON: " + msg +
                 " failed with error: " + e.what());
    }
    return true;
}

bool LinkWizard::tryObtainUnicastLink(const Persona &persona, LinkType linkType,
                                      const std::string &channelGid, LinkSide linkSide) {
    std::string uuid = persona.getRaceUuid();
    TRACE_METHOD(uuid, linkType, channelGid, linkSide);
    bool success = false;
    if (not channelsKnownForAllUuids({uuid})) {
        obtainUnicastQueue[uuid].emplace_back(persona, linkType, channelGid, linkSide);
    } else {
        success = obtainUnicastLink(persona, linkType, channelGid, linkSide);
        if (not success) {
            // We might not be able to create the requested link now, based on the current set of
            // enabled channels, so we put it in the retry queue in case the channels change later
            obtainUnicastQueue[uuid].emplace_back(persona, linkType, channelGid, linkSide);
        }
    }
    return success;
}

bool LinkWizard::obtainUnicastLink(const Persona &persona, LinkType linkType,
                                   const std::string &channelGid, LinkSide requestedLinkSide) {
    std::string uuid = persona.getRaceUuid();
    TRACE_METHOD(uuid, linkType, channelGid, requestedLinkSide);

    // Check if we know the other persona's supported channels
    if (not channelsKnownForAllUuids({uuid})) {
        logDebug(logPrefix + "Waiting for supported channels response from " + uuid);
        return false;
    }

    auto [isValid, props, linkSide] = verifyChannelIsSupported(
        {uuid}, linkType, true, personaType == P_CLIENT || persona.getPersonaType() == P_CLIENT,
        channelGid, requestedLinkSide);

    if (not isValid) {
        logWarning(logPrefix + "Unable to obtain unicast " + linkTypeToString(linkType) +
                   " link to " + uuid + " for channel " + channelGid + " with side " +
                   linkSideToString(requestedLinkSide) + ", not supported");
        return false;
    }

    bool success = true;
    logDebug(logPrefix + "Obtaining unicast " + linkTypeToString(linkType) + " link to " + uuid +
             " for channel " + channelGid + " with side " + linkSideToString(linkSide) +
             " (current role side is " + linkSideToString(props.currentRole.linkSide) +
             ") and direction " + linkDirectionToString(props.linkDirection));

    if (linkSide == LS_CREATOR) {
        logDebug("  ━☆ LinkWizard::obtainUnicastLink: creating link");
        SdkResponse response = plugin->getLinkManager()->createLink(channelGid, {uuid});
        if (response.status != SDK_OK) {
            logError("  ━☆ LinkWizard: Error creating link for channel GID: " + channelGid +
                     " failed with sdk response status: " + std::to_string(response.status));
            return false;
        }
        pendingUnicastCreate.emplace(response.handle, uuid);
    } else if (linkSide == LS_LOADER) {
        logDebug("  ━☆ LinkWizard::obtainUnicastLink: requesting create link from other persona");
        // we want to be the loader, so prompt the other node to create
        success = requestCreateUnicastLink(uuid, channelGid);
    } else {
        logError("  ━☆ LinkWizard: invalid link side: " + linkSideToString(linkSide));
        return false;
    }

    if (!success) {
        logError("  ━☆ LinkWizard: Error obtaining unicast link");
    }
    return success;
}

bool LinkWizard::tryObtainMulticastSend(const std::vector<Persona> &personaList, LinkType linkType,
                                        const std::string &channelGid, LinkSide linkSide) {
    auto [uuidStr, uuidList] = personaListToUuidList(personaList);
    TRACE_METHOD(uuidStr, linkType, channelGid, linkSide);
    // Check if we know the other persona's supported channels
    bool success = false;
    if (not channelsKnownForAllUuids(uuidList)) {
        obtainMulticastSendQueue.emplace_back(personaList, linkType, channelGid, linkSide);
    } else {
        success = obtainMulticastSend(personaList, linkType, channelGid, linkSide);
    }
    return success;
}

bool LinkWizard::obtainMulticastSend(const std::vector<Persona> &personaList, LinkType linkType,
                                     const std::string &channelGid, LinkSide requestedLinkSide) {
    auto [uuidStr, uuidList] = personaListToUuidList(personaList);
    TRACE_METHOD(uuidStr, linkType, channelGid, requestedLinkSide);

    // Check if we know the other persona's supported channels
    if (not channelsKnownForAllUuids(uuidList)) {
        logDebug(logPrefix + "Waiting for supported channels response from " + uuidStr);
        return false;
    }

    bool anyClients = personaType == P_CLIENT;
    for (auto persona : personaList) {
        anyClients |= persona.getPersonaType() == P_CLIENT;
    }

    auto [isValid, props, linkSide] = verifyChannelIsSupported(
        uuidList, linkType, false, anyClients, channelGid, requestedLinkSide);

    if (not isValid) {
        logWarning(logPrefix + "Unable to obtain multicast " + linkTypeToString(linkType) +
                   " send link to " + uuidStr + " for channel " + channelGid + " with side " +
                   linkSideToString(requestedLinkSide) + ", not supported");
        return false;
    }

    bool success = true;
    logDebug(logPrefix + "Obtaining multicast " + linkTypeToString(linkType) + " link to " +
             uuidStr + " for channel " + channelGid + " with side " + linkSideToString(linkSide) +
             " (current role side is " + linkSideToString(props.currentRole.linkSide) +
             ") and direction " + linkDirectionToString(props.linkDirection));

    if (linkSide == LS_CREATOR) {
        logDebug(logPrefix + "Creating bidirectional link");
        SdkResponse response = plugin->getLinkManager()->createLink(channelGid, uuidList);
        if (response.status != SDK_OK) {
            logError(logPrefix + "Error creating link for channel GID: " + channelGid +
                     " failed with sdk response status: " + std::to_string(response.status));
            return false;
        }
        pendingMulticastSendCreate.emplace(response.handle, uuidList);
    } else if (linkSide == LS_LOADER) {
        logDebug(logPrefix + "Requesting create link from other personas");
        // we want to be the loader, so prompt the other nodes to create
        std::string requestId = generateRequestId();
        addPendingMultiAddressLoads(requestId, uuidList);
        success = requestCreateMulticastRecvLink(uuidList, channelGid, requestId);
    } else {
        logError(logPrefix + "Invalid link side: " + linkSideToString(linkSide));
        return false;
    }

    if (not success) {
        logError(logPrefix + "Error obtaining multicast send link");
    }
    return success;
}

bool LinkWizard::handleChannelStatusUpdate(RaceHandle handle, std::string &channelGid,
                                           ChannelStatus status) {
    TRACE_METHOD(handle, channelGid, status);
    if (status == CHANNEL_DISABLED) {
        if (readyToRespond) {
            resendSupportedChannels();
        }
    }
    return true;
}

bool LinkWizard::handleLinkStatusUpdate(RaceHandle handle, const LinkID &linkId, LinkStatus status,
                                        const LinkProperties &properties) {
    TRACE_METHOD(handle, linkId, status);
    if (status == LINK_CREATED) {
        if (pendingUnicastCreate.count(handle) > 0) {
            // uuid we want to load this link's address
            std::string uuid = pendingUnicastCreate[handle];
            requestLoadLinkAddress(uuid, properties.channelGid, properties.linkAddress, {raceUuid});
            pendingUnicastCreate.erase(handle);
        } else if (pendingMulticastSendCreate.count(handle) > 0) {
            // list of personas we will want to load this link's address
            std::vector<std::string> uuidList = pendingMulticastSendCreate[handle];

            for (auto uuid : uuidList) {
                // TODO pass error status back if this fails?
                requestLoadLinkAddress(uuid, properties.channelGid, properties.linkAddress,
                                       {raceUuid});
            }
            pendingMulticastSendCreate.erase(handle);
        } else if (pendingRequestedMulticastRecvCreate.count(handle) > 0) {
            auto [requestId, uuid] = pendingRequestedMulticastRecvCreate[handle];

            respondRequestedCreateMulticastRecv(uuid, requestId, properties.channelGid,
                                                properties.linkAddress);
            pendingRequestedMulticastRecvCreate.erase(handle);
        }
    } else if (status == LINK_LOADED or status == LINK_DESTROYED) {
        pendingLoad.erase(handle);
    }
    return true;
}

void LinkWizard::retryRespondSupportedChannels() {
    logDebug("  ━☆ LinkWizard::retryRespondSupportedChannels: called");
    for (auto [uuid, tracingIds] : respondSupportedChannelsQueue) {
        respondSupportedChannels(uuid, tracingIds);
    }
    respondSupportedChannelsQueue.clear();
    logDebug("  ━☆ LinkWizard::retryRespondSupportedChannels: returned");
}

/**
 *  Private methods
 */

void LinkWizard::tryQueuedRequests(const std::string &uuid) {
    TRACE_METHOD(uuid);
    if (obtainUnicastQueue.count(uuid) > 0) {
        auto &unicastRequests = obtainUnicastQueue[uuid];
        for (auto it = unicastRequests.begin(); it != unicastRequests.end();) {
            auto [persona, linkType, channelGid, linkSide] = *it;
            if (obtainUnicastLink(persona, linkType, channelGid, linkSide)) {
                it = unicastRequests.erase(it);
            } else {
                ++it;
            }
        }
        if (unicastRequests.size() == 0) {
            obtainUnicastQueue.erase(uuid);
        }
    }
    if (obtainMulticastSendQueue.size() > 0) {
        for (auto it = obtainMulticastSendQueue.begin(); it != obtainMulticastSendQueue.end();) {
            auto [personaList, linkType, channelGid, linkSide] = *it;
            if (obtainMulticastSend(personaList, linkType, channelGid, linkSide)) {
                it = obtainMulticastSendQueue.erase(it);
            } else {
                ++it;
            }
        }
    }
    if (sendingMessageQueue.size() > 0) {
        std::vector<std::tuple<std::string, std::string, std::uint64_t, std::uint64_t>> requeue;
        for (auto [destUuid, msgString, traceId, spanId] : sendingMessageQueue) {
            logDebug("  ━☆ LinkWizard::tryQueuedRequests: retrying sending to " + destUuid);
            if (not plugin->sendFormattedMsg(destUuid, msgString, traceId, spanId)) {
                requeue.push_back({destUuid, msgString, traceId, spanId});
            }
        }
        sendingMessageQueue = requeue;
    }
}

/**
 * Supported channels request/handle
 */

bool LinkWizard::querySupportedChannels(const std::string &uuid) {
    TRACE_METHOD(uuid);
    ExtClrMsg msg(
        "{\"getSupportedChannels\": true}", raceUuid, uuid, 1, 0, 0, 0, 0, 0,
        MSG_LINKS);  // Using arbitrary/0 Nonce and Timestamp because this information is not used

    setOpenTracing(msg, "querySupportedChannels");
    std::string msgString = encryptor.formatDelimitedMessage(msg);
    logDebug("  ━☆ LinkWizard::querySupportedChannels: sending msg");
    bool success = plugin->sendFormattedMsg(uuid, msgString, msg.getTraceId(), msg.getSpanId());
    if (not success) {
        sendingMessageQueue.push_back({uuid, msgString, msg.getTraceId(), msg.getSpanId()});
    } else {
        outstandingQueries.insert(uuid);
    }
    logDebug("  ━☆ LinkWizard::querySupportedChannels: returned");
    return success;
}

bool LinkWizard::respondSupportedChannels(
    const std::string &uuid, const std::pair<std::uint64_t, std::uint64_t> &tracingIds) {
    TRACE_METHOD(uuid);
    if (not readyToRespond) {
        logDebug("  ━☆ LinkWizard::respondSupportedChannels: not ready, queueing");
        respondSupportedChannelsQueue.push_back({uuid, tracingIds});
        return false;
    }
    auto ctx = spanContextFromIds(tracingIds);
    ChannelToLinkSideMap mySupportedChannels;
    auto channelPropMap = plugin->getSdk()->getSupportedChannels();
    for (auto channelPropPair : channelPropMap) {
        if (channelPropPair.second.connectionType != CT_LOCAL) {
            mySupportedChannels[channelPropPair.first] =
                channelPropPair.second.currentRole.linkSide;
        }
    }
    json msgJson;
    msgJson["supportedChannels"] = mySupportedChannels;

    ExtClrMsg msg(
        msgJson.dump(), raceUuid, uuid, 1, 0, 0, 0, 0, 0,
        MSG_LINKS);  // Using arbitrary/0 Nonce and Timestamp because this information is not used
    setOpenTracing(msg, "respondSupportedChannels", ctx.get());
    logDebug("  ━☆ LinkWizard::respondSupportedChannels: " + msgJson.dump());
    std::string msgString = encryptor.formatDelimitedMessage(msg);
    bool success = plugin->sendFormattedMsg(uuid, msgString, msg.getTraceId(), msg.getSpanId());
    if (not success) {
        sendingMessageQueue.push_back({uuid, msgString, msg.getTraceId(), msg.getSpanId()});
    }
    return success;
}

/**
 * Unicast Link request/handle
 */
bool LinkWizard::requestCreateUnicastLink(const std::string &uuid, const std::string &channelGid) {
    TRACE_METHOD(uuid, channelGid);
    ExtClrMsg msg(
        "{\"requestCreateUnicastLink\": \"" + channelGid + "\"}", raceUuid, uuid, 1, 0, 0, 0, 0, 0,
        MSG_LINKS);  // Using arbitrary/0 Nonce and Timestamp because this information is not used
    setOpenTracing(msg, "requestCreateUnicastLink");
    std::string msgString = encryptor.formatDelimitedMessage(msg);
    bool success = plugin->sendFormattedMsg(uuid, msgString, msg.getTraceId(), msg.getSpanId());
    if (not success) {
        sendingMessageQueue.push_back({uuid, msgString, msg.getTraceId(), msg.getSpanId()});
    }
    return success;
}

bool LinkWizard::handleCreateUnicastLinkRequest(const std::string &uuid,
                                                const std::string &channelGid) {
    TRACE_METHOD(uuid, channelGid);
    auto supportedChannels = plugin->getSdk()->getSupportedChannels();
    if ((supportedChannels.count(channelGid) == 0) ||
        (supportedChannels.at(channelGid).connectionType == CT_LOCAL)) {
        logError("  ━☆ LinkWizard::Requested link is not supported " + channelGid);
        return false;
    }

    if (channelLinksFull(plugin->getSdk(), channelGid)) {
        logError(
            "  ━☆ LinkWizard::handleCreateUnicastLinkRequest: error creating or loading link for "
            "channel " +
            channelGid +
            " because the number of links on channel is at or exceeds the max number of links for "
            "channel");
        return false;
    }

    SdkResponse response = plugin->getLinkManager()->createLink(channelGid, {uuid});
    if (response.status != SDK_OK) {
        logError("  ━☆ LinkWizard::Error creating link " + channelGid);
        return false;
    }
    pendingUnicastCreate.emplace(response.handle, uuid);

    return true;
}

bool LinkWizard::requestLoadLinkAddress(const std::string &uuid, const std::string &channelGid,
                                        const std::string &linkAddress,
                                        const std::vector<std::string> &uuidList) {
    TRACE_METHOD(uuid, channelGid);
    json request;
    request["channelGid"] = channelGid;
    request["address"] = linkAddress;
    request["personas"] = uuidList;
    json msgJson;
    msgJson["requestLoadLinkAddress"] = request;
    ExtClrMsg msg(
        msgJson.dump(), raceUuid, uuid, 1, 0, 0, 0, 0, 0,
        MSG_LINKS);  // Using arbitrary/0 Nonce and Timestamp because this information is not used
    setOpenTracing(msg, "requestLoadLinkAddress");
    std::string msgString = encryptor.formatDelimitedMessage(msg);
    bool success = plugin->sendFormattedMsg(uuid, msgString, msg.getTraceId(), msg.getSpanId());
    if (not success) {
        sendingMessageQueue.push_back({uuid, msgString, msg.getTraceId(), msg.getSpanId()});
    }
    return success;
}

bool LinkWizard::handleLoadLinkAddressRequest(const std::string &uuid,
                                              const std::string &channelGid,
                                              const std::string &linkAddress,
                                              const std::vector<std::string> &uuidList) {
    TRACE_METHOD(uuid, channelGid);
    auto supportedChannels = plugin->getSdk()->getSupportedChannels();
    if ((supportedChannels.count(channelGid) == 0) ||
        (supportedChannels.at(channelGid).connectionType == CT_LOCAL)) {
        logError("  ━☆ LinkWizard::Requested link is not supported " + channelGid);
        return false;
    }

    if (channelLinksFull(plugin->getSdk(), channelGid)) {
        logError("  ━☆ LinkWizard::handleLoadLinkAddressRequest: error loading link for channel " +
                 channelGid +
                 " because the number of links on channel is at or exceeds the max number of links "
                 "for channel");
        return false;
    }

    // erase our own UUID from the list of personas reached by this link
    std::vector<std::string> personas = uuidList;
    personas.erase(std::remove(personas.begin(), personas.end(), raceUuid), personas.end());
    SdkResponse response =
        plugin->getLinkManager()->loadLinkAddress(channelGid, linkAddress, personas);
    if (response.status != SDK_OK) {
        logError("  ━☆ LinkWizard: Error loading link address for channel " + channelGid +
                 "  with address " + linkAddress +
                 " failed with sdk response status: " + std::to_string(response.status));
        return false;
    }
    pendingLoad.emplace(response.handle, std::vector<std::string>({uuid}));

    return true;
}

/**
 * Multicast send request/handle
 */
bool LinkWizard::requestCreateMulticastRecvLink(const std::vector<std::string> &uuidList,
                                                const std::string &channelGid,
                                                const std::string &requestId) {
    bool success = true;
    json request;
    request["channelGid"] = channelGid;
    request["requestId"] = requestId;
    json msgJson;
    msgJson["requestCreateMulticastRecvLink"] = request;
    TRACE_METHOD(channelGid, requestId);

    for (auto uuid : uuidList) {
        logDebug("  ━☆ LinkWizard::requestCreateMulticastRecvLink: called with uuid " + uuid);

        ExtClrMsg msg(msgJson.dump(), raceUuid, uuid, 1, 0, 0, 0, 0, 0,
                      MSG_LINKS);  // Using arbitrary/0 Nonce and Timestamp because this information
                                   // is not used
        setOpenTracing(msg, "requestCreateMulticastRecvLink");
        std::string msgString = encryptor.formatDelimitedMessage(msg);
        auto handle = plugin->sendFormattedMsg(uuid, msgString, msg.getTraceId(), msg.getSpanId());
        success &= handle != NULL_RACE_HANDLE;
        if (not success) {
            sendingMessageQueue.push_back({uuid, msgString, msg.getTraceId(), msg.getSpanId()});
        }
    }
    return success;
}

bool LinkWizard::handleCreateMulticastRecvLinkRequest(const std::string &uuid,
                                                      const std::string &channelGid,
                                                      const std::string &requestId) {
    TRACE_METHOD(uuid, channelGid, requestId);
    auto supportedChannels = plugin->getSdk()->getSupportedChannels();
    if ((supportedChannels.count(channelGid) == 0) ||
        (supportedChannels.at(channelGid).connectionType == CT_LOCAL)) {
        logError("  ━☆ LinkWizard::Requested channel is not supported: " + channelGid);
        return false;
    }

    if (channelLinksFull(plugin->getSdk(), channelGid)) {
        logError(
            "  ━☆ LinkWizard::handleCreateMulticastRecvLinkRequest: error creating or loading link "
            "for channel " +
            channelGid +
            " because the number of links on channel is at or exceeds the max number of links for "
            "channel");
        return false;
    }

    SdkResponse response = plugin->getLinkManager()->createLink(channelGid, {uuid});
    if (response.status != SDK_OK) {
        logError("  ━☆ LinkWizard::Error creating link for channel with GID: " + channelGid);
        return false;
    }
    pendingRequestedMulticastRecvCreate.emplace(
        response.handle, std::pair<std::string, std::string>({requestId, uuid}));

    return true;
}

bool LinkWizard::respondRequestedCreateMulticastRecv(const std::string &uuid,
                                                     const std::string &requestId,
                                                     const std::string &channelGid,
                                                     const std::string &linkAddress) {
    TRACE_METHOD(uuid, channelGid, requestId);
    json request;
    request["channelGid"] = channelGid;
    request["address"] = linkAddress;
    request["requestId"] = requestId;
    json msgJson;
    msgJson["respondRequestedCreateMulticastRecv"] = request;
    ExtClrMsg msg(
        msgJson.dump(), raceUuid, uuid, 1, 0, 0, 0, 0, 0,
        MSG_LINKS);  // Using arbitrary/0 Nonce and Timestamp because this information is not used
    setOpenTracing(msg, "responseRequestedCreateMulticastRecv");
    std::string msgString = encryptor.formatDelimitedMessage(msg);
    bool success = plugin->sendFormattedMsg(uuid, msgString, msg.getTraceId(), msg.getSpanId());
    if (not success) {
        sendingMessageQueue.push_back({uuid, msgString, msg.getTraceId(), msg.getSpanId()});
    }
    return success;
}

bool LinkWizard::handleCreateMulticastRecvResponse(const std::string &uuid,
                                                   const std::string &requestId,
                                                   const std::string &channelGid,
                                                   const std::string &linkAddress) {
    TRACE_METHOD(uuid, channelGid, requestId);

    if (pendingMultiAddressLoads.count(requestId) == 0) {
        logError(" ━☆ LinkWizard::handleCreateMulticastRecvResponse: no pending request for ID: " +
                 requestId);
        return false;
    }

    pendingMultiAddressLoads[requestId][uuid] = linkAddress;
    auto uuidAddressMap = pendingMultiAddressLoads[requestId];
    uuidAddressMap[uuid] = linkAddress;
    std::vector<std::string> addressList;
    std::vector<std::string> uuidList;
    for (auto uuidAddress : uuidAddressMap) {
        if (uuidAddress.second == "") {
            logDebug(
                " ━☆ LinkWizard::handleCreateMulticastRecvResponse: still waiting for (at least) "
                "the address for: " +
                uuidAddress.first);
            return false;
        } else {
            uuidList.push_back(uuidAddress.first);
            addressList.push_back(uuidAddress.second);
        }
    }

    SdkResponse response =
        plugin->getLinkManager()->loadLinkAddresses(channelGid, addressList, uuidList);
    if (response.status != SDK_OK) {
        logError("  ━☆ LinkWizard: Error loading link addresses for channel " + channelGid +
                 " failed with sdk response status: " + std::to_string(response.status));
        return false;
    }
    pendingLoad.emplace(response.handle, uuidList);

    return true;
}

bool LinkWizard::channelsKnownForAllUuids(const std::vector<std::string> &uuidList) {
    for (auto uuid : uuidList) {
        if (uuidToSupportedChannelsMap.count(uuid) == 0) {
            logInfo("  ━☆ LinkWizard:: supported channels unknown for uuid: " + uuid +
                    ". The query may still be outstanding or perhaps addPersona(" + uuid +
                    ") needs to be called. Enqueuing request for re-trying later.");
            return false;
        }
    }
    return true;
}

/**
 * @brief Determine the link side that will allow us to establish a link of the specified link
 * type, considering the link direction, our current role's allowed link sides and their current
 * role's allowed link sides.
 *
 * @param linkType The desired type of link to establish
 * @param linkDirection The directionality of the link
 * @param ourLinkSide Link side (creator, loader, both) allowed by our current channel role
 * @param theirLinkSide Link side (creator, loader, both) allowed by their current channel role
 * @return Link side (creator or loader) we can use to establish the link, or LS_UNDEF if no
 *         link can be established within the link constraints
 */
LinkSide determineLinkSide(LinkType linkType, LinkDirection linkDirection, LinkSide ourLinkSide,
                           LinkSide theirLinkSide) {
    // If any value is undefined, return undefined
    if (linkType == LT_UNDEF or linkDirection == LD_UNDEF or ourLinkSide == LS_UNDEF or
        theirLinkSide == LS_UNDEF) {
        return LS_UNDEF;
    }

    // If we want a bidirectional link but the channel doesn't support it, we can't use it
    if (linkType == LT_BIDI and linkDirection != LD_BIDI) {
        return LS_UNDEF;
    }

    // In order to create a link:
    // 1. we have to support creating while they have to support loading, and
    // 2. we have to be either:
    //   a. using a bidirectional link, or
    //   b. sending over a creator-to-loader link, or
    //   c. receiving over a loader-to-creator link
    if ((ourLinkSide == LS_CREATOR or ourLinkSide == LS_BOTH) and
        (theirLinkSide == LS_LOADER or theirLinkSide == LS_BOTH) and
        ((linkDirection == LD_BIDI) or
         (linkType == LT_SEND and linkDirection == LD_CREATOR_TO_LOADER) or
         (linkType == LT_RECV and linkDirection == LD_LOADER_TO_CREATOR))) {
        return LS_CREATOR;
    }

    // In order to load a link:
    // 1. we have to support loading while they have to support creating, and
    // 2. we have to be either:
    //   a. sending over a loader-to-creator link, or
    //   b. receiving over a creator-to-loader link
    if ((ourLinkSide == LS_LOADER or ourLinkSide == LS_BOTH) and
        (theirLinkSide == LS_CREATOR or theirLinkSide == LS_BOTH) and
        ((linkType == LT_SEND and linkDirection == LD_LOADER_TO_CREATOR) or
         (linkType == LT_RECV and linkDirection == LD_CREATOR_TO_LOADER))) {
        return LS_LOADER;
    }

    return LS_UNDEF;
}

std::tuple<bool, ChannelProperties, LinkSide> LinkWizard::verifyChannelIsSupported(
    const std::vector<std::string> &uuidList, LinkType linkType, bool unicast, bool anyClients,
    const std::string &channelGid, LinkSide requestedLinkSide) {
    std::string logPrefix = "LinkWizard::verifyChannelIsSupported: ";

    // Make sure we support the requested channel
    auto ourSupportedChannels = plugin->getSdk()->getSupportedChannels();
    if (ourSupportedChannels.count(channelGid) == 0) {
        logDebug(logPrefix + "channel " + channelGid + " is not a supported channel");
        return {false, {}, LS_UNDEF};
    }

    auto channelProps = ourSupportedChannels[channelGid];

    // Make sure channel isn't local
    if (channelProps.connectionType == CT_LOCAL) {
        logDebug(logPrefix + "channel " + channelGid +
                 " is not valid because it is a local connection channel");
        return {false, {}, LS_UNDEF};
    }

    // Make sure max-links isn't exceeded for the channel
    if (static_cast<int>(plugin->getSdk()->getLinksForChannel(channelGid).size()) >=
        channelProps.maxLinks) {
        logDebug(logPrefix + "channel " + channelGid +
                 " is at or exceeds the max number of links allowed");
        return {false, {}, LS_UNDEF};
    }

    // If link is unicast, any transmission type is supported, otherwise it has to be multicast
    if (not unicast and channelProps.transmissionType != TT_MULTICAST) {
        logDebug(logPrefix + "channel " + channelGid +
                 " is not multicast transmission type, cannot be used for multicast link");
        return {false, {}, LS_UNDEF};
    }

    // If we don't need bidirectional, then any link direction can work, otherwise it has to be bidi
    if (linkType == LT_BIDI and channelProps.linkDirection != LD_BIDI) {
        logDebug(logPrefix + "channel " + channelGid +
                 " is not bidirectional, cannot be used for bidirectional link");
        return {false, {}, LS_UNDEF};
    }

    // If any destinations are clients, then the connection type has to be indirect
    if (anyClients and channelProps.connectionType != CT_INDIRECT) {
        logDebug(logPrefix + "channel " + channelGid +
                 " is not indirect, cannot be used for clients");
        return {false, {}, LS_UNDEF};
    }

    // Make sure the requested link side matches the current channel role link side
    if (channelProps.currentRole.linkSide != LS_BOTH and requestedLinkSide != LS_BOTH and
        channelProps.currentRole.linkSide != requestedLinkSide) {
        logDebug(logPrefix + "channel " + channelGid + " does not support requested link side (" +
                 linkSideToString(channelProps.currentRole.linkSide) + " vs " +
                 linkSideToString(requestedLinkSide) + ")");
        return {false, {}, LS_UNDEF};
    }

    LinkSide chosenLinkSide = LS_UNDEF;

    // Make sure all destinations support the requested channel
    for (auto uuid : uuidList) {
        auto theirSupportedChannels = uuidToSupportedChannelsMap[uuid];
        if (theirSupportedChannels.count(channelGid) == 0) {
            logDebug(logPrefix + "channel " + channelGid + " is not supported by " + uuid);
            return {false, {}, LS_UNDEF};
        }

        // Make sure we can actually use this channel based on channel roles
        LinkSide linkSide =
            determineLinkSide(linkType, channelProps.linkDirection, requestedLinkSide,
                              theirSupportedChannels[channelGid]);
        if (linkSide == LS_UNDEF) {
            logDebug(logPrefix + "channel " + channelGid + " has incompatible link side roles");
            return {false, {}, LS_UNDEF};
        }
        // Check if this link side would conflict with a previously determined link side for
        // another node
        else if (chosenLinkSide != LS_UNDEF and chosenLinkSide != linkSide) {
            logDebug(logPrefix + "channel " + channelGid + " has conflicting link side roles");
            return {false, {}, LS_UNDEF};
        } else {
            // The channel is ok, but has to use the determined link side
            chosenLinkSide = linkSide;
        }
    }

    return {true, channelProps, chosenLinkSide};
}

std::pair<std::string, std::vector<std::string>> LinkWizard::personaListToUuidList(
    const std::vector<Persona> &personaList) {
    std::vector<std::string> uuidList;
    std::string uuidMsg;
    for (auto persona : personaList) {
        uuidList.push_back(persona.getRaceUuid());
        uuidMsg += persona.getRaceUuid() + " ";
    }
    return {uuidMsg, uuidList};
}

void LinkWizard::addPendingMultiAddressLoads(std::string requestId,
                                             std::vector<std::string> uuidList) {
    std::unordered_map<std::string, std::string> uuidAddressMap;
    for (auto uuid : uuidList) {
        if (uuid != raceUuid) {
            uuidAddressMap[uuid] = "";
        }
    }
    pendingMultiAddressLoads.emplace(requestId, uuidAddressMap);
}

std::string LinkWizard::generateRequestId() {
    return raceUuid + std::to_string(currentRequestId++);
}

void LinkWizard::setOpenTracing(ExtClrMsg &msg, const std::string &spanName,
                                const opentracing::SpanContext *ctx) {  // ctx = nullptr
    std::shared_ptr<opentracing::Span> span;
    if (ctx == nullptr) {
        span = tracer->StartSpan(spanName);
    } else {
        span = tracer->StartSpan(spanName, {opentracing::ChildOf(ctx)});
    }
    span->SetTag("source", "LinkWizard");
    span->SetTag("messageSize", std::to_string(msg.getMsg().size()));
    span->SetTag("messageFrom", msg.getFrom());
    span->SetTag("messageTo", msg.getTo());
    msg.setTraceId(traceIdFromContext(span->context()));
    msg.setSpanId(spanIdFromContext(span->context()));
}
