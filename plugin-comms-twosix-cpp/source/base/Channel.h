
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

#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <memory>  // std::shared_ptr
#include <mutex>   // std::mutex, std::lock_guard
#include <sstream>
#include <thread>
#include <unordered_map>

#include "../utils/PortAllocator.h"
#include "../utils/log.h"
#include "Connection.h"
#include "Link.h"

class PluginCommsTwoSixCpp;

class Channel {
public:
    std::string channelGid;
    PluginCommsTwoSixCpp &plugin;
    ChannelStatus status;
    ChannelProperties properties;
    LinkProperties linkProperties;
    int numLinks;

public:
    explicit Channel(PluginCommsTwoSixCpp &plugin, const std::string &channelGid);
    virtual ~Channel();

    virtual PluginResponse createLink(RaceHandle handle);
    virtual PluginResponse createLinkFromAddress(RaceHandle handle, const std::string &linkAddress);
    virtual PluginResponse loadLinkAddress(RaceHandle handle, const std::string &linkAddress);
    virtual PluginResponse loadLinkAddresses(RaceHandle handle,
                                             const std::vector<std::string> &linkAddresses);
    virtual PluginResponse activateChannel(RaceHandle handle);
    virtual PluginResponse deactivateChannel(RaceHandle handle);
    virtual PluginResponse createBootstrapLink(RaceHandle handle, const std::string &passphrase);

    virtual void onLinkDestroyed(Link *link);
    virtual bool onUserInputReceived(RaceHandle handle, bool answered, const std::string &response);

public:
    static std::unordered_map<std::string, std::shared_ptr<Channel>> createChannels(
        PluginCommsTwoSixCpp &plugin);

protected:
    virtual std::shared_ptr<Link> createLink(const LinkID &linkId) = 0;
    virtual std::shared_ptr<Link> createLinkFromAddress(const LinkID &linkId,
                                                        const std::string &linkAddress) = 0;
    virtual std::shared_ptr<Link> createBootstrapLink(const LinkID &linkId,
                                                      const std::string &passphrase);
    virtual std::shared_ptr<Link> loadLink(const LinkID &linkId,
                                           const std::string &linkAddress) = 0;
    virtual void onLinkDestroyedInternal(Link *link);

    virtual PluginResponse activateChannelInternal(RaceHandle handle) = 0;

    virtual LinkProperties getDefaultLinkProperties() = 0;

    virtual void onGenesisLinkCreated(Link *link);

private:
    /**
     * @brief validate that the channel can create a link for the for the load/create link methods.
     * Checks channel status, max links, and role. This function also generates and returns the link
     * id.
     *
     * @param logLabel a prefix to prepend to log messages to indicate the parent function / channel
     * id
     * @param handle The handle to use in sdk calls if the checks fail
     * @param invalidRole The role that is not allowed, or LS_UNDEF if any role is valid
     * @return std::string The id for the created link
     */
    std::string preLinkCreate(const std::string &logLabel, RaceHandle handle, LinkSide invalidRole);

    /**
     * @brief Handle common logic for the load/create link methods after the link has been created.
     * This method calls the sdk callback handles adding the link internally.
     *
     * @param logLabel a prefix to prepend to log messages to indicate the parent function / channel
     * id
     * @param handle The handle to use in sdk calls
     * @param linkId The id of the newly created link
     * @param link The newly created link
     * @param linkStatus Created or loaded status for the newly created link
     * @return PluginResponse Whether adding the link succeeded or not
     */
    PluginResponse postLinkCreate(const std::string &logLabel, RaceHandle handle,
                                  const std::string &linkId, const std::shared_ptr<Link> &link,
                                  LinkStatus linkStatus);
};

#endif