
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

#ifndef _CHANNEL_PROPERTIES_C_H__
#define _CHANNEL_PROPERTIES_C_H__

#include "ChannelProperties.h"
#include "ChannelRoleC.h"
#include "ConnectionType.h"
#include "LinkProperties.h"
#include "LinkPropertiesC.h"
#include "LinkType.h"
#include "SendType.h"
#include "TransmissionType.h"

extern "C" {

struct ChannelPropertiesC {
    ChannelStatus channelStatus;
    LinkDirection linkDirection;
    TransmissionType transmissionType;
    ConnectionType connectionType;
    SendType sendType;
    bool multiAddressable;
    bool reliable;
    bool bootstrap;
    bool isFlushable;
    std::int32_t duration_s;
    std::int32_t period_s;
    std::int32_t mtu;
    LinkPropertyPairC creatorExpected;
    LinkPropertyPairC loaderExpected;
    void *supported_hints;
    int32_t maxLinks;
    int32_t creatorsPerLoader;
    int32_t loadersPerCreator;
    ChannelRoleC *roles;
    size_t rolesLen;
    ChannelRoleC currentRole;
    int32_t maxSendsPerInterval;
    int32_t secondsPerInterval;
    uint64_t intervalEndTime;
    int32_t sendsRemainingInInterval;
    void *channelGid;
};
/**
 * @brief Allocate memory for a ChannelPropertiesC struct. The memory is allocated on the C++ side.
 * The caller of this function is responsible for cleaning up this memory by calling
 * destroy_channel_properties on the instance when they are done with it. This function is expected
 * to be called from Rust when it is necessary to send channel properties to the sdk as an argument
 * to an API call.
 *
 * @return ChannelPropertiesC The allocated struct.
 */
ChannelPropertiesC create_channel_properties();

/**
 * @brief Delete the memory allocated for a ChannelPropertiesC struct instance generated from a call
 * to create_channel_properties. This function is expected to be called from Rust when it is
 * necessary to send channel properties to the sdk as an argument to an API call.
 *
 * @param props
 */
void destroy_channel_properties(ChannelPropertiesC *props);

/**
 * @brief Add a supported hint to the ChannelPropertiesC struct instance. This function is expected
 * to be called by Rust to add hints to the supported hints data structure. Note that the
 * supported_hints pointer in the struct is opaque and no attempt should be made from Rust to modify
 * it.
 *
 * @param props A pointer to the ChannelPropertiesC struct.
 * @param hint The hint to add.
 */
void add_supported_hint_to_channel_properties(ChannelPropertiesC *props, const char *hint);

void set_channel_gid_for_channel_properties(ChannelPropertiesC *props, const char *channelGid);

void resize_roles_for_channel_properties(ChannelPropertiesC *props, size_t size);

const char **get_supported_hints_for_channel_properties(ChannelPropertiesC *props,
                                                        size_t *vector_length);

const char *get_channel_gid_for_channel_properties(ChannelPropertiesC *props);
}

#endif
