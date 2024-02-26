
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

#include <string>
#include <vector>

#include "IRacePluginNM.h"

/**
 * @brief encode the provided vector into a string in base64 format
 * @param data The data to encode
 *
 * @return string containing the encoded data
 */
std::string base64_encode(const std::vector<uint8_t> &data);

/**
 * @brief decode the base64 string into bytes
 * @param b64 a string containing base64 data
 *
 * @return a vector containing decoded data
 */
std::vector<uint8_t> base64_decode(const std::string &b64);

/**
 * @brief Check a channel to see if there is room to add links
 *
 * @param sdk the raceSdk that contains the channel to check
 * @param channelGid the name of the channel to check
 * @return true if the number of links on the given channel is at or exceeds the max links for
 * the channel
 */
bool channelLinksFull(IRaceSdkNM *sdk, const std::string &channelGid);

/**
 * @brief Get a string representation of a persona list
 *
 * @param personas A list of persona strings
 * @return string representation of the list of personas
 */
std::string personasToString(std::vector<std::string> personas);