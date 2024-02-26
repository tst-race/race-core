
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

#ifndef __CHANNEL_PROPERTIES_H_
#define __CHANNEL_PROPERTIES_H_

#include <cstdint>
#include <string>
#include <vector>

#include "ChannelRole.h"
#include "ChannelStatus.h"
#include "ConnectionType.h"
#include "LinkProperties.h"
#include "LinkType.h"
#include "SendType.h"
#include "TransmissionType.h"

enum LinkDirection {
    LD_UNDEF = 0,              // undefined
    LD_CREATOR_TO_LOADER = 1,  // creator sends to loader
    LD_LOADER_TO_CREATOR = 2,  // loader sends to creator
    LD_BIDI = 3                // bi-directional
};

class ChannelProperties {
public:
    /**
     * @brief Construct a new Channel Properties object.
     *
     */
    ChannelProperties();

    /**
     * @brief The status of the channel
     *
     */
    ChannelStatus channelStatus;

    /**
     * @brief The directionality of the final link relative to creator and loader
     *
     */
    LinkDirection linkDirection;

    /**
     * @brief If the channel's transmission is unicast or multicast
     *
     */
    TransmissionType transmissionType;

    /**
     * @brief The connection type, i.e. direct or indirect.
     *
     */
    ConnectionType connectionType;

    /**
     * @brief The send type of the channel
     *
     */
    SendType sendType;

    /**
     * @brief If the channel supports the loadLinkAddresses API (multiple addresses)
     *
     */
    bool multiAddressable;

    /**
     * @brief If the channel is reliable or not
     *
     */
    bool reliable;

    /**
     * @brief If the channel can be used for bootstrapping
     *
     */
    bool bootstrap;

    /**
     * @brief Should be set to true if the channel supports being flushed by the network manager,
     * otherwise false.
     *
     */
    bool isFlushable;

    /**
     * @brief The maximum amount of time the Channel can remain open
     *
     */
    std::int32_t duration_s;

    /**
     * @brief The amount of time (in seconds) the channel must wait between activations
     * (inclusive of duration)
     *
     */
    std::int32_t period_s;

    /**
     * @brief Maximum transmission unit of the channel (in Bytes).
     *
     */
    std::int32_t mtu;

    /**
     * @brief Expected performance of creator-side
     *
     */
    LinkPropertyPair creatorExpected;

    /**
     * @brief Expected performance of loader-side
     *
     */
    LinkPropertyPair loaderExpected;

    /**
     * @brief List of hint names supported by this channel
     *
     */
    std::vector<std::string> supported_hints;

    int maxLinks;

    int creatorsPerLoader;
    int loadersPerCreator;

    std::vector<ChannelRole> roles;

    ChannelRole currentRole;

    /**
     * @brief Maximum number of sends allowed within a given interval
     *
     * If this channel does not have a sends-per-interval constraint, this value will be -1.
     */
    std::int32_t maxSendsPerInterval;

    /**
     * @brief Number of seconds in each interval
     *
     * If this channel does not have a sends-per-interval constraint, this value will be -1.
     */
    std::int32_t secondsPerInterval;

    /**
     * @brief Time (unix epoch) at which the current interval will end
     *
     * If this channel does not have a sends-per-interval constraint, this value will be 0.
     */
    std::uint64_t intervalEndTime;

    /**
     * @brief Number of sends remaining in the current interval
     *
     * If this channel does not have a sends-per-interval constraint, this value will be -1.
     */
    std::int32_t sendsRemainingInInterval;

    /**
     * @brief Name of this channel
     *
     */
    std::string channelGid;
};

inline bool operator==(const ChannelProperties &a, const ChannelProperties &b) {
    return a.linkDirection == b.linkDirection && a.channelGid == b.channelGid &&
           a.sendType == b.sendType && a.transmissionType == b.transmissionType &&
           a.connectionType == b.connectionType && a.loaderExpected == b.loaderExpected &&
           a.creatorExpected == b.creatorExpected && a.mtu == b.mtu && a.reliable == b.reliable &&
           a.duration_s == b.duration_s && a.period_s == b.period_s &&
           a.supported_hints == b.supported_hints && a.multiAddressable == b.multiAddressable &&
           a.maxLinks == b.maxLinks && a.creatorsPerLoader == b.creatorsPerLoader &&
           a.loadersPerCreator == b.loadersPerCreator && a.roles == b.roles &&
           a.currentRole == b.currentRole && a.maxSendsPerInterval == b.maxSendsPerInterval &&
           a.secondsPerInterval == b.secondsPerInterval && a.intervalEndTime == b.intervalEndTime &&
           a.sendsRemainingInInterval == b.sendsRemainingInInterval;
}
inline bool operator!=(const ChannelProperties &a, const ChannelProperties &b) {
    return !(a == b);
}

/**
 * @brief Convert a ChannelProperties object to a human readable string. This function is strictly
 * for logging and debugging. The output formatting may change without notice. Do NOT use this for
 * any logical comparisons, etc. The functionality of your plugin should in no way rely on the
 * output of this function.
 *
 * @param props The ChannelProperties object to convert to a string.
 * @return std::string The string representation of the provided object.
 */
std::string channelPropertiesToString(const ChannelProperties &props);

/**
 * @brief Convert a LinkDirection value to a human readable string.
 *
 * @param linkDirection The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string linkDirectionToString(LinkDirection linkDirection);
std::ostream &operator<<(std::ostream &out, LinkDirection linkDirection);

/**
 * @brief Convert a string value to a LinkDirection.
 *
 * @param linkDirectionString The value to convert to a LinkDirection.
 * @return LinkDirection The LinkDirection represented by the string.
 */
LinkDirection linkDirectionFromString(const std::string &linkDirectionString);

/**
 * @brief Compare two ChannelProperties objects and determine if the static properties are equal to
 * each other. Properties are defined as static or dynamic in Channel Properties documentation.
 * Dynamic properties are not considered.
 *
 * @param props1 A ChannelProperties object to compare with.
 * @param props2 A second ChannelProperties object to compare against.
 * @return bool True if static Properties are equal, otherwise False.
 */
bool channelStaticPropertiesEqual(const ChannelProperties &a, const ChannelProperties &b);

#endif
