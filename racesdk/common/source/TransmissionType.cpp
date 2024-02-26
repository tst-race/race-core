
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

#include "TransmissionType.h"

#include <stdexcept>

std::string transmissionTypeToString(TransmissionType transmissionType) {
    switch (transmissionType) {
        case TT_UNDEF:
            return "TT_UNDEF";
        case TT_UNICAST:
            return "TT_UNICAST";
        case TT_MULTICAST:
            return "TT_MULTICAST";
        default:
            return "ERROR: INVALID TRANSMISSION TYPE: " + std::to_string(transmissionType);
    }
}

TransmissionType transmissionTypeFromString(const std::string &transmissionTypeString) {
    if (transmissionTypeString == "TT_UNDEF") {
        return TT_UNDEF;
    } else if (transmissionTypeString == "TT_UNICAST") {
        return TT_UNICAST;
    } else if (transmissionTypeString == "TT_MULTICAST") {
        return TT_MULTICAST;
    } else {
        throw std::invalid_argument("Invalid argument to transmissionTypeFromString: " +
                                    transmissionTypeString);
    }
}

std::ostream &operator<<(std::ostream &out, TransmissionType transmissionType) {
    return out << transmissionTypeToString(transmissionType);
}
