
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

#include "ConnectionStatus.h"

std::string connectionStatusToString(ConnectionStatus connectionStatus) {
    switch (connectionStatus) {
        case CONNECTION_INVALID:
            return "CONNECTION_INVALID";
        case CONNECTION_OPEN:
            return "CONNECTION_OPEN";
        case CONNECTION_CLOSED:
            return "CONNECTION_CLOSED";
        case CONNECTION_AWAITING_CONTACT:
            return "CONNECTION_AWAITING_CONTACT";
        case CONNECTION_INIT_FAILED:
            return "CONNECTION_INIT_FAILED";
        case CONNECTION_AVAILABLE:
            return "CONNECTION_AVAILABLE";
        case CONNECTION_UNAVAILABLE:
            return "CONNECTION_UNAVAILABLE";
        default:
            return "ERROR: INVALID CONNECTION STATUS: " + std::to_string(connectionStatus);
    }
}

std::ostream &operator<<(std::ostream &out, ConnectionStatus connectionStatus) {
    return out << connectionStatusToString(connectionStatus);
}
