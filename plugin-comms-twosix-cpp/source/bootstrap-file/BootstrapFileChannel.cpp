
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

#include "BootstrapFileChannel.h"

#include <climits>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "BootstrapFileLink.h"
#include "BootstrapFileLinkProfileParser.h"

const std::string BootstrapFileChannel::bootstrapFileChannelGid = "twoSixBootstrapFileCpp";

BootstrapFileChannel::BootstrapFileChannel(PluginCommsTwoSixCpp &plugin) :
    Channel(plugin, bootstrapFileChannelGid),
    directory("no-directory-provided-by-user"),
    watcher(plugin) {}

LinkProperties BootstrapFileChannel::getDefaultLinkProperties() {
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
    worstLinkPropertySet.bandwidth_bps = 1000;
    worstLinkPropertySet.latency_ms = INT_MAX;
    worstLinkPropertySet.loss = -1.0;
    linkProperties.worst.send = worstLinkPropertySet;
    linkProperties.worst.receive = worstLinkPropertySet;

    linkProperties.expected = properties.creatorExpected;

    LinkPropertySet bestLinkPropertySet;
    bestLinkPropertySet.bandwidth_bps = 1000;
    bestLinkPropertySet.latency_ms = INT_MAX;
    bestLinkPropertySet.loss = -1.0;
    linkProperties.best.send = bestLinkPropertySet;
    linkProperties.best.receive = bestLinkPropertySet;

    linkProperties.supported_hints = properties.supported_hints;
    linkProperties.channelGid = BootstrapFileChannel::bootstrapFileChannelGid;

    linkProperties.linkType = LT_BIDI;

    return linkProperties;
}

std::shared_ptr<Link> BootstrapFileChannel::createLink(const LinkID &linkId) {
    LinkProperties linkProps = linkProperties;

    BootstrapFileLinkProfileParser parser;
    parser.directory = directory;

    const auto link = std::make_shared<BootstrapFileLink>(plugin.raceSdk, &plugin, this, linkId,
                                                          linkProps, parser);
    return link;
}

std::shared_ptr<Link> BootstrapFileChannel::createLinkFromAddress(const LinkID &linkId,
                                                                  const std::string &linkAddress) {
    LinkProperties linkProps = linkProperties;

    BootstrapFileLinkProfileParser parser(linkAddress);
    parser.directory = directory;

    const auto link = std::make_shared<BootstrapFileLink>(plugin.raceSdk, &plugin, this, linkId,
                                                          linkProps, parser);
    return link;
}

std::shared_ptr<Link> BootstrapFileChannel::createBootstrapLink(
    const LinkID &linkId, const std::string & /* passphrase */) {
    return createLink(linkId);
}

std::shared_ptr<Link> BootstrapFileChannel::loadLink(const LinkID &linkId,
                                                     const std::string &linkAddress) {
    // copy default link properties and set link type and loader expected
    LinkProperties linkProps = linkProperties;
    linkProps.expected = properties.loaderExpected;
    BootstrapFileLinkProfileParser parser(linkAddress);
    parser.directory = directory;

    const auto link = std::make_shared<BootstrapFileLink>(plugin.raceSdk, &plugin, this, linkId,
                                                          linkProps, parser);
    return link;
}

PluginResponse BootstrapFileChannel::activateChannelInternal(RaceHandle handle) {
    const std::string logPrefix = "activateChannelInternal (handle: " + std::to_string(handle) +
                                  " channel GID: " + bootstrapFileChannelGid + "): ";

    logInfo(logPrefix + "called from BootstrapFileCpp");

    SdkResponse response = plugin.raceSdk->requestPluginUserInput(
        "directory", "What directory should be used for saving packages?", true);
    if (response.status != SDK_OK) {
        logError("Failed to request hostname from user, bootstrapFile channel cannot be used");
        status = CHANNEL_FAILED;
        plugin.raceSdk->onChannelStatusChanged(handle, bootstrapFileChannelGid, status, properties,
                                               RACE_BLOCKING);
    }
    requestDirectoryHandle = response.handle;
    return PLUGIN_OK;
}

bool BootstrapFileChannel::onUserInputReceived(RaceHandle handle, bool answered,
                                               const std::string &response) {
    const std::string logPrefix = "onUserInputReceived (handle: " + std::to_string(handle) + "): ";
    logDebug(logPrefix + " got hanedle: " + std::to_string(handle) + ", " +
             "expecting handle: " + std::to_string(requestDirectoryHandle));
    if (handle == requestDirectoryHandle) {
        if (answered) {
            directory = response;
            logInfo(logPrefix + "using directory " + directory);
            watcher.start(directory + "/receive");
            status = CHANNEL_AVAILABLE;
            plugin.raceSdk->onChannelStatusChanged(NULL_RACE_HANDLE, bootstrapFileChannelGid,
                                                   status, properties, RACE_BLOCKING);
        } else {
            logError(logPrefix + "bootstrapFile channel not available without the directory");
            status = CHANNEL_DISABLED;
            plugin.raceSdk->onChannelStatusChanged(NULL_RACE_HANDLE, bootstrapFileChannelGid,
                                                   status, properties, RACE_BLOCKING);
        }

        return true;
    }
    return false;
}