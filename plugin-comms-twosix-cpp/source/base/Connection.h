
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

#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <IRacePluginComms.h>  // ConnectionID, LinkId, LinkProperties

#include <memory>  // std::shared_ptr, std::weak_ptr

class Link;

class Connection {
protected:
    const std::weak_ptr<Link> link;

public:
    const ConnectionID connectionId;
    const LinkType linkType;
    const std::string linkHints;
    const int timeout;
    bool available = true;

public:
    Connection(const ConnectionID &connectionId, LinkType linkType,
               const std::shared_ptr<Link> &link, const std::string &linkHints, int timeout);

    std::shared_ptr<Link> getLink();
};

#endif