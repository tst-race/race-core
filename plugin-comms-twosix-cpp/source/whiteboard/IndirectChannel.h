
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

#ifndef _INDIRECT_CHANNEL_H_
#define _INDIRECT_CHANNEL_H_

#include <atomic>

#include "../base/Channel.h"

class IndirectChannel : public Channel {
public:
    static const std::string indirectChannelGid;

public:
    explicit IndirectChannel(PluginCommsTwoSixCpp &plugin);
    explicit IndirectChannel(PluginCommsTwoSixCpp &_plugin, const std::string &_channelGid);

protected:
    virtual std::shared_ptr<Link> createLink(const LinkID &linkId) override;
    virtual std::shared_ptr<Link> createLinkFromAddress(const LinkID &linkId,
                                                        const std::string &linkAddress) override;
    virtual std::shared_ptr<Link> loadLink(const LinkID &linkId,
                                           const std::string &linkAddress) override;
    virtual PluginResponse activateChannelInternal(RaceHandle handle) override;

    virtual LinkProperties getDefaultLinkProperties() override;

protected:
    // Next available hashtag suffix.
    // TODO: should probably pull from a pool of tags (randomly generated?) instead so we can reuse
    // old tags. Although I'm guessing with a 64 bit int it's unlikely this will ever rollover
    // (famous last words?) - GP
    std::atomic<int64_t> nextAvailableHashTag;

    std::string whiteboardHostname;
    uint16_t whiteboardPort;
};

#endif
