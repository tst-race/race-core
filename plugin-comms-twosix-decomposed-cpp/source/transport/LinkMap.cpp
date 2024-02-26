
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

#include "LinkMap.h"

int LinkMap::size() const {
    std::lock_guard<std::mutex> lock(mutex);
    return links.size();
}

void LinkMap::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    links.clear();
}

void LinkMap::add(const std::shared_ptr<Link> &link) {
    std::lock_guard<std::mutex> lock(mutex);
    links[link->getId()] = link;
}

std::shared_ptr<Link> LinkMap::get(const LinkID &linkId) const {
    std::lock_guard<std::mutex> lock(mutex);
    return links.at(linkId);
}

std::unordered_map<LinkID, std::shared_ptr<Link>> LinkMap::getMap() const {
    std::lock_guard<std::mutex> lock(mutex);
    return links;
}

std::shared_ptr<Link> LinkMap::remove(const LinkID &linkId) {
    std::lock_guard<std::mutex> lock(mutex);
    std::shared_ptr<Link> value;
    auto iter = links.find(linkId);
    if (iter != links.end()) {
        value = iter->second;
        links.erase(iter);
    }
    return value;
}