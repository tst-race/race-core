
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

#include "LinkStatus.h"

std::string linkStatusToString(LinkStatus linkStatus) {
    switch (linkStatus) {
        case LINK_UNDEF:
            return "LINK_UNDEF";
        case LINK_CREATED:
            return "LINK_CREATED";
        case LINK_LOADED:
            return "LINK_LOADED";
        case LINK_DESTROYED:
            return "LINK_DESTROYED";
        default:
            return "ERROR: INVALID LINK STATUS: " + std::to_string(linkStatus);
    }
}

std::ostream &operator<<(std::ostream &out, LinkStatus linkStatus) {
    return out << linkStatusToString(linkStatus);
}
