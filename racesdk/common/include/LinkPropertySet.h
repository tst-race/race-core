
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

#ifndef __LINK_PROPERTY_SET_H_
#define __LINK_PROPERTY_SET_H_

#include <cstdint>
#include <string>
#include <vector>

class LinkPropertySet {
public:
    LinkPropertySet() : bandwidth_bps(-1), latency_ms(-1), loss(-1.) {}

public:
    /**
     * @brief Bandwidth measured in bits per second. If the value is negative then the value
     * is either unset or unknown for this link.
     *
     */
    std::int32_t bandwidth_bps;
    /**
     * @brief latency measured in milliseconds, includes estimated comms plugin processing time on
     * both sides. If the value is negative then the value is either unset or unknown for this link.
     *
     */
    std::int32_t latency_ms;
    /**
     * @brief loss measured in chance-of-loss per-unit. If the value is negative then the value
     * is either unset or unknown for this link.
     *
     */
    float loss;
};

inline bool operator==(const LinkPropertySet &a, const LinkPropertySet &b) {
    return a.bandwidth_bps == b.bandwidth_bps && a.latency_ms == b.latency_ms && a.loss == b.loss;
}
inline bool operator!=(const LinkPropertySet &a, const LinkPropertySet &b) {
    return a.bandwidth_bps != b.bandwidth_bps || a.latency_ms != b.latency_ms || a.loss != b.loss;
}

/**
 * @brief Convert a LinkPropertySet object to a human-readable string. This function is strictly
 * for logging and debugging. The output formatting may change without notice. Do NOT use this for
 * any logical comparisons, etc. The functionality of your plugin should in no way rely on the
 * output of this function.
 *
 * @param props The LinkPropertySet object to convert to a string.
 * @return std::string The string representation of the provided object.
 */
std::string linkPropertySetToString(const LinkPropertySet &props);

#endif
