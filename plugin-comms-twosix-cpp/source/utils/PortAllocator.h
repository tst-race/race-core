
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

#ifndef __PORT_ALLOCATOR_H__
#define __PORT_ALLOCATOR_H__

#include <unordered_set>

class PortAllocator {
public:
    /**
     * @brief Construct a PortAllocator instance. Ports will be allocated in the range [min, max).
     *
     * @param min The minimum value in the port range.
     * @param max The maximum value in the port range.
     *
     * @throws std::invalid_arguemtn Exception thrown if the provided port range is invalid.
     */
    PortAllocator(uint16_t min, uint16_t max);

    /**
     * @brief Get an available port or throw an execption if no ports are available.
     *
     * @return uint16_t The available port.
     *
     * @throws std::out_of_range Exception thrown when no more ports are available.
     */
    uint16_t getAvailablePort();

    /**
     * @brief Notify the instance that this port is in use externally and should be marked as in
     * use.
     *
     * @param port The port to marke in use.
     */
    void usePort(uint16_t port);

    /**
     * @brief Notify the instance that the given port is no longer in use and may be reused.
     *
     * @param port The port to release.
     */
    void releasePort(uint16_t port);

    /**
     * @brief Set the port range start.
     *
     * @param start The new start for the port range.
     */
    void setPortRangeStart(uint16_t start);

    /**
     * @brief Set the port range end.
     *
     * @param end The new end for the port range.
     */
    void setPortRangeEnd(uint16_t end);

private:
    uint16_t portRangeStart;
    uint16_t portRangeEnd;
    uint16_t lastUsedPort;
    std::unordered_set<uint16_t> portsInUse;
};

#endif
