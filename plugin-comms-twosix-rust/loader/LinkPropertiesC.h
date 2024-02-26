
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

/*
 *
 * The C definition of LinkPropertiesC, also defined in shims/src/race_common/link_properties.rs.
 * This struct bridges the gap between the Rust struct and the C++ class. The Rust struct values
 * can be copied into an instance of this struct, sent over the C FFI boundary, and copied into an
 * instance of the C++ class.
 *
 */

#ifndef _LOADER_LINK_PROPERTIES_C_H__
#define _LOADER_LINK_PROPERTIES_C_H__

#include <cstdint>

#include "LinkProperties.h"

extern "C" {
struct LinkPropertySetC {
    std::int32_t bandwidth_bps;
    std::int32_t latency_ms;
    float loss;
};

struct LinkPropertyPairC {
    LinkPropertySetC send;
    LinkPropertySetC receive;
};

struct LinkPropertiesC {
    LinkType linkType;
    TransmissionType transmissionType;
    ConnectionType connectionType;
    SendType sendType;
    bool reliable;
    bool isFlushable;
    std::int32_t duration_s;
    std::int32_t period_s;
    std::int32_t mtu;
    LinkPropertyPairC worst;
    LinkPropertyPairC expected;
    LinkPropertyPairC best;
    /**
     * @brief opaque pointer to a C++ data structure that holds a list of supported hints.
     *
     */
    void *supported_hints;
    void *channelGid;
    void *linkAddress;
};

/**
 * @brief Allocate memory for a LinkPropertiesC struct. The memory is allocated on the C++ side. The
 * caller of this function is responsible for cleaning up this memory by calling
 * destroy_link_properties on the instance when they are done with it. This function is expected to
 * be called from Rust when it is necessary to send link properties to the sdk as an argument to an
 * API call.
 *
 * @return LinkPropertiesC The allocated struct.
 */
LinkPropertiesC create_link_properties();

/**
 * @brief Delete the memory allocated for a LinkPropertiesC struct instance generated from a call to
 * create_link_properties. This function is expected to be called from Rust when it is necessary to
 * send link properties to the sdk as an argument to an API call.
 *
 * @param props
 */
void destroy_link_properties(LinkPropertiesC *props);

/**
 * @brief Add a supported hint to the LinkPropertiesC struct instance. This function is expected to
 * be called by Rust to add hints to the supported hints data structure. Note that the
 * supported_hints pointer in the struct is opaque and no attempt should be made from Rust to modify
 * it.
 *
 * @param props A pointer to the LinkPropertiesC struct.
 * @param hint The hint to add.
 */
void add_supported_hint_to_link_properties(LinkPropertiesC *props, const char *hint);

void set_channel_gid_for_link_properties(LinkPropertiesC *props, const char *channelGid);

void set_link_address_for_link_properties(LinkPropertiesC *props, const char *linkAddress);
}

#endif
