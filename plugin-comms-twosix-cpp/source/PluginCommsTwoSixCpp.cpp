
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

#include "PluginCommsTwoSixCpp.h"

#include <RaceLog.h>

#include <algorithm>
#include <chrono>  // std::chrono::seconds
#include <map>
#include <nlohmann/json.hpp>
#include <thread>  // std::this_thread::sleep_for

#include "base/Channel.h"
#include "base/LinkProfileParser.h"
#include "direct/DirectChannel.h"
#include "direct/DirectLink.h"
#include "filesystem.h"
#include "utils/log.h"
#include "whiteboard/TwosixWhiteboardLink.h"

PluginCommsTwoSixCpp::PluginCommsTwoSixCpp(IRaceSdkComms *raceSdkIn) : raceSdk(raceSdkIn) {
    logDebug("PluginCommsTwoSixCpp::PluginCommsTwoSixCpp()");
    if (raceSdk == nullptr) {
        throw std::invalid_argument("Race SDK provided to Comms plugin is nullptr");
    }
}

PluginCommsTwoSixCpp::~PluginCommsTwoSixCpp() {
    // Right now, we assume links get destroyed before channels. This should be fine, as shutdown is
    // suppose to be called before we exit, but clean things up here just in case
    connections.clear();
    links.clear();
}

PluginResponse PluginCommsTwoSixCpp::init(const PluginConfig &_pluginConfig) {
    logInfo("init called");
    logInfo("etcDirectory: " + _pluginConfig.etcDirectory);
    logInfo("loggingDirectory: " + _pluginConfig.loggingDirectory);
    logInfo("auxDataDirectory: " + _pluginConfig.auxDataDirectory);
    logInfo("tmpDirectory: " + _pluginConfig.tmpDirectory);
    logInfo("pluginDirectory: " + _pluginConfig.pluginDirectory);
    this->pluginConfig = _pluginConfig;

    if (not _pluginConfig.auxDataDirectory.empty()) {
        logDebug("  contents of " + _pluginConfig.auxDataDirectory + ":");
        for (auto &iter : fs::recursive_directory_iterator(_pluginConfig.auxDataDirectory)) {
            logDebug(iter.path().string());
        }
    }

    channels = Channel::createChannels(*this);

    // Configuring Persona
    racePersona = raceSdk->getActivePersona();
    logDebug("    active persona: " + racePersona);

    /* Example of read/write API usage */
    std::string init_msg = "Comms CPP Plugin Initialized\n";
    SdkResponse response =
        raceSdk->writeFile("initialized.txt", {init_msg.begin(), init_msg.end()});
    if (response.status != SDK_OK) {
        logWarning("Failed to write to plugin storage");
    }
    std::vector<uint8_t> tmp = raceSdk->readFile("initialized.txt");
    std::string read = std::string(tmp.begin(), tmp.end());
    logDebug("Read Initialization File: " + read);

    return PLUGIN_OK;
}

PluginResponse PluginCommsTwoSixCpp::shutdown() {
    logInfo("shutdown: called");

    std::vector<ConnectionID> connIds;
    {
        std::lock_guard<std::mutex> lock(linksLock);

        for (const auto &connection : connections) {
            // cppcheck-suppress useStlAlgorithm
            connIds.push_back(connection.first);
        }
    }

    for (const auto &connId : connIds) {
        closeConnection(NULL_RACE_HANDLE, connId);
    }

    std::lock_guard<std::mutex> lock(linksLock);
    for (auto &link : links) {
        link.second->shutdown();
    }

    logInfo("shutdown: returned");
    return PLUGIN_OK;
}

void PluginCommsTwoSixCpp::addLink(const std::shared_ptr<Link> &link) {
    std::lock_guard<std::mutex> lock(linksLock);
    links[link->getId()] = link;
    raceSdk->updateLinkProperties(link->getId(), link->getProperties(), RACE_BLOCKING);
}

std::shared_ptr<Link> PluginCommsTwoSixCpp::getLink(const LinkID &linkId) {
    std::lock_guard<std::mutex> lock(linksLock);
    return links.at(linkId);
}

std::shared_ptr<Connection> PluginCommsTwoSixCpp::getConnection(const ConnectionID &connectionId) {
    std::lock_guard<std::mutex> lock(connectionLock);
    return connections.at(connectionId);
}

PluginResponse PluginCommsTwoSixCpp::sendPackage(RaceHandle handle, ConnectionID connectionId,
                                                 EncPkg pkg, double timeoutTimestamp,
                                                 uint64_t /*batchId*/) {
    const std::string loggingPrefix = "sendPackage (" + connectionId + "): ";
    logInfo(loggingPrefix + "called");

    try {
        auto conn = getConnection(connectionId);

        if (conn->linkType != LT_SEND && conn->linkType != LT_BIDI) {
            logError("Trying to send on a connection with invalid link type: link type: " +
                     std::to_string(static_cast<int>(conn->linkType)));
            return PLUGIN_ERROR;
        }

        return conn->getLink()->sendPackage(handle, pkg, timeoutTimestamp);
    } catch (std::out_of_range &e) {
        logError(loggingPrefix + "Failed to get connection: " + connectionId);
        raceSdk->onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING);
    } catch (std::bad_weak_ptr &e) {
        logError(loggingPrefix + "Failed to get link from connection: " + connectionId);
        raceSdk->onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING);
    }
    return PLUGIN_ERROR;
}

PluginResponse PluginCommsTwoSixCpp::openConnection(RaceHandle handle, LinkType linkType,
                                                    LinkID linkId, std::string linkHints,
                                                    int32_t sendTimeout) {
    const std::string loggingPrefix = "openConnection: ";
    logInfo(loggingPrefix + " called");
    logDebug(loggingPrefix + " type:        " + std::to_string(static_cast<int>(linkType)));
    logDebug(loggingPrefix + " link hints: " + linkHints);

    const std::string newConnectionId = raceSdk->generateConnectionId(linkId);

    std::shared_ptr<Link> link = nullptr;

    try {
        link = getLink(linkId);
    } catch (std::out_of_range &e) {
        logError(loggingPrefix + "Failed to get link: " + linkId);
        raceSdk->onConnectionStatusChanged(handle, newConnectionId, CONNECTION_CLOSED, {},
                                           RACE_BLOCKING);
        return PLUGIN_ERROR;
    }

    // TODO: this assumes linkProperties.linkType is constant, can we enforce that?
    if (link->getProperties().linkType != linkType && link->getProperties().linkType != LT_BIDI) {
        logError(loggingPrefix + "Tried to open link with mismatched Link type.");
        raceSdk->onConnectionStatusChanged(handle, newConnectionId, CONNECTION_CLOSED,
                                           link->getProperties(), RACE_BLOCKING);
        return PLUGIN_ERROR;
    }

    std::shared_ptr<Connection> newConnection =
        link->openConnection(linkType, newConnectionId, linkHints, sendTimeout);

    if (newConnection == nullptr) {
        logError(loggingPrefix + "Failed to create connection.");
        raceSdk->onConnectionStatusChanged(handle, newConnectionId, CONNECTION_CLOSED,
                                           link->getProperties(), RACE_BLOCKING);
        return PLUGIN_ERROR;
    }

    {
        std::lock_guard<std::mutex> lock(connectionLock);
        connections[newConnection->connectionId] = newConnection;
    }

    raceSdk->onConnectionStatusChanged(handle, newConnection->connectionId, CONNECTION_OPEN,
                                       link->getProperties(), RACE_BLOCKING);

    link->startConnection(newConnection.get());

    if (!link->isAvailable()) {
        raceSdk->onConnectionStatusChanged(handle, newConnection->connectionId,
                                           CONNECTION_UNAVAILABLE, link->getProperties(),
                                           RACE_BLOCKING);
    }

    logInfo(loggingPrefix + " returned");
    return PLUGIN_OK;
}

PluginResponse PluginCommsTwoSixCpp::closeConnection(RaceHandle handle, ConnectionID connectionId) {
    logInfo("closeConnection called");
    logDebug("    ID: " + connectionId);

    std::shared_ptr<Link> link = nullptr;
    try {
        std::lock_guard<std::mutex> lock(connectionLock);
        auto connection = connections.at(connectionId);
        link = connection->getLink();
        connections.erase(connectionId);
    } catch (std::out_of_range &e) {
        // This may happen if the receiveThread and the plugin thread both try to close the
        // connection
        logWarning("No connection by the given ID can be found to close");
        return PLUGIN_OK;
    } catch (std::bad_weak_ptr &e) {
        logError("Connection has invalid link");
        raceSdk->onConnectionStatusChanged(handle, connectionId, CONNECTION_CLOSED,
                                           link->getProperties(), RACE_BLOCKING);
        return PLUGIN_ERROR;
    }

    link->closeConnection(connectionId);

    raceSdk->onConnectionStatusChanged(handle, connectionId, CONNECTION_CLOSED,
                                       link->getProperties(), RACE_BLOCKING);
    logInfo("closeConnection returned");
    return PLUGIN_OK;
}

PluginResponse PluginCommsTwoSixCpp::destroyLink(RaceHandle handle, LinkID linkId) {
    const std::string logPrefix =
        "destroyLink: (handle: " + std::to_string(handle) + " link ID: " + linkId + "): ";
    logDebug(logPrefix + "called");

    std::shared_ptr<Link> link = nullptr;
    try {
        link = getLink(linkId);
    } catch (std::out_of_range &e) {
        logError(logPrefix + "link with ID does not exist");
        return PLUGIN_ERROR;
    }

    // Calls SDK API onLinkStatusChanged to nofity that link has been destroyed and calls
    // onConnectionStatusChanged to notify that all connections in link have been closed.
    link->shutdown();
    std::lock_guard<std::mutex> lock(linksLock);
    links.erase(linkId);

    logDebug(logPrefix + "returned");
    return PLUGIN_OK;
}

PluginResponse PluginCommsTwoSixCpp::createLink(RaceHandle handle, std::string channelGid) {
    try {
        return channels.at(channelGid)->createLink(handle);
    } catch (std::exception &e) {
        logError("createLink: got exception: " + std::string(e.what()));
    }

    return PLUGIN_ERROR;
}

PluginResponse PluginCommsTwoSixCpp::loadLinkAddress(RaceHandle handle, std::string channelGid,
                                                     std::string linkAddress) {
    try {
        return channels.at(channelGid)->loadLinkAddress(handle, linkAddress);
    } catch (std::exception &e) {
        logError("loadLinkAddress: got exception: " + std::string(e.what()));
    }

    return PLUGIN_ERROR;
}

PluginResponse PluginCommsTwoSixCpp::loadLinkAddresses(RaceHandle handle, std::string channelGid,
                                                       std::vector<std::string> linkAddresses) {
    try {
        return channels.at(channelGid)->loadLinkAddresses(handle, linkAddresses);
    } catch (std::exception &e) {
        logError("loadLinkAddresses: got exception: " + std::string(e.what()));
    }

    return PLUGIN_ERROR;
}

PluginResponse PluginCommsTwoSixCpp::createLinkFromAddress(RaceHandle handle,
                                                           std::string channelGid,
                                                           std::string linkAddress) {
    logDebug("createLinkFromAddress: channelGid: " + channelGid + " linkAddress: " + linkAddress);
    try {
        return channels.at(channelGid)->createLinkFromAddress(handle, linkAddress);
    } catch (std::exception &e) {
        logError("createLink: got exception: " + std::string(e.what()));
    }

    return PLUGIN_ERROR;
}

PluginResponse PluginCommsTwoSixCpp::activateChannel(RaceHandle handle, std::string channelGid,
                                                     std::string roleName) {
    (void)roleName;
    try {
        return channels.at(channelGid)->activateChannel(handle);
    } catch (std::exception &e) {
        logError("activateChannel: got exception: " + std::string(e.what()));
    }

    return PLUGIN_ERROR;
}

PluginResponse PluginCommsTwoSixCpp::deactivateChannel(RaceHandle handle, std::string channelGid) {
    try {
        return channels.at(channelGid)->deactivateChannel(handle);
    } catch (std::exception &e) {
        logError("deactivateChannel: got exception: " + std::string(e.what()));
    }

    return PLUGIN_ERROR;
}

PluginResponse PluginCommsTwoSixCpp::createBootstrapLink(RaceHandle handle, std::string channelGid,
                                                         std::string passphrase) {
    try {
        return channels.at(channelGid)->createBootstrapLink(handle, passphrase);
    } catch (std::exception &e) {
        logError("createBootstrapLink: got exception: " + std::string(e.what()));
    }

    return PLUGIN_ERROR;
}

PluginResponse PluginCommsTwoSixCpp::serveFiles(LinkID linkId, std::string path) {
    const std::string logPrefix = "serveFiles: (link ID: " + linkId + ", path: " + path + "): ";
    logDebug(logPrefix + "called");

    std::shared_ptr<Link> link = nullptr;
    try {
        link = getLink(linkId);
        return link->serveFiles(path);
    } catch (std::out_of_range &e) {
        logError(logPrefix + "got exception: " + std::string(e.what()));
        return PLUGIN_ERROR;
    }
}

PluginResponse PluginCommsTwoSixCpp::flushChannel(RaceHandle /*handle*/, std::string /*channelGid*/,
                                                  uint64_t /*batchId*/) {
    logError("flushChannel: plugin does not support flushing");
    return PLUGIN_ERROR;
}

PluginResponse PluginCommsTwoSixCpp::onUserInputReceived(RaceHandle handle, bool answered,
                                                         const std::string &response) {
    const std::string logPrefix = "onUserInputReceived (handle: " + std::to_string(handle) + "): ";
    logDebug(logPrefix + "called");
    bool responseHandled = false;
    for (auto const &channel : channels) {
        // cppcheck-suppress useStlAlgorithm
        // Short circuit if the responce was already handled. Everything should rely on handle to
        // determine if this is the response to a previous request.
        responseHandled =
            responseHandled or channel.second->onUserInputReceived(handle, answered, response);
    }

    if (!responseHandled) {
        logWarning(logPrefix + "handle is not recognized");
        return PLUGIN_ERROR;
    }

    logDebug(logPrefix + "returned");
    return PLUGIN_OK;
}

std::vector<std::shared_ptr<Link>> PluginCommsTwoSixCpp::linksForChannel(
    const std::string &channelGid) {
    std::vector<std::shared_ptr<Link>> channelLinks;
    std::lock_guard<std::mutex> lock(linksLock);
    for (auto &link : links) {
        if (link.second->getProperties().channelGid == channelGid) {
            channelLinks.push_back(link.second);
        }
    }

    return channelLinks;
}

PluginResponse PluginCommsTwoSixCpp::onUserAcknowledgementReceived(RaceHandle) {
    logDebug("onUserAcknowledgementReceived called");
    return PLUGIN_OK;
}

const PluginConfig &PluginCommsTwoSixCpp::getPluginConfig() const {
    return pluginConfig;
}

#ifndef TESTBUILD
IRacePluginComms *createPluginComms(IRaceSdkComms *sdk) {
    return new PluginCommsTwoSixCpp(sdk);
}

void destroyPluginComms(IRacePluginComms *plugin) {
    delete static_cast<PluginCommsTwoSixCpp *>(plugin);
}

const RaceVersionInfo raceVersion = RACE_VERSION;
const char *const racePluginId = "PluginCommsTwoSixStub";
const char *const racePluginDescription = "Plugin Comms Stub (Two Six Labs) " BUILD_VERSION;
#endif
