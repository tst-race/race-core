
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

#include "IndirectChannel.h"

#include <climits>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "TwosixWhiteboardLink.h"
#include "TwosixWhiteboardLinkProfileParser.h"

const std::string IndirectChannel::indirectChannelGid = "twoSixIndirectCpp";

IndirectChannel::IndirectChannel(PluginCommsTwoSixCpp &plugin) :
    Channel(plugin, indirectChannelGid),
    whiteboardHostname("twosix-whiteboard"),
    whiteboardPort(5000) {}

IndirectChannel::IndirectChannel(PluginCommsTwoSixCpp &_plugin, const std::string &_channelGid) :
    Channel(_plugin, _channelGid), whiteboardHostname("twosix-whiteboard"), whiteboardPort(5000) {}

LinkProperties IndirectChannel::getDefaultLinkProperties() {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_BIDI;
    linkProperties.transmissionType = properties.transmissionType;
    linkProperties.connectionType = properties.connectionType;
    linkProperties.sendType = properties.sendType;
    linkProperties.reliable = properties.reliable;
    linkProperties.isFlushable = properties.isFlushable;
    linkProperties.duration_s = properties.duration_s;
    linkProperties.period_s = properties.period_s;
    linkProperties.mtu = properties.mtu;

    LinkPropertySet worstLinkPropertySet;
    worstLinkPropertySet.bandwidth_bps = 277200;
    worstLinkPropertySet.latency_ms = 3190;
    worstLinkPropertySet.loss = 0.1;
    linkProperties.worst.send = worstLinkPropertySet;
    linkProperties.worst.receive = worstLinkPropertySet;

    linkProperties.expected = properties.creatorExpected;

    LinkPropertySet bestLinkPropertySet;
    bestLinkPropertySet.bandwidth_bps = 338800;
    bestLinkPropertySet.latency_ms = 2610;
    bestLinkPropertySet.loss = 0.1;
    linkProperties.best.send = bestLinkPropertySet;
    linkProperties.best.receive = bestLinkPropertySet;

    linkProperties.supported_hints = properties.supported_hints;
    linkProperties.channelGid = channelGid;

    return linkProperties;
}

PluginResponse IndirectChannel::activateChannelInternal(RaceHandle handle) {
    const std::string logPrefix = "activateChannelInternal (handle: " + std::to_string(handle) +
                                  " channel GID: " + channelGid + "): ";

    logInfo(logPrefix);

    status = CHANNEL_AVAILABLE;
    plugin.raceSdk->onChannelStatusChanged(handle, channelGid, status, properties, RACE_BLOCKING);
    plugin.raceSdk->displayInfoToUser(channelGid + " is available", RaceEnums::UD_TOAST);

    return PLUGIN_OK;
}

std::shared_ptr<Link> IndirectChannel::createLink(const LinkID &linkId) {
    LinkProperties linkProps = linkProperties;
    linkProps.linkType = LT_BIDI;

    TwosixWhiteboardLinkProfileParser parser;
    parser.hostname = whiteboardHostname;
    parser.port = whiteboardPort;
    parser.hashtag = "cpp_" + plugin.racePersona + "_" + std::to_string(nextAvailableHashTag++);
    parser.checkFrequency = 1000;
    std::chrono::duration<double> sinceEpoch =
        std::chrono::high_resolution_clock::now().time_since_epoch();
    parser.timestamp = sinceEpoch.count();
    parser.maxTries = 600;

    const auto link = std::make_shared<TwosixWhiteboardLink>(plugin.raceSdk, &plugin, this, linkId,
                                                             linkProps, parser);

    return link;
}

std::shared_ptr<Link> IndirectChannel::createLinkFromAddress(const LinkID &linkId,
                                                             const std::string &linkAddress) {
    // The TwoSix Whiteboard channel uses the same implementation for createLinkFromAddress and
    // loadLink. This may not be the case for all channels. Please see twoSixDirectCpp for a
    // different example.
    return loadLink(linkId, linkAddress);
}

std::shared_ptr<Link> IndirectChannel::loadLink(const LinkID &linkId,
                                                const std::string &linkAddress) {
    // copy default link properties and set link type
    LinkProperties linkProps = linkProperties;
    linkProps.linkType = LT_BIDI;

    const auto link = std::make_shared<TwosixWhiteboardLink>(plugin.raceSdk, &plugin, this, linkId,
                                                             linkProps, linkAddress);

    return link;
}
