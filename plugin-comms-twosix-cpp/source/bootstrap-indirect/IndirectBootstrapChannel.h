
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

#ifndef _INDIRECT_BOOTSTRAP_CHANNEL_H_
#define _INDIRECT_BOOTSTRAP_CHANNEL_H_

#include "../whiteboard/IndirectChannel.h"

class IndirectBootstrapChannel : public IndirectChannel {
public:
    static const std::string indirectBootstrapChannelGid;

public:
    explicit IndirectBootstrapChannel(PluginCommsTwoSixCpp &plugin);

protected:
    virtual std::shared_ptr<Link> createLink(const LinkID &linkId) override;
    virtual std::shared_ptr<Link> createLinkFromAddress(const LinkID &linkId,
                                                        const std::string &linkAddress) override;
    virtual std::shared_ptr<Link> createBootstrapLink(const LinkID &linkId,
                                                      const std::string &passphrase) override;
    virtual std::shared_ptr<Link> loadLink(const LinkID &linkId,
                                           const std::string &linkAddress) override;
};

#endif
