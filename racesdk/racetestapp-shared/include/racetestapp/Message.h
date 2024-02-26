
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

#ifndef __SOURCE_MESSAGE_MESSAGE_H__
#define __SOURCE_MESSAGE_MESSAGE_H__

#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace rta {

/**
 * @brief Class to represent a message in the RaceTestApp.
 *
 */
class Message {
public:
    using Clock = std::chrono::system_clock;
    using Millis = std::chrono::milliseconds;
    using Time = std::chrono::time_point<Clock>;

    Message() = delete;
    Message(const std::string &message, const std::string &recipient, Time sendTime,
            std::string_view _generated, bool isNMBypass,
            const std::string &networkManagerBypassRoute);

    /**
     * @brief Create a Message given an application input string.
     *
     * @param inputMessage The input message to the application.
     * @return vector<Message> The resulting Messages to be sent.
     * @throw On error a std::invalid_argument exception is thrown.
     */
    static std::vector<Message> createMessage(const nlohmann::json &inputMessage);

    /**
     * @brief The content of the message stored in a string.
     *
     */
    std::string messageContent;

    /**
     * @brief The randomly generated part of the message, or empty string for manual messages.
     *
     */
    std::string_view generated;

    /**
     * @brief The peronsa of the recipient stored in a string.
     *
     */
    std::string personaOfRecipient;

    Time sendTime;

    /**
     * @brief The message is to be send bypassing network manager processing.
     */
    bool isNMBypass;

    /**
     * @brief Route (connection ID, link ID, or channel ID) by which to send the
     * network-manager-bypass message.
     */
    std::string networkManagerBypassRoute;

    /**
     * @brief The number of bytes used to store the sequence number string
     *
     */
    static const uint64_t sequenceStringLength;

private:
    static std::vector<Message> parseSendMessage(const nlohmann::json &payload);
    static std::vector<Message> parseAutoMessage(const nlohmann::json &payload);
    static std::vector<Message> parseTestPlanMessage(const nlohmann::json &payload);

    /**
     * @brief zero pad a number until it's sequenceStringLength
     *
     * @param sequenceNumber The sequence number to insert pad with zeros
     */
    static std::string sequenceNumberToString(size_t sequenceNumber);
};

}  // namespace rta

#endif
