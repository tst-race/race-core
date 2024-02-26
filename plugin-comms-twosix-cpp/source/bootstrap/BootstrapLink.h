
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

#ifndef _BOOTSTRAP_LINK_H_
#define _BOOTSTRAP_LINK_H_

#include "../direct/DirectLink.h"

class BootstrapChannel;

class BootstrapLink : public DirectLink {
protected:
    std::string mPassphrase;

public:
    BootstrapLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin, Channel *channel,
                  const LinkID &linkId, const LinkProperties &linkProperties,
                  const DirectLinkProfileParser &parser, const std::string &passphrase = "");
    virtual ~BootstrapLink();

    virtual PluginResponse serveFiles(std::string path) override;

protected:
    BootstrapChannel *getChannel();
};

#endif
