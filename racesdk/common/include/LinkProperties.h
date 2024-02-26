
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

#ifndef __LINK_PROPERTIES_H_
#define __LINK_PROPERTIES_H_

#include <cstdint>
#include <string>
#include <vector>

#include "ConnectionType.h"
#include "LinkPropertyPair.h"
#include "LinkType.h"
#include "SendType.h"
#include "TransmissionType.h"

using LinkID = std::string;
using ConnectionID = std::string;

class LinkProperties {
public:
    /**
     * @brief Construct a new Link Properties object.
     *
     */
    LinkProperties();

    /**
     * @brief The type of link.
     *
     */
    LinkType linkType;

    /**
     * @brief If the link's transmission is unicast or multicast
     *
     */
    TransmissionType transmissionType;

    /**
     * @brief The connection type, i.e. direct or indirect.
     *
     */
    ConnectionType connectionType;

    /**
     * @brief The send type of the link
     *
     */
    SendType sendType;

    /**
     * @brief If the link is reliable or not
     *
     */
    bool reliable;

    /**
     * @brief Should be set to true if the link supports being flushed by the network manager,
     * otherwise false.
     *
     */
    bool isFlushable;

    /**
     * @brief The maximum amount of time the Link can remain open
     *
     */
    std::int32_t duration_s;

    /**
     * @brief The amount of time (in seconds) the link must wait between activations
     * (inclusive of duration)
     *
     */
    std::int32_t period_s;

    /**
     * @brief Maximum transmission unit of the link (in Bytes).
     *
     */
    std::int32_t mtu;

    /**
     * @brief Worst Case Properties
     *
     */
    LinkPropertyPair worst;
    /**
     * @brief Expected Case Properties
     *
     */
    LinkPropertyPair expected;
    /**
     * @brief Best Case Properties
     *
     */
    LinkPropertyPair best;

    /**
     * @brief List of hint names supported by this link
     *
     */
    std::vector<std::string> supported_hints;

    /**
     * @brief Name of the channel this link instantiates
     *
     */
    std::string channelGid;

    /**
     * @brief LinkAddress of this link - used by loadLinkAddress to establish a link with this side
     *
     */
    std::string linkAddress;
};

inline bool operator==(const LinkProperties &a, const LinkProperties &b) {
    return a.linkType == b.linkType && a.channelGid == b.channelGid && a.sendType == b.sendType &&
           a.transmissionType == b.transmissionType && a.connectionType == b.connectionType &&
           a.worst == b.worst && a.expected == b.expected && a.best == b.best && a.mtu == b.mtu &&
           a.reliable == b.reliable && a.duration_s == b.duration_s && a.period_s == b.period_s &&
           a.supported_hints == b.supported_hints && a.linkAddress == b.linkAddress;
}
inline bool operator!=(const LinkProperties &a, const LinkProperties &b) {
    return !(a == b);
}

#endif
