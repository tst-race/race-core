
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

#include "LinkManager.h"

#include "ConfigStaticLinks.h"
#include "Log.h"
#include "PluginNMTwoSix.h"
#include "helper.h"

void LinkManager::init(const std::unordered_map<std::string, std::string> &roles) {
    TRACE_METHOD();
    channelRoles = roles;
    activateChannels(roles);
    readLinkProfiles();
}

PluginResponse LinkManager::onChannelStatusChanged(RaceHandle /*handle*/,
                                                   const std::string &channelGid,
                                                   ChannelStatus status) {
    TRACE_METHOD(channelGid, status);
    if (status == ChannelStatus::CHANNEL_AVAILABLE) {
        if (channelsAwaitingActivation.count(channelGid) > 0) {
            initStaticLinks(channelGid);
            channelsAwaitingActivation.erase(channelGid);
            checkStaticLinksCreated();
        } else {
            logWarning(logPrefix + "unexpected channel activated");
            return PLUGIN_ERROR;
        }
    } else if (status == ChannelStatus::CHANNEL_ENABLED) {
        auto it = channelRoles.find(channelGid);
        if (it != channelRoles.end()) {
            logDebug(logPrefix + "Activating channel: " + channelGid + " role: " + it->second);
            plugin->getSdk()->activateChannel(channelGid, it->second, RACE_BLOCKING);
            channelsAwaitingActivation.insert(channelGid);
        } else {
            logWarning(logPrefix + "No role available for channel: " + channelGid);
        }
    } else if (status == ChannelStatus::CHANNEL_FAILED) {
        logError(logPrefix +
                 "Received CHANNEL_FAILED. Handling this is "
                 "unsupported by the TwoSix exemplars.");
        return PLUGIN_FATAL;
    }
    return PLUGIN_OK;
}

PluginResponse LinkManager::onLinkStatusChanged(RaceHandle handle, const LinkID &linkId,
                                                LinkStatus status,
                                                const LinkProperties &properties) {
    TRACE_METHOD(handle, linkId, status);

    if (status == LINK_CREATED) {
        // Check if this was created from createLink
        auto pendingCreateIter = pendingCreateLinks.find(handle);
        if (pendingCreateIter != pendingCreateLinks.end()) {
            auto linkProfileIter =
                addLinkProfile(properties.channelGid, properties.linkAddress,
                               pendingCreateIter->second.role, pendingCreateIter->second.personas);
            linkIdToProfileIter[linkId] = linkProfileIter;
            pendingCreateLinks.erase(pendingCreateIter);
        } else {
            // Else it was created from createLinkFromAddress
            auto pendingIter = pendingLinks.find(handle);
            if (pendingIter != pendingLinks.end()) {
                linkIdToProfileIter[linkId] = pendingIter->second;
                pendingLinks.erase(pendingIter);
            }
        }
    } else if (status == LINK_LOADED) {
        auto pendingIter = pendingLinks.find(handle);
        if (pendingIter != pendingLinks.end()) {
            linkIdToProfileIter[linkId] = pendingIter->second;
            pendingLinks.erase(pendingIter);
        }
    } else if (status == LINK_DESTROYED) {
        removeLinkProfile(properties.channelGid, linkId);
        pendingCreateLinks.erase(handle);
        pendingLinks.erase(handle);
    }

    if (staticLinkRequests.erase(handle) == 1) {
        checkStaticLinksCreated();
    }

    return PLUGIN_OK;
}

SdkResponse LinkManager::setPersonasForLink(const std::string &linkId,
                                            const std::vector<std::string> &personas) {
    TRACE_METHOD(linkId, personas);
    LinkProperties props = plugin->getSdk()->getLinkProperties(linkId);

    auto channelIter = linkProfiles.find(props.channelGid);
    if (channelIter == linkProfiles.end()) {
        logError(logPrefix + "Unable to find channelGid " + props.channelGid + " in linkProfiles");
        return SDK_INVALID_ARGUMENT;
    }

    auto linkProfileIter = linkIdToProfileIter.find(linkId);
    if (linkProfileIter == linkIdToProfileIter.end()) {
        logError(logPrefix + "Unable to find linkId " + linkId + " in linkIdToProfileIter");
        return SDK_INVALID_ARGUMENT;
    }

    linkProfileIter->second->personas = personas;
    writeLinkProfiles();

    return plugin->getSdk()->setPersonasForLink(linkId, personas);
}

bool linkSideMatchesRole(LinkSide linkSide, const std::string &role) {
    // clang-format off
    return (linkSide == LS_BOTH) or
           (linkSide == LS_CREATOR and role == "creator") or
           (linkSide == LS_LOADER and role == "loader");
    // clang-format on
}

bool LinkManager::hasLink(const std::vector<std::string> &personas, LinkType linkType,
                          const std::string &channelGid, LinkSide linkSide) {
    TRACE_METHOD(personas, linkType, channelGid, linkSide);

    auto iter = linkProfiles.find(channelGid);
    if (iter == linkProfiles.end()) {
        return false;
    }

    for (auto &linkProfile : iter->second) {
        if (linkProfile.personas == personas and linkSideMatchesRole(linkSide, linkProfile.role)) {
            return true;
        }
    }

    return false;
}

SdkResponse LinkManager::createLink(const std::string &channelGid,
                                    const std::vector<std::string> &personas) {
    TRACE_METHOD(channelGid, personas);

    // The link address is not available at this point. Add it to a set of pending links that will
    // be completed when the onLinkStatusChanged call is received with LINK_CREATED
    auto response = plugin->getSdk()->createLink(channelGid, personas, 0);
    if (response.status == SDK_OK) {
        addPendingCreateLink(response.handle, "creator", personas);
    }

    return response;
}

SdkResponse LinkManager::createLinkFromAddress(const std::string &channelGid,
                                               const std::string &linkAddress,
                                               const std::vector<std::string> &personas) {
    TRACE_METHOD(channelGid, linkAddress, personas);
    auto linkProfileIter = addLinkProfile(channelGid, linkAddress, "creator", personas);
    return createLinkFromAddress(channelGid, linkProfileIter);
}

SdkResponse LinkManager::createLinkFromAddress(const std::string &channelGid,
                                               const LinkProfileListIter &linkProfileIter) {
    auto response = plugin->getSdk()->createLinkFromAddress(channelGid, linkProfileIter->address,
                                                            linkProfileIter->personas, 0);
    if (response.status == SDK_OK) {
        pendingLinks[response.handle] = linkProfileIter;
    }
    return response;
}

SdkResponse LinkManager::loadLinkAddress(const std::string &channelGid,
                                         const std::string &linkAddress,
                                         const std::vector<std::string> &personas) {
    TRACE_METHOD(channelGid, linkAddress);
    auto linkProfileIter = addLinkProfile(channelGid, linkAddress, "loader", personas);
    return loadLinkAddress(channelGid, linkProfileIter);
}

SdkResponse LinkManager::loadLinkAddress(const std::string &channelGid,
                                         const LinkProfileListIter &linkProfileIter) {
    auto response = plugin->getSdk()->loadLinkAddress(channelGid, linkProfileIter->address,
                                                      linkProfileIter->personas, 0);
    if (response.status == SDK_OK) {
        pendingLinks[response.handle] = linkProfileIter;
    }
    return response;
}

SdkResponse LinkManager::loadLinkAddresses(const std::string &channelGid,
                                           const std::vector<std::string> &linkAddresses,
                                           const std::vector<std::string> &personas) {
    TRACE_METHOD(channelGid, linkAddresses, personas);
    auto linkProfileIter = addLinkProfile(channelGid, linkAddresses, "loader", personas);
    return loadLinkAddresses(channelGid, linkProfileIter);
}

SdkResponse LinkManager::loadLinkAddresses(const std::string &channelGid,
                                           const LinkProfileListIter &linkProfileIter) {
    auto response = plugin->getSdk()->loadLinkAddresses(channelGid, linkProfileIter->address_list,
                                                        linkProfileIter->personas, 0);
    if (response.status == SDK_OK) {
        pendingLinks[response.handle] = linkProfileIter;
    }
    return response;
}

auto LinkManager::addLinkProfile(const std::string &channelGid, const std::string &address,
                                 const std::string &role, const std::vector<std::string> &personas)
    -> LinkProfileListIter {
    TRACE_METHOD(channelGid, address, role, personas);

    LinkProfile linkProfile;
    linkProfile.address = address;
    linkProfile.personas = personas;
    linkProfile.role = role;

    // Pushed to the front so we can use begin to get the iterator to the newly added entry
    linkProfiles[channelGid].push_front(linkProfile);
    writeLinkProfiles();

    return linkProfiles[channelGid].begin();
}

auto LinkManager::addLinkProfile(const std::string &channelGid,
                                 const std::vector<std::string> &addresses, const std::string &role,
                                 const std::vector<std::string> &personas) -> LinkProfileListIter {
    TRACE_METHOD(channelGid, addresses, role, personas);

    LinkProfile linkProfile;
    linkProfile.address_list = addresses;
    linkProfile.personas = personas;
    linkProfile.role = role;

    // Pushed to the front so we can use begin to get the iterator to the newly added entry
    linkProfiles[channelGid].push_front(linkProfile);
    writeLinkProfiles();

    return linkProfiles[channelGid].begin();
}

void LinkManager::removeLinkProfile(const std::string &channelGid, const std::string &linkId) {
    TRACE_METHOD(channelGid, linkId);
    auto linkProfileIter = linkIdToProfileIter.find(linkId);
    if (linkProfileIter != linkIdToProfileIter.end()) {
        linkProfiles[channelGid].erase(linkProfileIter->second);
        linkIdToProfileIter.erase(linkId);
    }
    writeLinkProfiles();
}

void LinkManager::addPendingCreateLink(RaceHandle handle, const std::string &role,
                                       const std::vector<std::string> &personas) {
    TRACE_METHOD(handle, role, personas);

    LinkProfile linkProfile;
    linkProfile.personas = personas;
    linkProfile.role = role;

    pendingCreateLinks[handle] = linkProfile;
}

void LinkManager::activateChannels(const std::unordered_map<std::string, std::string> &roles) {
    TRACE_METHOD();
    std::vector<ChannelProperties> channels = plugin->getSdk()->getAllChannelProperties();
    if (channels.size() == 0) {
        logWarning(logPrefix + "received zero channels from sdk->getAllChannelProperties()");
    }
    for (ChannelProperties channel : channels) {
        auto it = roles.find(channel.channelGid);
        if (channel.channelStatus == CHANNEL_ENABLED && it != roles.end()) {
            logDebug(logPrefix + "Activating channel: " + channel.channelGid +
                     " role: " + it->second);
            plugin->getSdk()->activateChannel(channel.channelGid, it->second, RACE_BLOCKING);
            // Expect all channels in use to be "available" prior sending messages
            // network managers can be smarter about this and start with a subset of enabled
            // channels
            channelsAwaitingActivation.insert(channel.channelGid);
        } else if (channel.roles.size() == 0) {
            logWarning(logPrefix + "No roles available for channel: " + channel.channelGid);
        }
    }

    if (roles.size() != channelsAwaitingActivation.size()) {
        logError(logPrefix + "Expected to activate " + std::to_string(roles.size()) +
                 " roles. "
                 "Activated " +
                 std::to_string(channelsAwaitingActivation.size()) + " roles");
    }
}

void LinkManager::readLinkProfiles() {
    TRACE_METHOD();
    linkProfiles = ConfigStaticLinks::loadLinkProfiles(*plugin->getSdk(), "link-profiles.json");
}

bool LinkManager::initStaticLinks(const std::string &channelGid) {
    TRACE_METHOD(channelGid);

    if (channelLinksFull(plugin->getSdk(), channelGid)) {
        logWarning(
            logPrefix + "the number of links on channel: " + channelGid +
            " is at or exceeds the max number of links for the channel, please update config gen "
            "scripts to not fulfill more than the maximum number of links supported.");
    }

    auto channelIter = linkProfiles.find(channelGid);
    if (channelIter == linkProfiles.end()) {
        logWarning(logPrefix + "no links found for channel " + channelGid);
        return true;  // not a failure
    }

    for (auto linkProfileIter = channelIter->second.begin();
         linkProfileIter != channelIter->second.end(); ++linkProfileIter) {
        const auto &linkProfile = *linkProfileIter;
        if (linkProfile.role == "creator") {
            logDebug(logPrefix + "creating link: " + linkProfile.description);
            SdkResponse response = createLinkFromAddress(channelGid, linkProfileIter);
            if (response.status != SDK_OK) {
                logError(logPrefix + "error creating link from address for link + " +
                         linkProfile.description + " for channel " + channelGid + " with address " +
                         linkProfile.address +
                         ", failed with sdk response status: " + std::to_string(response.status));
                return false;
            }
            staticLinkRequests.insert(response.handle);

        } else if (linkProfile.role == "loader") {
            logDebug(logPrefix + "loading link: " + linkProfile.description);
            SdkResponse response;
            if (not linkProfile.address_list.empty()) {
                response = loadLinkAddresses(channelGid, linkProfileIter);
                if (response.status != SDK_OK) {
                    logError(
                        logPrefix + "error loading link addresses for link + " +
                        linkProfile.description + " for channel " + channelGid +
                        " with addresses " + nlohmann::json(linkProfile.address_list).dump() +
                        ", failed with sdk response status: " + std::to_string(response.status));
                    return false;
                }
            } else {
                response = loadLinkAddress(channelGid, linkProfileIter);
                if (response.status != SDK_OK) {
                    logError(
                        logPrefix + "error loading link address for link + " +
                        linkProfile.description + " for channel " + channelGid + " with address " +
                        linkProfile.address +
                        ", failed with sdk response status: " + std::to_string(response.status));
                    return false;
                }
            }
            staticLinkRequests.insert(response.handle);

        } else {
            logError(logPrefix + "unrecognized role " + linkProfile.role + " for link " +
                     linkProfile.description + " for channel " + channelGid);
        }
    }

    return true;
}

void LinkManager::checkStaticLinksCreated() {
    // If this was the last link request and there are no more channels to wait for, notify the
    // plugin
    if (staticLinkRequests.size() == 0 and channelsAwaitingActivation.size() == 0) {
        plugin->onStaticLinksCreated();
    }
}

void LinkManager::writeLinkProfiles() {
    TRACE_METHOD();
    std::string configFilePath = "link-profiles.json";
    ConfigStaticLinks::writeLinkProfiles(*plugin->getSdk(), configFilePath, linkProfiles);
}