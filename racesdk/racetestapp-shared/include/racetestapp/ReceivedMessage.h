
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

#ifndef __SOURCE_CLIENT_RECEIVED_MESSAGE_H__
#define __SOURCE_CLIENT_RECEIVED_MESSAGE_H__

#include "ClrMsg.h"

/**
 * @brief Class to hold a ClrMsg that was received by the application.
 *
 */
class ReceivedMessage : public ClrMsg {
public:
    /**
     * @brief Don't allow the class to be instantiated without a ClrMsg.
     *
     */
    ReceivedMessage() = delete;

    /**
     * @brief Construct a ReceivedMessage from a ClrMsg. Will set the received time to the current
     * time.
     *
     * @param msg
     */
    explicit ReceivedMessage(const ClrMsg &msg);

    /**
     * @brief The time in milliseconds that the message was received.
     *
     */
    const std::int64_t receivedTime;
};

#endif
