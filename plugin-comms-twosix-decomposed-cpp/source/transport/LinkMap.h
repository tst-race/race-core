
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

#ifndef __COMMS_TWOSIX_TRANSPORT_LINK_MAP_H__
#define __COMMS_TWOSIX_TRANSPORT_LINK_MAP_H__

#include <memory>
#include <mutex>
#include <unordered_map>

#include "Link.h"

class LinkMap {
public:
    int size() const;
    void clear();
    void add(const std::shared_ptr<Link> &link);
    std::shared_ptr<Link> get(const LinkID &linkId) const;
    std::unordered_map<LinkID, std::shared_ptr<Link>> getMap() const;
    std::shared_ptr<Link> remove(const LinkID &linkId);

private:
    mutable std::mutex mutex;
    std::unordered_map<LinkID, std::shared_ptr<Link>> links;
};

#endif  // __COMMS_TWOSIX_TRANSPORT_LINK_MAP_H__