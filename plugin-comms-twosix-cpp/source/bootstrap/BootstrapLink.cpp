
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

#include "BootstrapLink.h"

#include "BootstrapChannel.h"

BootstrapLink::BootstrapLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin, Channel *channel,
                             const LinkID &linkId, const LinkProperties &linkProperties,
                             const DirectLinkProfileParser &parser, const std::string &passphrase) :
    DirectLink(sdk, plugin, channel, linkId, linkProperties, parser), mPassphrase(passphrase) {}

BootstrapLink::~BootstrapLink() {
    getChannel()->server.stopServing(mPassphrase);
}

PluginResponse BootstrapLink::serveFiles(std::string path) {
    if (!mPassphrase.empty()) {
        getChannel()->server.serveFiles(mPassphrase, path);
        std::string downloadUrl =
            "http://" + getChannel()->plugin.racePersona + ":2626/" + mPassphrase;
        getChannel()->plugin.raceSdk->displayBootstrapInfoToUser(downloadUrl, RaceEnums::UD_QR_CODE,
                                                                 RaceEnums::BS_DOWNLOAD_BUNDLE);
    }

    return PLUGIN_OK;
}

BootstrapChannel *BootstrapLink::getChannel() {
    return dynamic_cast<BootstrapChannel *>(mChannel);
}
