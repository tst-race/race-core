
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

#ifndef __COMMON_INCLUDE_MESSAGESTATUS_H__
#define __COMMON_INCLUDE_MESSAGESTATUS_H__

#include <string>

enum MessageStatus {
    MS_UNDEF = 0,  // Undefined, default value.
    MS_SENT = 1,   // The message has been sent from the node, i.e. all the packages associated with
                   // the message have been marked with the status PACKAGE_SENT
    MS_FAILED = 2  // Sending the message failed.
};

/**
 * @brief Convert a MessageStatus value to a human readable string. This function is strictly for
 * logging and debugging. The output formatting may change without notice. Do NOT use this for any
 * logical comparisons, etc. The functionality of your plugin should in no way rely on the output of
 * this function.
 *
 * @param messageStatus The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string messageStatusToString(MessageStatus messageStatus);

std::ostream &operator<<(std::ostream &out, MessageStatus messageStatus);

#endif
