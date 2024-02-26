
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

#ifndef __LINK_MANAGER_H__
#define __LINK_MANAGER_H__

#include <IRacePluginNM.h>
#include <IRaceSdkNM.h>

#include <list>
#include <unordered_map>
#include <unordered_set>

#include "LinkProfile.h"

class PluginNMTwoSix;

class LinkManager {
public:
    explicit LinkManager(PluginNMTwoSix *_plugin) : plugin(_plugin) {}
    void init(const std::unordered_map<std::string, std::string> &roles);
    virtual PluginResponse onChannelStatusChanged(RaceHandle handle, const std::string &channelGid,
                                                  ChannelStatus status);
    virtual PluginResponse onLinkStatusChanged(RaceHandle handle, const LinkID &linkId,
                                               LinkStatus status, const LinkProperties &properties);

    virtual SdkResponse createLink(const std::string &channelGid,
                                   const std::vector<std::string> &personas);
    virtual SdkResponse createLinkFromAddress(const std::string &channelGid,
                                              const std::string &linkAddress,
                                              const std::vector<std::string> &personas);
    virtual SdkResponse loadLinkAddress(const std::string &channelGid,
                                        const std::string &linkAddress,
                                        const std::vector<std::string> &personas);
    virtual SdkResponse loadLinkAddresses(const std::string &channelGid,
                                          const std::vector<std::string> &linkAddresses,
                                          const std::vector<std::string> &personas);

    virtual SdkResponse setPersonasForLink(const std::string &linkId,
                                           const std::vector<std::string> &personas);

    virtual bool hasLink(const std::vector<std::string> &personas, LinkType linkType,
                         const std::string &channelGid, LinkSide linkSide);

private:
    using LinkProfileList = std::list<LinkProfile>;
    using LinkProfileListIter = LinkProfileList::iterator;

private:
    /**
     * @brief Activate all channels in CHANNEL_ENABLED state.
     *
     * @param roles The role to activate for each channel
     * @return void
     */
    void activateChannels(const std::unordered_map<std::string, std::string> &roles);

    /**
     * @brief Load static links for this node from the link-profiles file
     *
     * @return void
     */
    void readLinkProfiles();

    /**
     * @brief Initialize (create or load) static links for the specified channel
     *
     * @param channelGid Channel GID for which to initialize static links
     * @return True if all static links were initialized
     */
    bool initStaticLinks(const std::string &channelGid);

    // Functions to invoke the SDK and add to pendingLinks
    SdkResponse createLinkFromAddress(const std::string &channelGid,
                                      const LinkProfileListIter &linkProfileIter);
    SdkResponse loadLinkAddress(const std::string &channelGid,
                                const LinkProfileListIter &linkProfileIter);
    SdkResponse loadLinkAddresses(const std::string &channelGid,
                                  const LinkProfileListIter &linkProfileIter);

    void addPendingCreateLink(RaceHandle handle, const std::string &role,
                              const std::vector<std::string> &personas);

    // Functions to add/remove from linkProfiles
    LinkProfileListIter addLinkProfile(const std::string &channelGid, const std::string &address,
                                       const std::string &role,
                                       const std::vector<std::string> &personas);
    LinkProfileListIter addLinkProfile(const std::string &channelGid,
                                       const std::vector<std::string> &addresses,
                                       const std::string &role,
                                       const std::vector<std::string> &personas);
    void removeLinkProfile(const std::string &channelGid, const std::string &linkId);

    void checkStaticLinksCreated();

    void writeLinkProfiles();

private:
    PluginNMTwoSix *plugin;

    std::unordered_map<std::string, std::string> channelRoles;

    std::unordered_set<std::string> channelsAwaitingActivation;
    std::unordered_set<RaceHandle> staticLinkRequests;

    // Links being created and have no address yet, so not yet inserted into linkProfiles
    std::unordered_map<RaceHandle, LinkProfile> pendingCreateLinks;
    // Links being created from address or loaded and have address(es),
    // so already inserted into linkProfiles
    std::unordered_map<RaceHandle, LinkProfileListIter> pendingLinks;

    // Links that have been fully created or loaded by the corresponding comms channel
    std::unordered_map<LinkID, LinkProfileListIter> linkIdToProfileIter;

    // Profile lists indexed by channel GID, this structure directly maps to the
    // link-profiles.json file. Channels in this map may not have been activated yet.
    // Profile lists may contain dynamically loaded links that have not yet been fully
    // loaded by the comms plugin, meaning that they will have a corresponding entry in
    // pendingLinks.
    std::unordered_map<std::string, LinkProfileList> linkProfiles;
};

#endif
