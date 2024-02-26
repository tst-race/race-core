
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

#ifndef __SOURCE_RACE_CHANNELS_H__
#define __SOURCE_RACE_CHANNELS_H__

#include <ChannelProperties.h>  // ChannelProperties
#include <ChannelStatus.h>      // ChannelStatus
#include <IRaceSdkCommon.h>

#include <atomic>         // std::atomic
#include <map>            // std::unordered_map
#include <memory>         // std::unique_ptr
#include <mutex>          // std::mutex, std::lock_guard
#include <set>            // std::set
#include <string>         // std::string
#include <unordered_map>  // std::unordered_map
#include <unordered_set>  // std::unordered_set
#include <vector>         // std::vector

#include "SdkResponse.h"

class RaceChannels {
public:
    class ChannelInfo {
    public:
        explicit ChannelInfo(const ChannelProperties &props) : properties(props) {}

        ChannelProperties properties;
        std::vector<std::string> plugins;
        std::string wrapperId;

        // how to set?
        std::vector<std::string> tags;
    };

    RaceChannels();
    RaceChannels(const std::vector<ChannelProperties> &channelProperties, IRaceSdkCommon *sdk);
    virtual ~RaceChannels() {}

    // TODO docs
    virtual std::map<std::string, ChannelProperties> getSupportedChannels();

    // TODO docs
    virtual ChannelProperties getChannelProperties(const std::string &channelGid);

    // TODO docs
    virtual std::vector<std::string> getPluginsForChannel(const std::string &channelGid);

    // TODO docs
    virtual std::string getWrapperIdForChannel(const std::string &channelGid);

    // TODO docs
    virtual bool isAvailable(const std::string &channelGid);

    // // TODO docs
    virtual bool update(const std::string &channelGid, ChannelStatus status,
                        const ChannelProperties &properties);
    virtual bool update(const ChannelProperties &properties);

    // // TODO docs
    // void add(const std::string &channelGid, const std::string &pluginId,
    //          const std::vector<std::string> &tags);

    // TODO docs
    void add(const ChannelProperties &properties);

    // TODO docs
    virtual std::vector<ChannelProperties> getChannels();

    // TODO docs
    virtual std::vector<std::string> getChannelIds();

    /**
     * @brief Returns a vector of the LinkIDs for a given channel.
     *
     * @param channelGid the name of the channel
     * @return the LinkIDs for the channel in a vector
     */
    virtual std::vector<LinkID> getLinksForChannel(const std::string &channelGid);

    // TODO docs
    virtual void setPluginsForChannel(const std::string &channelGid,
                                      const std::vector<std::string> &plugins);
    virtual void setWrapperIdForChannel(const std::string &channelGid,
                                        const std::string &wrapperId);

    /**
     * @brief Stores the LinkID and corresponding channelGid in a map for reference by
     * getLinksForChannel()
     *
     * @param channelGid the name of the channel of the link
     * @param linkId the ID of the link
     * @return N/A
     */
    virtual void setLinkId(const std::string &channelGid, const LinkID &linkId);

    /**
     * @brief Removes the LinkID from the channel in the map referenced by getLinksForChannel()
     *
     * @param channelGid the name of the channel of the link
     * @param linkId the ID of the link
     * @return N/A
     */
    virtual void removeLinkId(const std::string &channelGid, const LinkID &linkId);

    /**
     * @brief Gets the current status of the specified channel. If the channel does not exist,
     * an std::out_of_range exception is thrown.
     *
     * @param channelGid The name of the channel
     * @return Channel status
     */
    virtual ChannelStatus getStatus(const std::string &channelGid);

    /**
     * @brief Sets the current status of the specified channel. If the channel does not exist,
     * no action occurs.
     *
     * @param channelGid The name of the channel
     * @param status New channel status
     */
    virtual void setStatus(const std::string &channelGid, ChannelStatus status);

    /**
     * @brief return true if these tags conflict with an existing channel
     *
     * @param tags list of tags to check for conflicts
     * @return true if there is a conflict
     */
    virtual bool checkMechanicalTags(const std::vector<std::string> &tags);

    /**
     * @brief return true if these tags conflict with the environment
     *
     * @param tags list of tags to check for conflicts
     * @return true if there is a conflict
     */
    virtual bool checkBehavioralTags(const std::vector<std::string> &tags);

    // TODO docs
    virtual bool activate(const std::string &channelGid, const std::string &roleName);

    virtual void channelFailed(const std::string &channelGid);

    virtual void setAllowedTags(const std::vector<std::string> &tags);

    virtual std::vector<std::string> getPluginChannelIds(const std::string &pluginId);

    /**
     * @brief Set the set of user-enabled channels. The set of enabled channels will be
     * written to disk. This should only be invoked during initialization, if at all.
     *
     * @param channelGids The names of channels to be enabled
     */
    virtual void setUserEnabledChannels(const std::vector<std::string> &channelGids);

    /**
     * @brief Set the specified channel as user-enabled. The updated set of enabled channels
     * will be re-written to disk.
     *
     * @param channelGid The name of the channel to be enabled
     */
    virtual void setUserEnabled(const std::string &channelGid);

    /**
     * @brief Set the specified channel as user-disabled. The updated set of enabled channels
     * will be re-written to disk.
     *
     * @param channelGid The name of the channel to be disabled
     */
    virtual void setUserDisabled(const std::string &channelGid);

    /**
     * @brief Check if a channel has been enabled by the user.
     *
     * @param channelGid The name of the channel
     * @return True if channel is enabled by the user, else false
     */
    virtual bool isUserEnabled(const std::string &channelGid);

private:
    std::mutex channelsLock;

    IRaceSdkCommon *sdk;

    // unique ptr because vector may move objects in memory when resizing
    std::vector<std::unique_ptr<ChannelInfo>> channels;
    std::unordered_map<std::string, ChannelInfo *> channelIdToInfo;
    std::unordered_map<std::string, std::unordered_set<LinkID>> channelIdToLinkIds;

    std::set<std::string> userEnabledChannels;

    // allowed behavior tags
    std::vector<std::string> allowedTags;

protected:
    virtual void readUserEnabledChannels();
    virtual void writeUserEnabledChannels();
};

#endif
