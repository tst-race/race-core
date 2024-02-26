
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

#ifndef _CHANNEL_ROLE_C_H__
#define _CHANNEL_ROLE_C_H__

#include "ChannelRole.h"

extern "C" {

struct ChannelRoleC {
    char *roleName;
    char **mechanicalTags;
    size_t mechanicalTagsLen;
    char **behavioralTags;
    size_t behavioralTagsLen;
    LinkSide linkSide;
};

ChannelRoleC create_channel_role(char *roleName, char **mechanicalTags, size_t mechanicalTagsLen,
                                 char **behavioralTags, size_t behavioralTagsLen,
                                 LinkSide linkSide);

void destroy_channel_role(ChannelRoleC *props);
}

#endif
