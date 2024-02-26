
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

#ifndef __RACE_CHANNEL_ROLE_H_
#define __RACE_CHANNEL_ROLE_H_

#include <string>
#include <vector>

enum LinkSide {
    LS_UNDEF = 0,
    LS_CREATOR = 1,
    LS_LOADER = 2,
    LS_BOTH = 3,
};

struct ChannelRole {
    ChannelRole() : linkSide(LS_UNDEF) {}

    std::string roleName;
    std::vector<std::string> mechanicalTags;
    std::vector<std::string> behavioralTags;
    LinkSide linkSide;
};

inline bool operator==(const ChannelRole &a, const ChannelRole &b) {
    return a.roleName == b.roleName && a.mechanicalTags == b.mechanicalTags &&
           a.behavioralTags == b.behavioralTags && a.linkSide == b.linkSide;
}
inline bool operator!=(const ChannelRole &a, const ChannelRole &b) {
    return !(a == b);
}

/**
 * @brief Convert a LinkSide value to a human readable string.
 *
 * @param LinkSide The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string linkSideToString(LinkSide linkSide);
LinkSide linkSideFromString(const std::string &linkSideString);

std::ostream &operator<<(std::ostream &out, LinkSide linkSide);

/**
 * @brief Convert a ChannelRole value to a human readable string. This function is strictly for
 * logging and debugging. The output formatting may change without notice. Do NOT use this for any
 * logical comparisons, etc. The functionality of your plugin should in no way rely on the output of
 * this function.
 *
 * @param ChannelRole The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string channelRoleToString(const ChannelRole &channelRole);

#endif
