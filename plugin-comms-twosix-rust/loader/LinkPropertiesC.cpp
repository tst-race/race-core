
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

#include "LinkPropertiesC.h"

LinkPropertiesC create_link_properties() {
    LinkPropertiesC props;
    props.supported_hints = new std::vector<std::string>;
    props.channelGid = new std::string;
    props.linkAddress = new std::string;
    return props;
}

void destroy_link_properties(LinkPropertiesC *props) {
    auto supported_hints = static_cast<std::vector<std::string> *>(props->supported_hints);
    delete supported_hints;
    props->supported_hints = nullptr;

    auto channelGid = static_cast<std::string *>(props->channelGid);
    delete channelGid;
    props->channelGid = nullptr;

    auto linkAddress = static_cast<std::string *>(props->linkAddress);
    delete linkAddress;
    props->linkAddress = nullptr;
}

void add_supported_hint_to_link_properties(LinkPropertiesC *props, const char *hint) {
    if (props != nullptr && props->supported_hints != nullptr) {
        std::vector<std::string> *supportedHints =
            static_cast<std::vector<std::string> *>(props->supported_hints);
        supportedHints->push_back(std::string(hint));
    }
}

void set_channel_gid_for_link_properties(LinkPropertiesC *props, const char *channelGid) {
    if (props != nullptr && props->channelGid != nullptr) {
        std::string *channelGidInProps = static_cast<std::string *>(props->channelGid);
        channelGidInProps->assign(std::string(channelGid));
    }
}

void set_link_address_for_link_properties(LinkPropertiesC *props, const char *linkAddress) {
    if (props != nullptr && props->linkAddress != nullptr) {
        std::string *linkAddressInProps = static_cast<std::string *>(props->linkAddress);
        linkAddressInProps->assign(std::string(linkAddress));
    }
}
