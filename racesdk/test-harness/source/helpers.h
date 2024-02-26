
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

#ifndef __SOURCE_HELPERS_H__
#define __SOURCE_HELPERS_H__

#include <LinkProperties.h>

#include <string>
#include <vector>

namespace helpers {

/**
 * @brief Token a message into a vector of strings using a given delimiter.
 *
 * @param message The message to tokenize.
 * @param delimiter The delimiter to split the string on.
 * @return std::vector<std::string> A vector of tokens from the message.
 */
std::vector<std::string> tokenizeMessage(const std::string &message,
                                         const std::string &delimiter = " ");

/**
 * @brief Get the first LinkID in the vector of LinkIDs.
 * WARNING: this function makes some assumptions about how the SDK is generating LinkIDs. Currently,
 * link IDs are simply an auto-incrementing counter starting at zero with the prefix "LinkID_". If
 * this implementation changes (e.g. to randomly generate )
 *
 * @param linkIds A vector of LinkID.
 * @return LinkID The first LinkID.
 */
LinkID getFirstLink(const std::vector<LinkID> &linkIds);

/**
 * @brief Split the given string value using the specified delimiter
 *
 * @param value String value to be split
 * @param delimiter Delimiter to use for splitting
 * @return Vector of string fragments between delimiters
 */
std::vector<std::string> split(const std::string &value, const std::string &delimiter);

}  // namespace helpers

#endif
