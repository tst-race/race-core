
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

#include "ConnectionType.h"

#include <stdexcept>

std::string connectionTypeToString(ConnectionType connectionType) {
    switch (connectionType) {
        case CT_UNDEF:
            return "CT_UNDEF";
        case CT_DIRECT:
            return "CT_DIRECT";
        case CT_INDIRECT:
            return "CT_INDIRECT";
        case CT_MIXED:
            return "CT_MIXED";
        case CT_LOCAL:
            return "CT_LOCAL";
        default:
            return "ERROR: INVALID CONNECTION TYPE: " + std::to_string(connectionType);
    }
}

ConnectionType connectionTypeFromString(const std::string &connectionTypeString) {
    if (connectionTypeString == "CT_UNDEF") {
        return CT_UNDEF;
    } else if (connectionTypeString == "CT_DIRECT") {
        return CT_DIRECT;
    } else if (connectionTypeString == "CT_INDIRECT") {
        return CT_INDIRECT;
    } else if (connectionTypeString == "CT_MIXED") {
        return CT_MIXED;
    } else if (connectionTypeString == "CT_LOCAL") {
        return CT_LOCAL;
    } else {
        throw std::invalid_argument("Invalid argument to connectionTypeFromString: " +
                                    connectionTypeString);
    }
}

std::ostream &operator<<(std::ostream &out, ConnectionType connectionType) {
    return out << connectionTypeToString(connectionType);
}