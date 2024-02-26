
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

#include "BootstrapManager.h"

#include <nlohmann/json.hpp>

#include "JsonIO.h"
#include "Log.h"
#include "PluginNMTwoSix.h"
#include "PluginNMTwoSixServerCpp.h"
#include "helper.h"

PluginResponse BootstrapManager::onPrepareToBootstrap(
    RaceHandle handle, LinkID linkId, const std::string &configPath,
    const std::vector<std::string> &entranceCommittee) {
    TRACE_METHOD(handle, linkId, configPath);
    std::uniform_int_distribution<uint64_t> dist(1, 0xffffffffffffffff);

    OutstandingBootstrap requests;
    requests.sdkHandle = handle;
    requests.bootstrapHandle = dist(bootstrapHandleGenerator);
    requests.configPath = configPath;
    requests.bootstrapLinkId = linkId;

    // Open a connection on this link so we can receive messages from the introduced node.
    // Note: the SDK is going to also open its own connection, but the SDK will close it as soon as
    // the bootstrap package is received.
    SdkResponse resp =
        plugin->getSdk()->openConnection(LT_RECV, linkId, "", 0, RACE_UNLIMITED, RACE_UNLIMITED);
    if (resp.status == SDK_OK) {
        requests.outstandingOpenConnectionHandle = resp.handle;
    }

    auto props = plugin->getSdk()->getLinkProperties(linkId);
    requests.receivedLinks.push_back({props.linkAddress, props.channelGid, plugin->getUuid()});

    for (const std::string &com : entranceCommittee) {
        logDebug("BootstrapManager: requesting links from committee member: " + com);
        BootstrapMessage bMsg;

        bMsg.type = BootstrapMessageType::LINK_CREATE_REQUEST;
        bMsg.bootstrapHandle = requests.bootstrapHandle;
        bMsg.messageHandle = messageCounter++;
        bMsg.channelGids = plugin->getExpectedChannels(com);

        requests.outstandingHandles.push_back(bMsg.messageHandle);
        sendBootstrapMsg(bMsg, com);
    }

    bootstrapEntranceCommittee = entranceCommittee;
    outstandingBootstraps.push_back(requests);
    return PLUGIN_OK;
}

PluginResponse BootstrapManager::onBootstrapFinished(RaceHandle _bootstrapHandle,
                                                     BootstrapState state) {
    TRACE_METHOD(_bootstrapHandle, state);

    auto it = std::find_if(
        outstandingBootstraps.begin(), outstandingBootstraps.end(),
        [_bootstrapHandle](const auto &info) { return _bootstrapHandle == info.sdkHandle; });

    if (it != outstandingBootstraps.end()) {
        if (state == BootstrapState::BOOTSTRAP_CANCELLED) {
            // close bootstrap connection or destroy the link (if not connected)
            if (not it->bootstrapConnectionId.empty()) {
                plugin->getSdk()->closeConnection(it->bootstrapConnectionId, RACE_UNLIMITED);
                // onConnectionStatusChanged destroys link and removes OutstandingBootstrap entry
            } else if (not it->bootstrapLinkId.empty()) {
                plugin->getSdk()->destroyLink(it->bootstrapLinkId, RACE_UNLIMITED);
                outstandingBootstraps.erase(it);  // not removed in onLinkStatusChanged
            } else {
                outstandingBootstraps.erase(it);
            }
        }
        plugin->getSdk()->removeDir(it->configPath);
    } else {
        logInfo(logPrefix + " unable to lookup bootstrap record by handle");
    }

    return PLUGIN_OK;
}

PluginResponse BootstrapManager::onBootstrapMessage(const ExtClrMsg &msg) {
    TRACE_METHOD();
    BootstrapMessage bMsg = parseMsg(msg);
    std::string sender = msg.getFrom();
    switch (bMsg.type) {
        case BootstrapMessageType::LINK_CREATE_REQUEST:
            return handleLinkCreateRequest(bMsg, sender);
        case BootstrapMessageType::LINK_CREATE_RESPONSE:
            return handleLinkCreateResponse(bMsg, sender);
        case BootstrapMessageType::ADD_PERSONA:
            return handleAddPersona(bMsg, sender);
        case BootstrapMessageType::LINK_LOAD_REQUEST:
            return handleLinkLoadRequest(bMsg, sender);
        case BootstrapMessageType::LINK_LOAD_REQUEST_FORWARD:
            return handleLinkLoadRequestForward(bMsg, sender);
        case BootstrapMessageType::DESTROY_LINK:
            return handleDestroyLink(bMsg, sender);
        default:
            logError("Invalid bootstrap message type for onBootstrapMessage: " +
                     stringFromBootstrapMessageType(bMsg.type));
            return PLUGIN_ERROR;
    }
}

PluginResponse BootstrapManager::onBootstrapPackage(
    const std::string &persona, const ExtClrMsg &msg,
    const std::vector<std::string> &entranceCommittee) {
    TRACE_METHOD(persona);
    BootstrapMessage bMsg = parseMsg(msg);
    if (bMsg.type == BootstrapMessageType::BOOTSTRAP_PACKAGE) {
        return handleBootstrapPackage(bMsg, entranceCommittee);
    } else {
        logError("Invalid bootstrap message type for onBootstrapPackage: " +
                 stringFromBootstrapMessageType(bMsg.type));
    }
    return PLUGIN_ERROR;
}

PluginResponse BootstrapManager::onBootstrapStart(const std::string &introducer,
                                                  const std::vector<std::string> &entranceCommittee,
                                                  uint64_t handle) {
    TRACE_METHOD(introducer);

    // NOTE: only called on target
    // set these and wait for the bootstrap connection to be opened
    bootstrapIntroducer = introducer;
    bootstrapHandle = handle;
    bootstrapEntranceCommittee = entranceCommittee;

    return PLUGIN_OK;
}

void BootstrapManager::onLinkStatusChanged(RaceHandle handle, LinkID linkId, LinkStatus status,
                                           LinkProperties properties) {
    TRACE_METHOD(handle, linkId);
    // naively check all bootstraps. This is normally empty, so it shouldn't take long
    for (auto it = outstandingCreateLinks.begin(); it != outstandingCreateLinks.end(); ++it) {
        bool found = false;
        logDebug("BootstrapManager::onLinkStatusChanged checking outstandingCreateLink handle: " +
                 std::to_string(it->message.bootstrapHandle) + ", dest: " + it->dest);

        // check if this handle was in response to one of these createLink requests
        for (auto it2 = it->outstandingHandles.begin(); it2 != it->outstandingHandles.end();
             ++it2) {
            if (*it2 == handle) {
                logDebug("onLinkStatusChanged: found matching handle");
                found = true;
                it->outstandingHandles.erase(it2);
                if (status == LINK_CREATED) {
                    it->message.linkAddresses.push_back(properties.linkAddress);
                    it->message.channelGids.push_back(properties.channelGid);
                    if (it->message.type == LINK_CREATE_RESPONSE) {
                        linksToUpdate[it->message.bootstrapHandle].push_back(linkId);
                    }
                } else {
                    logWarning("Bootstrap requested link failed to be created");
                }
                break;
            }
        }

        if (found) {
            // If all the requested links were created, send a response back to the sender node
            if (it->outstandingHandles.empty()) {
                logDebug("onBootstrapStart: created all links for " +
                         stringFromBootstrapMessageType(it->message.type) + " message");
                sendBootstrapMsg(it->message, it->dest);
                outstandingCreateLinks.erase(it);
            }
            break;
        }
    }

    // Once we're done creating all links to the entrance committee and sent them to the
    // introducer to be forwarded, we can destroy the bootstrap link connection to the introducer.
    destroyBootstrapLinkIfComplete();
}

PluginResponse BootstrapManager::onConnectionStatusChanged(RaceHandle handle,
                                                           const ConnectionID &connId,
                                                           ConnectionStatus status,
                                                           const LinkID &linkId,
                                                           const LinkProperties & /*properties*/) {
    TRACE_METHOD(handle, connId, linkId);
    auto personas = plugin->getSdk()->getPersonasForLink(linkId);

    if (!bootstrapIntroducer.empty() && !personas.empty() && personas[0] == bootstrapIntroducer) {
        // target case
        if (status == CONNECTION_OPEN) {
            // NOTE this should never be called on the introducer
            return handleBootstrapConnectionOpened(connId);
        } else if (status == CONNECTION_CLOSED && connId == bootstrapConnectionId) {
            // If we've closed the bootstrap link connection, we can destroy the link since we're
            // done bootstrapping
            logDebug("Bootstrap connection has been closed, destroying bootstrap link");
            SdkResponse resp = plugin->getSdk()->destroyLink(linkId, RACE_UNLIMITED);
            if (resp.status != SDK_OK) {
                logError("Received sdk error from destroyLink");
            }

            // Clean up the bootstrap info
            bootstrapConnectionId.clear();
            bootstrapIntroducer.clear();
            bootstrapEntranceCommittee.clear();
        } else {
            logError(
                "Connection to bootstrap node failed to open. Cannot send bootstrap package. This "
                "is a fatal error.");
            return PLUGIN_FATAL;
        }
    } else {
        // introducer case
        // Check if this link ID is a bootstrap link for any outstanding bootstraps we're
        // introducing
        for (auto it = outstandingBootstraps.begin(); it != outstandingBootstraps.end(); ++it) {
            if (it->outstandingOpenConnectionHandle == handle) {
                if (status == CONNECTION_OPEN) {
                    // Save the connection ID for later so we can close it
                    logDebug("Bootstrap connection is open: " + connId);
                    it->bootstrapConnectionId = connId;
                } else {
                    logError("Bootstrap connection failed to open");
                }
                break;
            } else if (it->bootstrapConnectionId == connId) {
                if (status == CONNECTION_CLOSED) {
                    // If we've closed the bootstrap link connection, we can destroy the link since
                    // we're done bootstrapping
                    logDebug("Bootstrap connection has been closed, destroying bootstrap link");
                    SdkResponse resp = plugin->getSdk()->destroyLink(linkId, RACE_UNLIMITED);
                    if (resp.status != SDK_OK) {
                        logError("Received sdk error from destroyLink");
                    }

                    // Clean up the bootstrap info
                    outstandingBootstraps.erase(it);
                }
                break;
            }
        }
    }

    return PLUGIN_OK;
}

void BootstrapManager::onPackageStatusChanged(RaceHandle handle, PackageStatus status,
                                              RaceHandle resendHandle) {
    TRACE_METHOD(handle);
    if (handle == bootstrapDestroyLinkPackageHandle) {
        logDebug("Package status for destroy-bootstrap-link message = " + std::to_string(status) +
                 ", handle = " + std::to_string(handle));

        if (bootstrapConnectionId.empty()) {
            logWarning("No bootstrap connection ID, no action necessary");
            return;
        }

        if (status == PACKAGE_SENT) {
            logDebug("Closing bootstrap link connection");
            // Link deletion will occur when the connection closed callback occurs
            SdkResponse resp =
                plugin->getSdk()->closeConnection(bootstrapConnectionId, RACE_UNLIMITED);
            if (resp.status != SDK_OK) {
                logError("Received sdk error from closeConnection");
            }
        } else if (resendHandle != NULL_RACE_HANDLE) {
            logDebug("Destroy-bootstrap-link package was re-sent, new handle = " +
                     std::to_string(resendHandle));
            bootstrapDestroyLinkPackageHandle = resendHandle;
        } else {
            logError("Destroy-bootstrap-link package failed and was not resent");
        }
    }
}

PluginResponse BootstrapManager::handleBootstrapConnectionOpened(const ConnectionID &connId) {
    TRACE_METHOD(connId);
    BootstrapMessage package;
    package.type = BootstrapMessageType::BOOTSTRAP_PACKAGE;
    package.bootstrapHandle = bootstrapHandle;
    package.persona = plugin->getUuid();
    package.key = base64_encode(plugin->getAesKeyForSelf());

    sendBootstrapPkg(package, bootstrapIntroducer, connId);

    for (const std::string &com : bootstrapEntranceCommittee) {
        logDebug(logPrefix + "creating links for " + com);
        OutstandingCreateLink requestStatus;
        requestStatus.message.messageHandle = messageCounter++;
        requestStatus.message.bootstrapHandle = bootstrapHandle;
        requestStatus.message.type = LINK_LOAD_REQUEST;
        requestStatus.message.persona = com;

        // send to introducer for forwarding
        requestStatus.dest = bootstrapIntroducer;

        auto supportedChannels = plugin->getSdk()->getSupportedChannels();
        for (auto &channelGid : plugin->getExpectedChannels(com)) {
            auto iter = supportedChannels.find(channelGid);
            if (iter == supportedChannels.end()) {
                logWarning(logPrefix + "skipping channel " + channelGid +
                           " because it is not a supported channel");
            } else if (iter->second.connectionType != CT_INDIRECT ||
                       iter->second.linkDirection == LD_BIDI) {
                logDebug(logPrefix + "skipping channel " + channelGid +
                         " because the channel not indirect or is bidirectional");
            } else if (not(iter->second.currentRole.linkSide == LS_CREATOR or
                           iter->second.currentRole.linkSide == LS_BOTH)) {
                logWarning(logPrefix + "skipping channel " + channelGid +
                           " because the current role does not allow creating links on this node");
            } else if (channelLinksFull(plugin->getSdk(), channelGid)) {
                logDebug(logPrefix + "skipping channel " + channelGid +
                         " because the channel has reached the max number of links");
            } else {
                logDebug(logPrefix + "requesting link for channel " + channelGid);
                SdkResponse resp = plugin->getLinkManager()->createLink(channelGid, {com});
                if (resp.status == SDK_OK) {
                    requestStatus.outstandingHandles.push_back(resp.handle);
                } else {
                    logError(logPrefix + "Received sdk error from createLink");
                }
            }
        }

        if (!requestStatus.outstandingHandles.empty()) {
            outstandingCreateLinks.push_back(requestStatus);
        }
    }

    // Save the connection ID for later so we can close it
    bootstrapConnectionId = connId;

    // If no additional links needed to be created, we can destroy the bootstrap link connection to
    // the introducer.
    destroyBootstrapLinkIfComplete();

    return PLUGIN_OK;
}

PluginResponse BootstrapManager::handleLinkCreateRequest(const BootstrapMessage &bMsg,
                                                         const std::string &sender) {
    TRACE_METHOD(sender);
    OutstandingCreateLink requestStatus;
    requestStatus.message.type = LINK_CREATE_RESPONSE;
    requestStatus.message.bootstrapHandle = bMsg.bootstrapHandle;
    requestStatus.message.messageHandle = bMsg.messageHandle;
    requestStatus.message.persona = plugin->getUuid();
    requestStatus.dest = sender;

    auto supportedChannels = plugin->getSdk()->getSupportedChannels();
    for (auto &channelGid : bMsg.channelGids) {
        auto iter = supportedChannels.find(channelGid);
        if (iter == supportedChannels.end()) {
            logWarning(logPrefix + "skipping channel " + channelGid +
                       " because it is not a supported channel");
        } else if (iter->second.connectionType != CT_INDIRECT) {
            logWarning(logPrefix + "skipping channel " + channelGid +
                       " because the channel is not indirect");
        } else if (not(iter->second.currentRole.linkSide == LS_CREATOR or
                       iter->second.currentRole.linkSide == LS_BOTH)) {
            logWarning(logPrefix + "skipping channel " + channelGid +
                       " because the current role does not allow creating links on this node");
        } else if (channelLinksFull(plugin->getSdk(), channelGid)) {
            logWarning(logPrefix + "skipping channel " + channelGid +
                       " because the channel has reached the max number of links");
        } else {
            logDebug(logPrefix + "creating link for " + channelGid);
            SdkResponse resp = plugin->getLinkManager()->createLink(channelGid, {});
            if (resp.status == SDK_OK) {
                requestStatus.outstandingHandles.push_back(resp.handle);
            } else {
                logError(logPrefix + "Received sdk error from createLink");
            }
        }
    }

    if (!requestStatus.outstandingHandles.empty()) {
        outstandingCreateLinks.push_back(requestStatus);
    }

    return PLUGIN_OK;
}

PluginResponse BootstrapManager::handleLinkCreateResponse(const BootstrapMessage &bMsg,
                                                          const std::string &sender) {
    TRACE_METHOD(sender);
    bool found = false;
    for (auto it = outstandingBootstraps.begin(); it != outstandingBootstraps.end(); ++it) {
        for (auto it2 = it->outstandingHandles.begin(); it2 != it->outstandingHandles.end();
             ++it2) {
            if (*it2 == bMsg.messageHandle) {
                logDebug("handleLinkCreateResponse: found matching handle");
                found = true;
                it->outstandingHandles.erase(it2);

                if (bMsg.linkAddresses.size() != bMsg.channelGids.size()) {
                    logError(
                        "handleLinkCreateResponse: mismatched sizes of link addresses and "
                        "channelGids: linkAddresses.size()=" +
                        std::to_string(bMsg.linkAddresses.size()) +
                        ", channelGids.size()=" + std::to_string(bMsg.channelGids.size()));
                    break;
                }

                for (size_t i = 0; i < bMsg.channelGids.size(); ++i) {
                    it->receivedLinks.push_back(
                        {bMsg.linkAddresses[i], bMsg.channelGids[i], bMsg.persona});
                }
                break;
            }
        }

        if (found) {
            // If all the requests have sent a response, write the config file
            if (it->outstandingHandles.empty()) {
                writeConfigs(*it);
            }
            break;
        }
    }

    if (!found) {
        logWarning("Received unexpected LINK_CREATE_RESPONSE");
    }

    return PLUGIN_OK;
}

PluginResponse BootstrapManager::handleBootstrapPackage(
    const BootstrapMessage &bMsg, const std::vector<std::string> &entranceCommittee) {
    TRACE_METHOD();
    for (const std::string &com : entranceCommittee) {
        BootstrapMessage forward;
        forward.type = BootstrapMessageType::ADD_PERSONA;
        forward.bootstrapHandle = bMsg.bootstrapHandle;
        forward.persona = bMsg.persona;
        forward.key = bMsg.key;

        sendBootstrapMsg(forward, com);
    }

    return PLUGIN_OK;
}
PluginResponse BootstrapManager::handleAddPersona(const BootstrapMessage &bMsg,
                                                  const std::string &sender) {
    TRACE_METHOD(sender);

    RawData rawKey = base64_decode(bMsg.key);
    plugin->addClient(bMsg.persona, rawKey);

    auto it = linksToUpdate.find(bMsg.bootstrapHandle);
    if (it == linksToUpdate.end()) {
        logWarning("handleAddPersona: no links found to update with new persona");
        return PLUGIN_ERROR;
    }

    for (LinkID &linkId : it->second) {
        SdkResponse resp = plugin->getLinkManager()->setPersonasForLink(linkId, {bMsg.persona});
        if (resp.status != SDK_OK) {
            logError("handleAddPersona: Received sdk error from setPersonasForLink");
            break;
        }

        LinkProperties props = plugin->getSdk()->getLinkProperties(linkId);
        if (props.linkType == LT_UNDEF) {
            logError("handleAddPersona: link properties have undefined link type");
            break;
        }

        bool success = plugin->openConnectionsForLink(linkId, props);
        if (!success) {
            logError("handleAddPersona: Failed to open connection for link to bootstrapped node");
            break;
        }
    }

    linksToUpdate.erase(it);

    return PLUGIN_OK;
}

PluginResponse BootstrapManager::handleLinkLoadRequest(const BootstrapMessage &bMsg,
                                                       const std::string &sender) {
    TRACE_METHOD(sender);
    BootstrapMessage forward;
    forward.type = BootstrapMessageType::LINK_LOAD_REQUEST_FORWARD;

    // For LINK_LOAD_REQUEST, the persona is the destination node. For LINK_LOAD_REQUEST_FORWARD, it
    // is the sender node.
    forward.persona = sender;
    forward.linkAddresses = bMsg.linkAddresses;
    forward.channelGids = bMsg.channelGids;

    sendBootstrapMsg(forward, bMsg.persona);
    return PLUGIN_OK;
}

PluginResponse BootstrapManager::handleLinkLoadRequestForward(const BootstrapMessage &bMsg,
                                                              const std::string &sender) {
    TRACE_METHOD(sender);

    std::vector<std::string> channels = bMsg.channelGids;
    std::vector<std::string> links = bMsg.linkAddresses;
    if (channels.size() != links.size()) {
        logError(
            "BootstrapManager::handleLinkLoadRequestForward: Mismatched sizes. "
            "bMsg.channelGids.size()=" +
            std::to_string(channels.size()) +
            ",  bMsg.linkAddresses.size()=" + std::to_string(links.size()));
        return PLUGIN_ERROR;
    }

    for (size_t i = 0; i < channels.size(); ++i) {
        if (channelLinksFull(plugin->getSdk(), channels[i])) {
            logError(
                "BootstrapManager::handleLinkLoadRequestForward: cannot load link for channel " +
                channels[i] + " because the channel has reached the max number of links");
            channels.erase(channels.begin() +
                           static_cast<std::vector<std::string>::difference_type>(i));
            links.erase(links.begin() + static_cast<std::vector<std::string>::difference_type>(i));
        }
    }

    if (channels.size() == 0) {
        logError(
            "BootstrapManager::handleLinkLoadRequestForward: no valid channels to load links for");
        return PLUGIN_ERROR;
    }

    for (size_t i = 0; i < channels.size(); ++i) {
        SdkResponse resp =
            plugin->getLinkManager()->loadLinkAddress(channels[i], links[i], {bMsg.persona});
        if (resp.status != SDK_OK) {
            logError(
                "BootstrapManager::handleLinkLoadRequestForward: Received sdk error from "
                "loadLinkAddress");
        }
    }
    return PLUGIN_OK;
}

PluginResponse BootstrapManager::handleDestroyLink(const BootstrapMessage &msg,
                                                   const std::string &sender) {
    TRACE_METHOD(sender);
    bool found = false;
    for (auto it = outstandingBootstraps.begin(); it != outstandingBootstraps.end(); ++it) {
        if (it->bootstrapHandle == msg.bootstrapHandle) {
            logDebug("handleDestroyLink: found matching bootstrap handle");
            found = true;

            if (it->bootstrapConnectionId.empty()) {
                logError("handleDestroyLink: bootstrap does not have a connection");
            } else {
                // Link deletion will occur when the connection closed callback occurs
                SdkResponse resp =
                    plugin->getSdk()->closeConnection(it->bootstrapConnectionId, RACE_UNLIMITED);
                if (resp.status != SDK_OK) {
                    logError("handleDestroyLink: received sdk error from closeConnection");
                }
            }
        }
    }

    if (!found) {
        logWarning("Received unexpected DESTROY_LINK");
    }

    return PLUGIN_OK;
}

void BootstrapManager::destroyBootstrapLinkIfComplete() {
    // Only destroy the bootstrap link once all link creations to the entrance committee have
    // been completed
    if (outstandingCreateLinks.empty() && !bootstrapConnectionId.empty()) {
        logDebug("Notifying introducer to destroy the bootstrap link");
        BootstrapMessage message;
        message.type = BootstrapMessageType::DESTROY_LINK;
        message.bootstrapHandle = bootstrapHandle;
        message.persona = plugin->getUuid();
        // Connection close will occur when the package sent callback occurs
        bootstrapDestroyLinkPackageHandle = sendBootstrapMsg(message, bootstrapIntroducer);

        // Write updated configs to disk without bootstrap info
        plugin->writeConfigs();
    } else if (!bootstrapConnectionId.empty()) {
        logDebug("Still waiting on " + std::to_string(outstandingCreateLinks.size()) +
                 " links to be created before destroying the bootstrap link");
    }
}

RaceHandle BootstrapManager::sendBootstrapMsg(const BootstrapMessage &bMsg,
                                              const std::string &destination) {
    TRACE_METHOD();
    ExtClrMsg msg = createClrMsg(bMsg, destination);
    std::string msgString = plugin->getEncryptor().formatDelimitedMessage(msg);
    return plugin->sendFormattedMsg(destination, msgString, msg.getTraceId(), msg.getSpanId());
}

void BootstrapManager::sendBootstrapPkg(const BootstrapMessage &bMsg,
                                        const std::string &destination,
                                        const ConnectionID &connId) {
    TRACE_METHOD();
    ExtClrMsg msg = createClrMsg(bMsg, destination);
    std::string msgString = plugin->getEncryptor().formatDelimitedMessage(msg);
    plugin->sendBootstrapPkg(connId, destination, msgString);
}

void BootstrapManager::writeConfigs(const OutstandingBootstrap &bootstrapInfo) {
    using json = nlohmann::json;
    TRACE_METHOD();

    try {
        json config;

        // deduplicate the list of channels
        std::set<std::string> channelSet;
        std::transform(bootstrapInfo.receivedLinks.begin(), bootstrapInfo.receivedLinks.end(),
                       std::inserter(channelSet, channelSet.begin()),
                       [](const LinkInfo &linkInfo) { return linkInfo.channel; });

        for (const std::string &channel : channelSet) {
            auto links = json::array();
            for (const LinkInfo &linkInfo : bootstrapInfo.receivedLinks) {
                if (channel == linkInfo.channel) {
                    links.push_back({
                        {"address", linkInfo.address},
                        {"personas", {linkInfo.persona}},
                        {"description", ""},
                        {"role", "loader"},
                    });
                }
            }
            config[channel] = links;
        }

        const int jsonIndentLevel = 4;
        std::string linkProfilesStr = config.dump(jsonIndentLevel);
        plugin->getSdk()->writeFile(bootstrapInfo.configPath + "/link-profiles.json",
                                    {linkProfilesStr.begin(), linkProfilesStr.end()});

        json committeeConfig = JsonIO::loadJson(*plugin->getSdk(), "config.json");
        committeeConfig["bootstrapIntroducer"] = plugin->getUuid();
        committeeConfig["bootstrapHandle"] = bootstrapInfo.bootstrapHandle;

        std::string committeeConfigStr = committeeConfig.dump(jsonIndentLevel);
        plugin->getSdk()->writeFile(bootstrapInfo.configPath + "/config.json",
                                    {committeeConfigStr.begin(), committeeConfigStr.end()});

        std::vector<std::string> channels;
        for (auto &channel : plugin->getSdk()->getAllChannelProperties()) {
            if (channel.channelStatus != CHANNEL_UNSUPPORTED) {
                channels.push_back(channel.channelGid);
            }
        }
        plugin->getSdk()->bootstrapDevice(bootstrapInfo.sdkHandle, channels);
    } catch (std::exception &e) {
        logError("Got exception" + std::string(e.what()));
    }
}

bool BootstrapManager::isBootstrapConnection(const ConnectionID &connId) {
    return connId == bootstrapConnectionId;
}

BootstrapManager::BootstrapMessage BootstrapManager::parseMsg(const ExtClrMsg &msg) {
    using json = nlohmann::json;
    TRACE_METHOD();
    if (msg.getMsgType() != MSG_BOOTSTRAPPING) {
        logError("BootstrapManager::parseMsg: Tried to parse non-bootstrap message");
        return {};
    }

    json msgJson = json::parse(msg.getMsg());

    BootstrapMessage bMsg;

    try {
        bMsg.type = bootstrapMessageTypeFromString(msgJson.at("type").get<std::string>());
        bMsg.bootstrapHandle = msgJson.at("bootstrapHandle").get<uint64_t>();
        bMsg.messageHandle = msgJson.at("messageHandle").get<uint64_t>();
        bMsg.linkAddresses = msgJson.at("linkAddresses").get<std::vector<std::string>>();
        bMsg.channelGids = msgJson.at("channelGids").get<std::vector<std::string>>();
        bMsg.persona = msgJson.at("persona").get<std::string>();
        bMsg.key = msgJson.at("key").get<std::string>();
    } catch (std::exception &e) {
        logError("Failed to parse message json" + std::string(e.what()));
    }

    return bMsg;
}

ExtClrMsg BootstrapManager::createClrMsg(const BootstrapMessage &bMsg, const std::string &dest) {
    using json = nlohmann::json;
    TRACE_METHOD();
    json msgJson;
    msgJson["type"] = stringFromBootstrapMessageType(bMsg.type);
    msgJson["bootstrapHandle"] = bMsg.bootstrapHandle;
    msgJson["messageHandle"] = bMsg.messageHandle;
    msgJson["linkAddresses"] = bMsg.linkAddresses;
    msgJson["channelGids"] = bMsg.channelGids;
    msgJson["persona"] = bMsg.persona;
    msgJson["key"] = bMsg.key;
    ExtClrMsg msg(msgJson.dump(), plugin->getUuid(), dest, 1, 0, 0, 0, 0, 0, MSG_BOOTSTRAPPING);
    return msg;
}

BootstrapManager::BootstrapMessageType BootstrapManager::bootstrapMessageTypeFromString(
    const std::string &typeString) {
    if (typeString == "LINK_CREATE_REQUEST") {
        return BootstrapMessageType::LINK_CREATE_REQUEST;
    } else if (typeString == "LINK_CREATE_RESPONSE") {
        return BootstrapMessageType::LINK_CREATE_RESPONSE;
    } else if (typeString == "BOOTSTRAP_PACKAGE") {
        return BootstrapMessageType::BOOTSTRAP_PACKAGE;
    } else if (typeString == "ADD_PERSONA") {
        return BootstrapMessageType::ADD_PERSONA;
    } else if (typeString == "LINK_LOAD_REQUEST") {
        return BootstrapMessageType::LINK_LOAD_REQUEST;
    } else if (typeString == "LINK_LOAD_REQUEST_FORWARD") {
        return BootstrapMessageType::LINK_LOAD_REQUEST_FORWARD;
    } else if (typeString == "DESTROY_LINK") {
        return BootstrapMessageType::DESTROY_LINK;
    }
    return BootstrapMessageType::UNDEFINED;
}

std::string BootstrapManager::stringFromBootstrapMessageType(BootstrapMessageType type) {
    switch (type) {
        case BootstrapMessageType::LINK_CREATE_REQUEST:
            return "LINK_CREATE_REQUEST";
        case BootstrapMessageType::LINK_CREATE_RESPONSE:
            return "LINK_CREATE_RESPONSE";
        case BootstrapMessageType::BOOTSTRAP_PACKAGE:
            return "BOOTSTRAP_PACKAGE";
        case BootstrapMessageType::ADD_PERSONA:
            return "ADD_PERSONA";
        case BootstrapMessageType::LINK_LOAD_REQUEST:
            return "LINK_LOAD_REQUEST";
        case BootstrapMessageType::LINK_LOAD_REQUEST_FORWARD:
            return "LINK_LOAD_REQUEST_FORWARD";
        case BootstrapMessageType::DESTROY_LINK:
            return "DESTROY_LINK";
        default:
            return "UNDEFINED";
    }
}