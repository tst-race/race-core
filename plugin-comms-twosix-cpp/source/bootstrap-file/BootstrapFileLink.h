
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

#ifndef _BOOSTRAP_FILE_LINK_H_
#define _BOOSTRAP_FILE_LINK_H_

#include <atomic>  // std::atomic
#include <thread>  // std::thread

#include "../base/Link.h"
#include "BootstrapFileLinkProfileParser.h"

class BootstrapFileLink : public Link {
protected:
    const std::string directory;

    virtual bool sendPackageInternal(RaceHandle handle, const EncPkg &pkg) override;
    virtual bool sendBytes(const std::string &name, const std::vector<uint8_t> &pkg);

public:
    BootstrapFileLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin, Channel *channel,
                      const LinkID &linkId, const LinkProperties &linkProperties,
                      const BootstrapFileLinkProfileParser &parser = {});
    virtual ~BootstrapFileLink();

    virtual std::shared_ptr<Connection> openConnection(LinkType linkType,
                                                       const ConnectionID &connectionId,
                                                       const std::string &linkHints,
                                                       int timeout) override;
    virtual void closeConnection(const ConnectionID &connectionId) override;
    virtual void startConnection(Connection *connection) override;

    virtual void shutdownInternal() override;

    virtual PluginResponse serveFiles(std::string path) override;
    /**
     * @brief Get the link address.
     *
     * @return std::string The link address.
     */
    virtual std::string getLinkAddress() override;
};

#endif
