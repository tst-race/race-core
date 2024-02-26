
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

#ifndef __SOURCE_MESSAGE_SERIALIZER_HELPERS_H__
#define __SOURCE_MESSAGE_SERIALIZER_HELPERS_H__

#include <ClrMsg.h>

#include <cstdint>
#include <string>
#include <vector>

namespace msh {

std::string convertToHexString(size_t input, size_t paddedLength = 0);

/**
 * @brief Convert a hexadecimal string into an integer value stored in size_t. If the hexadeciaml
 * string is negative then zero will be returned.
 * @warning If an error occurs the function will return zero. This is valid for the current use
 * case, but may cause issues if used elswehere.
 *
 * @param hexString A hexadeciaml string.
 * @return size_t The value of the hexademcial string, or zero on error.
 */
size_t convertFromHexString(const std::string &hexString);

/**
 * @brief Append data to a serialized message.First, it appends a fixed length header containing the
 * length of the data in hexadecimal. Then, it appends the data.
 *
 * @param seriliazedMessage The serialized message to append to.
 * @param data The data to appened to the serialized message.
 * @param headerSize The fixed length size of the header.
 */
void appendDataToSerializedMessage(std::string &seriliazedMessage, const std::string &data,
                                   const size_t headerSize);

/**
 * @brief Convert the data in a ClrMsg object into a vector of strings. Data is stored in the order:
 *      1. message
 *      2. from
 *      3. to
 *      4. time
 *      5. nonce
 * Note that this function is the inverse of convertVectorToClrMsg. The following is and must remain
 * true:
 *      convertClrMsgToVector(convertVectorToClrMsg(input)) == input
 *
 * @param message A ClrMsg object.
 * @return std::vector<std::string> A vector containing all the data in a ClrMsg as strings.
 */
std::vector<std::string> convertClrMsgToVector(const ClrMsg &message);

/**
 * @brief Convert the data in a vector of strings into a ClrMsg object. Data is expected to be
 * stored in the order:
 *      1. message
 *      2. from
 *      3. to
 *      4. time
 *      5. nonce
 * Note that this function is the inverse of convertVectorToClrMsg. The following is and must remain
 * true:
 *      convertVectorToClrMsg(convertClrMsgToVector(input)) == input
 *
 * @param input
 * @return ClrMsg
 */
ClrMsg convertVectorToClrMsg(const std::vector<std::string> &input);

}  // namespace msh

#endif
