
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

#include "MessageStatus.h"

std::string messageStatusToString(MessageStatus messageStatus) {
    switch (messageStatus) {
        case MS_UNDEF:
            return "MS_UNDEF";
        case MS_SENT:
            return "MS_SENT";
        case MS_FAILED:
            return "MS_FAILED";
        default:
            return "ERROR: INVALID MESSAGE STATUS: " + std::to_string(messageStatus);
    }
}

std::ostream &operator<<(std::ostream &out, MessageStatus messageStatus) {
    return out << messageStatusToString(messageStatus);
}
