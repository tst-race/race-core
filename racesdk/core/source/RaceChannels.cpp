
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

#include "RaceChannels.h"

#include <atomic>  // std::atomic
#include <nlohmann/json.hpp>
#include <stdexcept>      // std::invalid_argument
#include <unordered_set>  // std::unordered_set

#include "helper.h"

static const std::string USER_ENABLED_CHANNELS_FILE = "userEnabledChannels";
static const int jsonIndentLevel = 4;

RaceChannels::RaceChannels() : RaceChannels(std::vector<ChannelProperties>{}, nullptr) {}

RaceChannels::RaceChannels(const std::vector<ChannelProperties> &channelProperties,
                           IRaceSdkCommon *sdk) :
    sdk(sdk) {
    for (auto &channel : channelProperties) {
        add(channel);
    }
}

std::map<std::string, ChannelProperties> RaceChannels::getSupportedChannels() {
    std::lock_guard<std::mutex> lock{channelsLock};
    std::map<std::string, ChannelProperties> available;
    for (auto &channel : channels) {
        if (channel->properties.channelStatus == CHANNEL_AVAILABLE) {
            available.insert({channel->properties.channelGid, channel->properties});
        }
    }

    return available;
}

ChannelProperties RaceChannels::getChannelProperties(const std::string &channelGid) {
    std::lock_guard<std::mutex> lock{channelsLock};
    return channelIdToInfo.at(channelGid)->properties;
}

std::vector<std::string> RaceChannels::getPluginsForChannel(const std::string &channelGid) {
    std::lock_guard<std::mutex> lock{channelsLock};
    std::vector<std::string> plugins = channelIdToInfo.at(channelGid)->plugins;
    if (plugins.empty()) {
        throw std::out_of_range("Channel has no plugin ids associated with it");
    }
    return plugins;
}

std::string RaceChannels::getWrapperIdForChannel(const std::string &channelGid) {
    std::lock_guard<std::mutex> lock{channelsLock};
    std::string wrapperId = channelIdToInfo.at(channelGid)->wrapperId;
    if (wrapperId.empty()) {
        throw std::out_of_range("Channel has no wrapper id associated with it");
    }
    return wrapperId;
}

bool RaceChannels::isAvailable(const std::string &channelGid) {
    std::lock_guard<std::mutex> lock{channelsLock};
    auto it = channelIdToInfo.find(channelGid);
    if (it != channelIdToInfo.end()) {
        return it->second->properties.channelStatus == CHANNEL_AVAILABLE;
    }

    return false;
}

bool RaceChannels::update(const ChannelProperties &properties) {
    return update(properties.channelGid, properties.channelStatus, properties);
}

bool RaceChannels::update(const std::string &channelGid, ChannelStatus status,
                          const ChannelProperties &properties) {
    std::lock_guard<std::mutex> lock{channelsLock};
    auto it = channelIdToInfo.find(channelGid);
    if (it != channelIdToInfo.end()) {
        it->second->properties = properties;
        it->second->properties.channelStatus = status;

        if (status == CHANNEL_ENABLED) {
            // if the channel becomes enabled, then the role should be empty to prevent conflicts
            it->second->properties.currentRole = ChannelRole();
        }
    }
    return true;
}

void RaceChannels::add(const ChannelProperties &properties) {
    std::lock_guard<std::mutex> lock{channelsLock};
    if (channelIdToInfo.count(properties.channelGid) > 0) {
        throw std::invalid_argument("Already contain a channel with channelGid: " +
                                    properties.channelGid);
    }

    channels.emplace_back(std::make_unique<ChannelInfo>(properties));
    channelIdToInfo[properties.channelGid] = channels.back().get();
}

std::vector<ChannelProperties> RaceChannels::getChannels() {
    std::lock_guard<std::mutex> lock{channelsLock};
    std::vector<ChannelProperties> out;
    out.reserve(channels.size());
    for (auto &channel : channels) {
        out.push_back(channel->properties);
    }
    return out;
}

std::vector<std::string> RaceChannels::getChannelIds() {
    std::lock_guard<std::mutex> lock{channelsLock};
    std::vector<std::string> out;
    out.reserve(channels.size());
    for (auto &channel : channels) {
        out.push_back(channel->properties.channelGid);
    }
    return out;
}

std::vector<LinkID> RaceChannels::getLinksForChannel(const std::string &channelGid) {
    std::lock_guard<std::mutex> lock{channelsLock};
    try {
        std::unordered_set<LinkID> setLinks = channelIdToLinkIds.at(channelGid);
        return std::vector<LinkID>(setLinks.begin(), setLinks.end());
    } catch (const std::out_of_range &) {
        helper::logDebug("getLinksForChannel: unable to find links for channel: " + channelGid);
        return {};
    }
}

void RaceChannels::setWrapperIdForChannel(const std::string &channelGid,
                                          const std::string &wrapperId) {
    std::lock_guard<std::mutex> lock{channelsLock};
    auto it = channelIdToInfo.find(channelGid);
    if (it != channelIdToInfo.end()) {
        it->second->wrapperId = wrapperId;
    }
}

void RaceChannels::setPluginsForChannel(const std::string &channelGid,
                                        const std::vector<std::string> &plugins) {
    std::lock_guard<std::mutex> lock{channelsLock};
    auto it = channelIdToInfo.find(channelGid);
    if (it != channelIdToInfo.end()) {
        it->second->plugins = plugins;
    }
}

void RaceChannels::setLinkId(const std::string &channelGid, const LinkID &linkId) {
    std::lock_guard<std::mutex> lock{channelsLock};
    channelIdToLinkIds[channelGid].insert(linkId);
}

void RaceChannels::removeLinkId(const std::string &channelGid, const LinkID &linkId) {
    std::lock_guard<std::mutex> lock{channelsLock};
    channelIdToLinkIds[channelGid].erase(linkId);
}

ChannelStatus RaceChannels::getStatus(const std::string &channelGid) {
    std::lock_guard<std::mutex> lock{channelsLock};
    return channelIdToInfo.at(channelGid)->properties.channelStatus;
}

void RaceChannels::setStatus(const std::string &channelGid, ChannelStatus status) {
    std::lock_guard<std::mutex> lock{channelsLock};
    auto it = channelIdToInfo.find(channelGid);
    if (it != channelIdToInfo.end()) {
        it->second->properties.channelStatus = status;
    }
}

/**
 * @brief return true if these tags conflict with an existing channel
 *
 * @param tags list of tags to check for conflicts
 * @return true if there is a conflict
 */
bool RaceChannels::checkMechanicalTags(const std::vector<std::string> &tags) {
    // Mechanical tags conflict if there is a channel already using that tag
    // e.g. SRI pixelfed and STR pixelfed would both have the 'pixelfed' tag and thus conflict.
    // Other channels should only have a (non-empty) current role if they are active
    for (auto &channel : channels) {
        for (auto &existingTag : channel->properties.currentRole.mechanicalTags) {
            for (auto &newTag : tags) {
                if (newTag == existingTag) {
                    helper::logError(newTag + " mechanical tag conflicts with channel " +
                                     channel->properties.channelGid);
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * @brief return true if these tags conflict with the environment
 *
 * @param tags list of tags to check for conflicts
 * @return true if there is a conflict
 */
bool RaceChannels::checkBehavioralTags(const std::vector<std::string> &tags) {
    // Behavioral tags conflict if they are not in the list of allowed tags, and the allowed list is
    // not empty. The list being empty is used as a way to signal that any tags are allowed.
    // e.g. A Minecraft server has the 'server' tag which is not allowed on phones.
    bool conflict = false;
    for (auto &newTag : tags) {
        bool allowed = allowedTags.empty();
        for (auto &allowedTag : allowedTags) {
            if (newTag == allowedTag) {
                allowed = true;
                break;
            }
        }

        if (!allowed) {
            helper::logError(newTag + " is not allowed in this environment");
        }

        conflict |= !allowed;
    }

    return conflict;
}

bool RaceChannels::activate(const std::string &channelGid, const std::string &roleName) {
    std::lock_guard<std::mutex> lock{channelsLock};
    auto it = channelIdToInfo.find(channelGid);
    if (it != channelIdToInfo.end()) {
        if (it->second->properties.channelStatus == CHANNEL_ENABLED) {
            // TODO check for conflicting channels here
            for (ChannelRole &role : it->second->properties.roles) {
                if (roleName == role.roleName) {
                    if (checkMechanicalTags(role.mechanicalTags)) {
                        helper::logError("Channel conflicts with an already active channel");
                        return false;
                    } else if (checkBehavioralTags(role.behavioralTags)) {
                        helper::logError("Channel is not allowed in this environment");
                        return false;
                    }
                    it->second->properties.currentRole = role;
                }
            }

            if (it->second->properties.currentRole.linkSide == LS_UNDEF) {
                helper::logError("Got invalid role when activating channel '" + channelGid + "'");
                return false;
            }

            it->second->properties.channelStatus = CHANNEL_STARTING;
            return true;
        } else {
            helper::logError("Channel " + channelGid + " not in ENABLED state");
        }
    } else {
        helper::logError("Channel " + channelGid + " not found");
    }
    return false;
}

void RaceChannels::channelFailed(const std::string &channelGid) {
    std::lock_guard<std::mutex> lock{channelsLock};
    auto it = channelIdToInfo.find(channelGid);
    if (it != channelIdToInfo.end()) {
        if (it->second->properties.channelStatus == CHANNEL_STARTING) {
            it->second->properties.channelStatus = CHANNEL_FAILED;
        }
    }
}

void RaceChannels::setAllowedTags(const std::vector<std::string> &tags) {
    allowedTags = tags;
}

std::vector<std::string> RaceChannels::getPluginChannelIds(const std::string &pluginId) {
    std::lock_guard<std::mutex> lock{channelsLock};
    std::vector<std::string> channelIds;

    for (auto channelInfoIt = channels.begin(); channelInfoIt != channels.end(); ++channelInfoIt) {
        std::unique_ptr<ChannelInfo> &channelInfo = *channelInfoIt;
        for (std::string &pId : channelInfo->plugins) {
            if (pId == pluginId) {
                channelIds.push_back(channelInfo->properties.channelGid);
            }
        }
    }
    return channelIds;
}

void RaceChannels::setUserEnabledChannels(const std::vector<std::string> &channelGids) {
    std::lock_guard<std::mutex> lock{channelsLock};
    userEnabledChannels = {channelGids.begin(), channelGids.end()};
    writeUserEnabledChannels();
}

void RaceChannels::setUserEnabled(const std::string &channelGid) {
    std::lock_guard<std::mutex> lock{channelsLock};

    auto [iter, inserted] = userEnabledChannels.insert(channelGid);
    if (inserted) {
        writeUserEnabledChannels();
    }
}

void RaceChannels::setUserDisabled(const std::string &channelGid) {
    std::lock_guard<std::mutex> lock{channelsLock};

    if (userEnabledChannels.erase(channelGid) == 1) {
        writeUserEnabledChannels();
    }
}

bool RaceChannels::isUserEnabled(const std::string &channelGid) {
    std::lock_guard<std::mutex> lock{channelsLock};
    if (userEnabledChannels.empty()) {
        readUserEnabledChannels();
    }

    auto iter = userEnabledChannels.find(channelGid);
    return iter != userEnabledChannels.end();
}

void RaceChannels::readUserEnabledChannels() {
    // This shouldn't actually happen, but unit tests might not pass in an SDK
    if (sdk == nullptr) {
        helper::logWarning("No SDK, unable to read user enabled channels");
        return;
    }

    auto bytes = sdk->readFile(USER_ENABLED_CHANNELS_FILE);
    if (not bytes.empty()) {
        userEnabledChannels =
            nlohmann::json::parse(bytes.begin(), bytes.end()).get<std::set<std::string>>();
    } else {
        helper::logWarning(
            "No data read from user enabled channels file, no channels will be enabled");
    }
}

void RaceChannels::writeUserEnabledChannels() {
    // This shouldn't actually happen, but unit tests might not pass in an SDK
    if (sdk == nullptr) {
        helper::logWarning("No SDK, unable to write user enabled channels");
        return;
    }

    std::string str = nlohmann::json(userEnabledChannels).dump(jsonIndentLevel);
    SdkResponse response = sdk->writeFile(USER_ENABLED_CHANNELS_FILE, {str.begin(), str.end()});
    if (response.status != SDK_OK) {
        helper::logError("Failed to write json to " + USER_ENABLED_CHANNELS_FILE);
    }
}
