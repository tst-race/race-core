
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

#ifndef __COMMS_TWOSIX_TRANSPORT_MESSAGE_HASH_QUEUE_H__
#define __COMMS_TWOSIX_TRANSPORT_MESSAGE_HASH_QUEUE_H__

#include <cinttypes>
#include <deque>
#include <string>
#include <vector>

class MessageHashQueue {
public:
    std::size_t addMessage(const std::string &message);
    void removeHash(std::size_t hash);
    bool findAndRemoveMessage(const std::string &message);

private:
    static const std::deque<std::size_t>::size_type max{1024};
    static std::size_t hash(const std::string &message);

    std::deque<std::size_t> queue;
};

#endif  // __COMMS_TWOSIX_TRANSPORT_MESSAGE_HASH_QUEUE_H__