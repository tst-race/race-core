
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

#include "MessageSerializer.h"

#include <stdexcept>  // std::invalid_argument

#include "helpers.h"

// The fixed length header size for each portion of the message. The header will contain the length
// of the message that follows it in hexadecimal.
// NOTE: The header size will limit the size of the messages that can be sent. The header size is in
// hexadecimal so the size limit will equal (16 ^ HEADER_SIZE) - 1.
constexpr size_t HEADER_SIZE = 7;
constexpr size_t MESSAGE_DATA_SIZE_LIMIT = (1 << (HEADER_SIZE * 4)) - 1;

std::string MessageSerializer::serialize(const ClrMsg &message) {
    if (message.getMsg().size() > MESSAGE_DATA_SIZE_LIMIT ||
        message.getFrom().size() > MESSAGE_DATA_SIZE_LIMIT ||
        message.getTo().size() > MESSAGE_DATA_SIZE_LIMIT) {
        throw std::invalid_argument(
            "MessageSerializer::serialize(): message exceeds size limit of " +
            std::to_string(MESSAGE_DATA_SIZE_LIMIT));
    }
    std::string seriliazedMessage;

    std::vector<std::string> messageValues = msh::convertClrMsgToVector(message);
    for (const auto &value : messageValues) {
        msh::appendDataToSerializedMessage(seriliazedMessage, value, HEADER_SIZE);
    }

    return seriliazedMessage;
}

ClrMsg MessageSerializer::deserialize(const std::string &serializedMessage) {
    try {
        std::vector<std::string> messageValues;
        for (size_t pointer = 0; pointer < serializedMessage.length();) {
            const size_t dataSize =
                msh::convertFromHexString(serializedMessage.substr(pointer, HEADER_SIZE));
            pointer += HEADER_SIZE;
            messageValues.push_back(serializedMessage.substr(pointer, dataSize));
            pointer += dataSize;
        }

        return msh::convertVectorToClrMsg(messageValues);
    } catch (...) {
        throw std::invalid_argument("Invalid message to parse");
    }

    return ClrMsg("", "", "", 0, 0);
}
