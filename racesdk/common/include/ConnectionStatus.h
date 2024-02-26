
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

#ifndef __RACE_CONNECTION_STATUS_H_
#define __RACE_CONNECTION_STATUS_H_

#include <iostream>
#include <string>

enum ConnectionStatus {
    CONNECTION_INVALID = 0,           // default / undefined status
    CONNECTION_OPEN = 1,              // opened
    CONNECTION_CLOSED = 2,            // closed
    CONNECTION_AWAITING_CONTACT = 3,  // awaiting contact
    CONNECTION_INIT_FAILED = 4,       // init failed
    CONNECTION_AVAILABLE = 5,         // available
    CONNECTION_UNAVAILABLE = 6        // unavailable
};

/**
 * @brief Convert a ConnectionStatus value to a human readable string. This function is strictly for
 * logging and debugging. The output formatting may change without notice. Do NOT use this for any
 * logical comparisons, etc. The functionality of your plugin should in no way rely on the output of
 * this function.
 *
 * @param connectionStatus The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string connectionStatusToString(ConnectionStatus connectionStatus);

std::ostream &operator<<(std::ostream &out, ConnectionStatus connectionStatus);

#endif
