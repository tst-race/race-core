
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

#ifndef __RACE_CONNECTION_TYPE_H_
#define __RACE_CONNECTION_TYPE_H_

#include <string>

enum ConnectionType {
    CT_UNDEF = 0,     // undefined
    CT_DIRECT = 1,    // direct connection
    CT_INDIRECT = 2,  // indirect connection
    CT_MIXED = 3,     // link both reveals its IP to other node and contacts a 3rd-party service
    CT_LOCAL = 4      // link is physically local (e.g. a WiFi hotspot)
};

/**
 * @brief Convert a ConnectionType value to a human readable string. This function is strictly for
 * logging and debugging. The output formatting may change without notice. Do NOT use this for any
 * logical comparisons, etc. The functionality of your plugin should in no way rely on the output of
 * this function.
 *
 * @param connectionType The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string connectionTypeToString(ConnectionType connectionType);

// TODO: documentation
ConnectionType connectionTypeFromString(const std::string &connectionTypeString);

std::ostream &operator<<(std::ostream &out, ConnectionType connectionType);

#endif
