
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

#include "helpers.h"

#include <iomanip>    // std::setfill, std::setw
#include <sstream>    // std::stringstream
#include <stdexcept>  // std::invalid_argument

std::string msh::convertToHexString(size_t input, size_t paddedLength) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(paddedLength) << std::hex << input;
    return stream.str();
}

size_t msh::convertFromHexString(const std::string &hexString) {
    if (hexString.length() > 0 && hexString[0] == '-') {
        return 0;
    }

    try {
        // TODO: any valid concerns for casting 'unsigned long' to 'size_t'?
        return static_cast<size_t>(std::stoul(hexString, nullptr, 16));
    } catch (std::invalid_argument &err) {
    } catch (std::out_of_range &err) {
    }

    return 0;
}

void msh::appendDataToSerializedMessage(std::string &seriliazedMessage, const std::string &data,
                                        const size_t headerSize) {
    std::string header = msh::convertToHexString(data.length(), headerSize);
    seriliazedMessage += header;
    seriliazedMessage += data;
}

std::vector<std::string> msh::convertClrMsgToVector(const ClrMsg &message) {
    std::vector<std::string> result;
    result.push_back(message.getMsg());
    result.push_back(message.getFrom());
    result.push_back(message.getTo());
    result.push_back(std::to_string(message.getTime()));
    result.push_back(std::to_string(message.getNonce()));
    return result;
}

ClrMsg msh::convertVectorToClrMsg(const std::vector<std::string> &input) {
    const std::int32_t numDataMembersInClrMsg = 5;
    if (input.size() != numDataMembersInClrMsg) {
        throw std::invalid_argument("invalid input size");
    }

    return ClrMsg(input[0], input[1], input[2], std::stol(input[3]), std::stoi(input[4]));
}
