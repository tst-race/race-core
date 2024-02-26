
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

#include "PortAllocator.h"

PortAllocator::PortAllocator(uint16_t min, uint16_t max) :
    portRangeStart(min), portRangeEnd(max), lastUsedPort(portRangeStart) {
    if (portRangeStart >= portRangeEnd) {
        throw std::invalid_argument("invalid port range");
    }
}

uint16_t PortAllocator::getAvailablePort() {
    // Throw an exception if there are no more available ports.
    if (portsInUse.size() >= (portRangeEnd - portRangeStart)) {
        throw std::out_of_range("no more available ports");
    }

    // Find and available port.
    while (portsInUse.find(lastUsedPort) != portsInUse.end()) {
        if (lastUsedPort == portRangeEnd) {
            lastUsedPort = portRangeStart;
        }
        ++lastUsedPort;
    }

    // Mark the port as in use.
    this->usePort(lastUsedPort);

    return lastUsedPort;
}

void PortAllocator::usePort(uint16_t port) {
    portsInUse.insert(port);
}

void PortAllocator::releasePort(uint16_t port) {
    portsInUse.erase(port);
}

void PortAllocator::setPortRangeStart(uint16_t start) {
    portRangeStart = start;
    lastUsedPort = portRangeStart;
}

void PortAllocator::setPortRangeEnd(uint16_t end) {
    if (end <= portRangeStart) {
        throw std::out_of_range("invalid value for port range end!");
    }
    portRangeEnd = end;
}
