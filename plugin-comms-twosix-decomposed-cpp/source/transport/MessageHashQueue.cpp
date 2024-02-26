
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

#include "MessageHashQueue.h"

#include <algorithm>
#include <string>

std::size_t MessageHashQueue::hash(const std::string &message) {
    return std::hash<std::string>()({message.begin(), message.end()});
}

std::size_t MessageHashQueue::addMessage(const std::string &message) {
    if (queue.size() > max) {
        queue.pop_front();
    }
    auto msgHash = hash(message);
    queue.push_back(msgHash);
    return msgHash;
}

void MessageHashQueue::removeHash(std::size_t hash) {
    auto iter = std::find(queue.begin(), queue.end(), hash);
    if (iter != queue.end()) {
        queue.erase(iter);
    }
}

bool MessageHashQueue::findAndRemoveMessage(const std::string &message) {
    auto iter = std::find(queue.begin(), queue.end(), hash(message));
    bool found = iter != queue.end();
    if (found) {
        queue.erase(queue.begin(), iter + 1);
    }
    return found;
}