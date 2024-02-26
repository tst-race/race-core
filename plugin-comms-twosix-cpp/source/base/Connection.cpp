
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

#include "Connection.h"

#include "Link.h"

Connection::Connection(const ConnectionID &_connectionId, LinkType _linkType,
                       const std::shared_ptr<Link> &_link, const std::string &linkHints,
                       int _timeout) :
    link(_link),
    connectionId(_connectionId),
    linkType(_linkType),
    linkHints(linkHints),
    timeout(_timeout),
    available(true) {}

std::shared_ptr<Link> Connection::getLink() {
    return std::shared_ptr<Link>(link);  // throws std::bad_weak_ptr if promotion fails
}
