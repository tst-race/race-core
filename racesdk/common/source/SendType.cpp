
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

#include "SendType.h"

#include <stdexcept>

std::string sendTypeToString(SendType sendType) {
    switch (sendType) {
        case ST_UNDEF:
            return "ST_UNDEF";
        case ST_STORED_ASYNC:
            return "ST_STORED_ASYNC";
        case ST_EPHEM_SYNC:
            return "ST_EPHEM_SYNC";
        default:
            return "ERROR: INVALID SEND TYPE: " + std::to_string(sendType);
    }
}

SendType sendTypeFromString(const std::string &sendTypeString) {
    if (sendTypeString == "ST_UNDEF") {
        return ST_UNDEF;
    } else if (sendTypeString == "ST_STORED_ASYNC") {
        return ST_STORED_ASYNC;
    } else if (sendTypeString == "ST_EPHEM_SYNC") {
        return ST_EPHEM_SYNC;
    } else {
        throw std::invalid_argument("Invalid argument to sendTypeFromString: " + sendTypeString);
    }
}

std::ostream &operator<<(std::ostream &out, SendType sendType) {
    return out << sendTypeToString(sendType);
}
