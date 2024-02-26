
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

#include "LinkType.h"

std::string linkTypeToString(LinkType linkType) {
    switch (linkType) {
        case LT_UNDEF:
            return "LT_UNDEF";
        case LT_SEND:
            return "LT_SEND";
        case LT_RECV:
            return "LT_RECV";
        case LT_BIDI:
            return "LT_BIDI";
        default:
            return "ERROR: INVALID LINK TYPE: " + std::to_string(linkType);
    }
}

std::ostream &operator<<(std::ostream &out, LinkType linkType) {
    return out << linkTypeToString(linkType);
}
