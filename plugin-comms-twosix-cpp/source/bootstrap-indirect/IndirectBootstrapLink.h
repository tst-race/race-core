
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

#ifndef _INDIRECT_BOOTSTRAP_LINK_H_
#define _INDIRECT_BOOTSTRAP_LINK_H_

#include "../whiteboard/TwosixWhiteboardLink.h"

class IndirectBootstrapChannel;

class IndirectBootstrapLink : public TwosixWhiteboardLink {
    std::string bootstrapDir;

protected:
    std::string mPassphrase;

public:
    IndirectBootstrapLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin, Channel *channel,
                          const LinkID &linkId, const LinkProperties &linkProperties,
                          const TwosixWhiteboardLinkProfileParser &parser,
                          const std::string &passphrase = "");

    virtual ~IndirectBootstrapLink();

    virtual PluginResponse serveFiles(std::string path) override;

    bool uploadFile(std::string filename);
};

#endif
