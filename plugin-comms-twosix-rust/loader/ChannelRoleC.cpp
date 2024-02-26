
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

#include "ChannelRoleC.h"

#include <cstring>

ChannelRoleC create_channel_role(char *roleName, char **mechanicalTags, size_t mechanicalTagsLen,
                                 char **behavioralTags, size_t behavioralTagsLen,
                                 LinkSide linkSide) {
    ChannelRoleC props;
    props.roleName = roleName ? strdup(roleName) : nullptr;
    props.mechanicalTagsLen = mechanicalTagsLen;
    props.mechanicalTags = new char *[mechanicalTagsLen];
    for (size_t i = 0; i < mechanicalTagsLen; i++) {
        props.mechanicalTags[i] = strdup(mechanicalTags[i]);
    }
    props.behavioralTagsLen = behavioralTagsLen;
    props.behavioralTags = new char *[behavioralTagsLen];
    for (size_t i = 0; i < behavioralTagsLen; i++) {
        props.behavioralTags[i] = strdup(behavioralTags[i]);
    }
    props.linkSide = linkSide;
    return props;
}

void destroy_channel_role(ChannelRoleC *props) {
    free(props->roleName);
    props->roleName = nullptr;

    for (size_t i = 0; i < props->mechanicalTagsLen; i++) {
        free(props->mechanicalTags[i]);
        props->mechanicalTags[i] = nullptr;
    }
    delete[] props->mechanicalTags;
    props->mechanicalTags = nullptr;
    props->mechanicalTagsLen = 0;

    for (size_t i = 0; i < props->behavioralTagsLen; i++) {
        free(props->behavioralTags[i]);
        props->behavioralTags[i] = nullptr;
    }
    delete[] props->behavioralTags;
    props->behavioralTags = nullptr;
    props->behavioralTagsLen = 0;
}