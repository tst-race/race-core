
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

#include "BootstrapChannel.h"

#include <climits>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "../direct/DirectLinkProfileParser.h"
#include "BootstrapLink.h"

const std::string BootstrapChannel::bootstrapChannelGid = "twoSixBootstrapCpp";

BootstrapChannel::BootstrapChannel(PluginCommsTwoSixCpp &plugin) :
    Channel(plugin, bootstrapChannelGid),
    server(plugin.getPluginConfig().tmpDirectory),
    hostname("no-hostname-provided-by-user"),
    portAllocator(10000, 30000) {}

LinkProperties BootstrapChannel::getDefaultLinkProperties() {
    LinkProperties linkProperties;

    linkProperties.transmissionType = properties.transmissionType;
    linkProperties.connectionType = properties.connectionType;
    linkProperties.sendType = properties.sendType;
    linkProperties.reliable = properties.reliable;
    linkProperties.isFlushable = properties.isFlushable;
    linkProperties.duration_s = properties.duration_s;
    linkProperties.period_s = properties.period_s;
    linkProperties.mtu = properties.mtu;

    LinkPropertySet worstLinkPropertySet;
    worstLinkPropertySet.bandwidth_bps = 23130000;
    worstLinkPropertySet.latency_ms = 17;
    worstLinkPropertySet.loss = -1.0;
    linkProperties.worst.send = worstLinkPropertySet;
    linkProperties.worst.receive = worstLinkPropertySet;

    linkProperties.expected = properties.creatorExpected;

    LinkPropertySet bestLinkPropertySet;
    bestLinkPropertySet.bandwidth_bps = 28270000;
    bestLinkPropertySet.latency_ms = 14;
    bestLinkPropertySet.loss = -1.0;
    linkProperties.best.send = bestLinkPropertySet;
    linkProperties.best.receive = bestLinkPropertySet;

    linkProperties.supported_hints = properties.supported_hints;
    linkProperties.channelGid = BootstrapChannel::bootstrapChannelGid;

    return linkProperties;
}

PluginResponse BootstrapChannel::activateChannelInternal(RaceHandle handle) {
    const std::string logPrefix = "activateChannelInternal (handle: " + std::to_string(handle) +
                                  " channel GID: " + bootstrapChannelGid + "): ";

    logInfo(logPrefix);

    status = CHANNEL_STARTING;
    plugin.raceSdk->onChannelStatusChanged(handle, bootstrapChannelGid, status, properties,
                                           RACE_BLOCKING);

    SdkResponse response = plugin.raceSdk->requestPluginUserInput(
        "startPort", "What is the first available port?", true);
    if (response.status != SDK_OK) {
        logWarning("Failed to request start port from user");
    }
    requestStartPortHandle = response.handle;
    userRequestHandles.insert(requestStartPortHandle);

    response =
        plugin.raceSdk->requestPluginUserInput("endPort", "What is the last available port?", true);
    if (response.status != SDK_OK) {
        logWarning("Failed to request end port from user");
    }
    requestEndPortHandle = response.handle;
    userRequestHandles.insert(requestEndPortHandle);

    response = plugin.raceSdk->requestCommonUserInput("hostname");
    if (response.status != SDK_OK) {
        logWarning("Failed to request hostname from user");
    }
    requestHostnameHandle = response.handle;
    userRequestHandles.insert(requestHostnameHandle);

    return PLUGIN_OK;
}

bool BootstrapChannel::onUserInputReceived(RaceHandle handle, bool answered,
                                           const std::string &response) {
    const std::string logPrefix = "onUserInputReceived (handle: " + std::to_string(handle) + "): ";
    bool responseHandled = false;
    if (handle == requestHostnameHandle) {
        if (answered) {
            hostname = response;
            logInfo(logPrefix + "using hostname " + hostname);
        } else {
            logError(logPrefix + "bootstrap channel not available without the hostname");
            plugin.raceSdk->onChannelStatusChanged(NULL_RACE_HANDLE, bootstrapChannelGid,
                                                   ChannelStatus::CHANNEL_DISABLED, properties,
                                                   RACE_BLOCKING);
            status = CHANNEL_DISABLED;
            // do not continue handling input
            return true;
        }
        responseHandled = true;
    } else if (handle == requestStartPortHandle) {
        if (answered) {
            int port = std::stoi(response);
            logInfo(logPrefix + "using start port " + std::to_string(port));
            portAllocator.setPortRangeStart(static_cast<uint16_t>(port));
        } else {
            logWarning(logPrefix + "no answer, using default start port");
        }
        responseHandled = true;
    } else if (handle == requestEndPortHandle) {
        if (answered) {
            int port = std::stoi(response);
            logInfo(logPrefix + "using end port " + std::to_string(port));
            portAllocator.setPortRangeEnd(static_cast<uint16_t>(port));
        } else {
            logWarning(logPrefix + "no answer, using default end port");
        }
        responseHandled = true;
    }

    // Check if all requests have been fulfilled
    if (responseHandled) {
        userRequestHandles.erase(handle);
        if (userRequestHandles.size() == 0) {
            // all requests have been handled, channel is now availabled
            status = CHANNEL_AVAILABLE;
            plugin.raceSdk->onChannelStatusChanged(NULL_RACE_HANDLE, bootstrapChannelGid, status,
                                                   properties, RACE_BLOCKING);
        }
    }
    return responseHandled;
}

std::shared_ptr<Link> BootstrapChannel::createLink(const LinkID &linkId) {
    return createBootstrapLink(linkId, "");
}

std::shared_ptr<Link> BootstrapChannel::createBootstrapLink(const LinkID &linkId,
                                                            const std::string &passphrase) {
    LinkProperties linkProps = linkProperties;
    linkProps.linkType = LT_RECV;

    DirectLinkProfileParser parser;
    parser.hostname = hostname;
    parser.port = portAllocator.getAvailablePort();

    const auto link = std::make_shared<BootstrapLink>(plugin.raceSdk, &plugin, this, linkId,
                                                      linkProps, parser, passphrase);
    return link;
}

std::shared_ptr<Link> BootstrapChannel::createLinkFromAddress(const LinkID &linkId,
                                                              const std::string &linkAddress) {
    LinkProperties linkProps = linkProperties;
    linkProps.linkType = LT_RECV;

    DirectLinkProfileParser parser(linkAddress);

    const auto link = std::make_shared<BootstrapLink>(plugin.raceSdk, &plugin, this, linkId,
                                                      linkProps, parser, "");
    return link;
}

std::shared_ptr<Link> BootstrapChannel::loadLink(const LinkID &linkId,
                                                 const std::string &linkAddress) {
    // copy default link properties and set link type
    LinkProperties linkProps = linkProperties;
    linkProps.linkType = LT_SEND;

    DirectLinkProfileParser parser(linkAddress);

    const auto link =
        std::make_shared<BootstrapLink>(plugin.raceSdk, &plugin, this, linkId, linkProps, parser);
    return link;
}

void BootstrapChannel::onLinkDestroyedInternal(Link *link) {
    const auto linkProps = link->getProperties();
    if (linkProps.linkType == LT_RECV || linkProps.linkType == LT_BIDI) {
        portAllocator.releasePort(
            nlohmann::json::parse(link->getLinkAddress())["port"].get<uint16_t>());
    }
}

void BootstrapChannel::onGenesisLinkCreated(Link *link) {
    const auto linkProps = link->getProperties();
    if (linkProps.linkType == LT_RECV || linkProps.linkType == LT_BIDI) {
        portAllocator.usePort(
            nlohmann::json::parse(link->getLinkAddress())["port"].get<uint16_t>());
    }
}
