
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

#include "ChannelPropertiesC.h"

ChannelPropertiesC create_channel_properties() {
    ChannelPropertiesC props;
    props.supported_hints = new std::vector<std::string>;
    props.channelGid = new std::string;
    props.roles = nullptr;
    props.rolesLen = 0;
    props.currentRole = create_channel_role(nullptr, nullptr, 0, nullptr, 0, LS_UNDEF);
    props.maxSendsPerInterval = -1;
    props.secondsPerInterval = -1;
    props.intervalEndTime = 0;
    props.sendsRemainingInInterval = -1;
    return props;
}

void destroy_channel_properties(ChannelPropertiesC *props) {
    auto supported_hints = static_cast<std::vector<std::string> *>(props->supported_hints);
    delete supported_hints;
    props->supported_hints = nullptr;
    auto channelGid = static_cast<std::string *>(props->channelGid);
    delete channelGid;
    props->channelGid = nullptr;

    for (size_t i = 0; i < props->rolesLen; i++) {
        destroy_channel_role(&props->roles[i]);
    }
    delete[] props->roles;
    props->roles = nullptr;

    destroy_channel_role(&props->currentRole);
}

void add_supported_hint_to_channel_properties(ChannelPropertiesC *props, const char *hint) {
    if (props != nullptr && props->supported_hints != nullptr) {
        std::vector<std::string> *supportedHints =
            static_cast<std::vector<std::string> *>(props->supported_hints);
        supportedHints->push_back(std::string(hint));
    }
}

void set_channel_gid_for_channel_properties(ChannelPropertiesC *props, const char *channelGid) {
    if (props != nullptr && props->channelGid != nullptr) {
        std::string *channelGidInProps = static_cast<std::string *>(props->channelGid);
        channelGidInProps->assign(std::string(channelGid));
    }
}

const char **get_supported_hints_for_channel_properties(ChannelPropertiesC *props,
                                                        size_t *vector_length) {
    std::vector<std::string> &hints =
        *static_cast<std::vector<std::string> *>(props->supported_hints);
    *vector_length = hints.size();

    const char **hintsC = new const char *[*vector_length];
    for (size_t i = 0; i < *vector_length; ++i) {
        hintsC[i] = hints[i].c_str();
    }

    return hintsC;
}

const char *get_channel_gid_for_channel_properties(ChannelPropertiesC *props) {
    return static_cast<std::string *>(props->channelGid)->c_str();
}

void resize_roles_for_channel_properties(ChannelPropertiesC *props, size_t size) {
    for (size_t i = 0; i < props->rolesLen; i++) {
        destroy_channel_role(&props->roles[i]);
    }
    delete[] props->roles;
    props->roles = new ChannelRoleC[size];
    props->rolesLen = size;
    for (size_t i = 0; i < props->rolesLen; i++) {
        props->roles[i] = create_channel_role(nullptr, nullptr, 0, nullptr, 0, LS_UNDEF);
    }
}
