
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

#include "Channel.h"

#include <nlohmann/json.hpp>

#include "../PluginCommsTwoSixCpp.h"
#include "../bootstrap-file/BootstrapFileChannel.h"
#include "../bootstrap-indirect/IndirectBootstrapChannel.h"
#include "../bootstrap/BootstrapChannel.h"
#include "../direct/DirectChannel.h"
#include "../filesystem.h"
#include "../whiteboard/IndirectChannel.h"

Channel::Channel(PluginCommsTwoSixCpp &_plugin, const std::string &_channelGid) :
    channelGid(_channelGid), plugin(_plugin), status(CHANNEL_UNAVAILABLE), numLinks(0) {}

Channel::~Channel() {}

std::string Channel::preLinkCreate(const std::string &logLabel, RaceHandle handle,
                                   LinkSide invalidRole) {
    const LinkID linkId = plugin.raceSdk->generateLinkId(channelGid);

    if (status != CHANNEL_AVAILABLE) {
        logError(logLabel + "preLinkCreate: channel not available.");
        plugin.raceSdk->onLinkStatusChanged(handle, linkId, LINK_DESTROYED, linkProperties,
                                            RACE_BLOCKING);
        return {};
    }

    if (numLinks >= properties.maxLinks) {
        logError(logLabel + "preLinkCreate: Too many links. links: " + std::to_string(numLinks) +
                 ", maxLinks: " + std::to_string(properties.maxLinks));
        plugin.raceSdk->onLinkStatusChanged(handle, linkId, LINK_DESTROYED, linkProperties,
                                            RACE_BLOCKING);
        return {};
    }

    if (properties.currentRole.linkSide == LS_UNDEF ||
        properties.currentRole.linkSide == invalidRole) {
        logError(logLabel + "preLinkCreate: Invalid role for this call. currentRole: '" +
                 properties.currentRole.roleName +
                 "'' linkSide: " + linkSideToString(properties.currentRole.linkSide));
        plugin.raceSdk->onLinkStatusChanged(handle, linkId, LINK_DESTROYED, linkProperties,
                                            RACE_BLOCKING);
        return {};
    }

    return linkId;
}

PluginResponse Channel::postLinkCreate(const std::string &logLabel, RaceHandle handle,
                                       const std::string &linkId, const std::shared_ptr<Link> &link,
                                       LinkStatus linkStatus) {
    if (link == nullptr) {
        logError(logLabel + "postLinkCreate: link was null");
        plugin.raceSdk->onLinkStatusChanged(handle, linkId, LINK_DESTROYED, linkProperties,
                                            RACE_BLOCKING);
        return PLUGIN_ERROR;
    }

    plugin.raceSdk->onLinkStatusChanged(handle, linkId, linkStatus, link->getProperties(),
                                        RACE_BLOCKING);
    plugin.addLink(link);
    numLinks++;

    return PLUGIN_OK;
}

PluginResponse Channel::createLink(RaceHandle handle) {
    const std::string logPrefix =
        "createLink (handle: " + std::to_string(handle) + " channel GID: " + channelGid + "): ";
    logDebug(logPrefix + "called");

    const std::string linkId = preLinkCreate(logPrefix, handle, LS_LOADER);
    if (linkId.empty()) {
        return PLUGIN_OK;
    }
    const auto link = createLink(linkId);
    PluginResponse resp = postLinkCreate(logPrefix, handle, linkId, link, LINK_CREATED);

    logDebug(logPrefix + "returned");
    return resp;
}

PluginResponse Channel::createLinkFromAddress(RaceHandle handle, const std::string &linkAddress) {
    const std::string logPrefix = "createLinkFromAddress (handle: " + std::to_string(handle) +
                                  " channel GID: " + channelGid + "): ";
    logDebug(logPrefix + "called");

    const std::string linkId = preLinkCreate(logPrefix, handle, LS_LOADER);
    if (linkId.empty()) {
        return PLUGIN_OK;
    }
    const auto link = createLinkFromAddress(linkId, linkAddress);
    PluginResponse resp = postLinkCreate(logPrefix, handle, linkId, link, LINK_CREATED);

    logDebug(logPrefix + "returned");
    return resp;
}

PluginResponse Channel::loadLinkAddress(RaceHandle handle, const std::string &linkAddress) {
    const std::string logPrefix = "loadLinkAddress (handle: " + std::to_string(handle) +
                                  " channel GID: " + channelGid + "): ";
    logDebug(logPrefix + "called with link address: " + linkAddress);

    const std::string linkId = preLinkCreate(logPrefix, handle, LS_CREATOR);
    if (linkId.empty()) {
        return PLUGIN_OK;
    }
    const auto link = loadLink(linkId, linkAddress);
    PluginResponse resp = postLinkCreate(logPrefix, handle, linkId, link, LINK_LOADED);

    logDebug(logPrefix + "returned");
    return resp;
}

PluginResponse Channel::loadLinkAddresses(RaceHandle handle,
                                          const std::vector<std::string> &linkAddresses) {
    const std::string logPrefix = "loadLinkAddresses: (handle: " + std::to_string(handle) +
                                  " channel GID: " + channelGid + "): ";
    logDebug(logPrefix + "called");
    if (linkAddresses.size() == 0) {
        logWarning(logPrefix + "no link addresses provided");
        plugin.raceSdk->onLinkStatusChanged(handle, "", LINK_DESTROYED, linkProperties,
                                            RACE_BLOCKING);
        return PLUGIN_ERROR;
    }

    // None of our channels support multiple address loading
    if (status != CHANNEL_AVAILABLE) {
        logError(logPrefix + "channel not available.");
        plugin.raceSdk->onLinkStatusChanged(handle, "", LINK_DESTROYED, linkProperties,
                                            RACE_BLOCKING);
        return PLUGIN_ERROR;
    }

    if (!properties.multiAddressable) {
        logError(logPrefix + "API not supported for this channel");
        plugin.raceSdk->onLinkStatusChanged(handle, "", LINK_DESTROYED, linkProperties,
                                            RACE_BLOCKING);
        return PLUGIN_ERROR;
    }

    logDebug(logPrefix + "returned");
    return PLUGIN_OK;
}

PluginResponse Channel::activateChannel(RaceHandle handle) {
    properties = plugin.raceSdk->getChannelProperties(channelGid);
    linkProperties = getDefaultLinkProperties();
    return activateChannelInternal(handle);
}

PluginResponse Channel::deactivateChannel(RaceHandle handle) {
    const std::string logPrefix = "deactivateChannel (handle: " + std::to_string(handle) +
                                  " channel GID: " + channelGid + "): ";

    logInfo(logPrefix);

    status = CHANNEL_UNAVAILABLE;
    plugin.raceSdk->onChannelStatusChanged(handle, channelGid, status, properties, RACE_BLOCKING);

    for (const auto &link : plugin.linksForChannel(channelGid)) {
        plugin.destroyLink(handle, link->getId());
    }

    return PLUGIN_OK;
}

PluginResponse Channel::createBootstrapLink(RaceHandle handle, const std::string &passphrase) {
    const std::string logPrefix = "createBootstrapLink (handle: " + std::to_string(handle) +
                                  " channel GID: " + channelGid + "): ";
    logDebug(logPrefix + "called");

    const std::string linkId = preLinkCreate(logPrefix, handle, LS_UNDEF);
    if (linkId.empty()) {
        return PLUGIN_OK;
    }
    const auto link = createBootstrapLink(linkId, passphrase);
    PluginResponse resp = postLinkCreate(logPrefix, handle, linkId, link, LINK_CREATED);

    logDebug(logPrefix + "returned");
    return resp;
}

void Channel::onLinkDestroyed(Link *link) {
    numLinks--;
    onLinkDestroyedInternal(link);
}

bool Channel::onUserInputReceived(RaceHandle, bool, const std::string &) {
    // default onUserInputReceived returns false to show it did not handle any response
    return false;
}

std::shared_ptr<Link> Channel::createBootstrapLink(const LinkID & /*linkId*/,
                                                   const std::string & /*passphrase*/) {
    logError("createBootstrapLink not implemented for channel: " + channelGid);
    return nullptr;
}

void Channel::onLinkDestroyedInternal(Link * /*link*/) {}

void Channel::onGenesisLinkCreated(Link * /*link*/) {}

std::unordered_map<std::string, std::shared_ptr<Channel>> Channel::createChannels(
    PluginCommsTwoSixCpp &plugin) {
    return {{DirectChannel::directChannelGid, std::make_shared<DirectChannel>(plugin)},
            {IndirectChannel::indirectChannelGid, std::make_shared<IndirectChannel>(plugin)},
            {BootstrapChannel::bootstrapChannelGid, std::make_shared<BootstrapChannel>(plugin)},
            {BootstrapFileChannel::bootstrapFileChannelGid,
             std::make_shared<BootstrapFileChannel>(plugin)},
            {IndirectBootstrapChannel::indirectBootstrapChannelGid,
             std::make_shared<IndirectBootstrapChannel>(plugin)}};
}
