
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

#ifndef __RACE_LINK_STATUS_H_
#define __RACE_LINK_STATUS_H_

#include <iostream>
#include <string>

enum LinkStatus {
    LINK_UNDEF = 0,     // default / undefined status
    LINK_CREATED = 1,   // opened
    LINK_LOADED = 2,    // opened
    LINK_DESTROYED = 3  // closed
};

/**
 * @brief Convert a LinkStatus value to a human readable string. This function is strictly for
 * logging and debugging. The output formatting may change without notice. Do NOT use this for any
 * logical comparisons, etc. The functionality of your plugin should in no way rely on the output of
 * this function.
 *
 * @param linkStatus The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string linkStatusToString(LinkStatus linkStatus);

std::ostream &operator<<(std::ostream &out, LinkStatus linkStatus);

#endif
