
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

#include "../../source/utils/PortAllocator.h"

#include "gtest/gtest.h"

// The returned port should be in the range [min, max).
TEST(PortAllocator, returns_port_in_range) {
    const uint16_t min = 5;
    const uint16_t max = 7;

    PortAllocator portAllocator(min, max);

    const auto result = portAllocator.getAvailablePort();

    EXPECT_GE(result, min);
    EXPECT_LE(result, max);
}

// Should throw an expception if the provided port range is invalid.
TEST(PortAllocator, should_throw_for_bad_range) {
    EXPECT_THROW(PortAllocator(10, 5), std::invalid_argument);
    EXPECT_THROW(PortAllocator(1, 1), std::invalid_argument);
}

// Should throw an exception if all the available ports are in use.
TEST(PortAllocator, should_throw_if_no_available_ports) {
    PortAllocator portAllocator(0, 2);
    portAllocator.getAvailablePort();
    portAllocator.getAvailablePort();
    EXPECT_THROW(portAllocator.getAvailablePort(), std::out_of_range);
}

// The port allocator should reuse ports that have been released.
TEST(PortAllocator, reuses_released_ports) {
    const uint16_t min = 5;
    const uint16_t max = 7;

    PortAllocator portAllocator(min, max);

    portAllocator.getAvailablePort();
    portAllocator.releasePort(portAllocator.getAvailablePort());
    portAllocator.getAvailablePort();
}

// Ports marked as in use are not provided as available.
TEST(PortAllocator, can_mark_a_port_as_in_use) {
    const uint16_t min = 5;
    const uint16_t max = 7;

    PortAllocator portAllocator(min, max);

    portAllocator.usePort(min);
    // Port allocator should return the only other available port, i.e. 6.
    EXPECT_EQ(portAllocator.getAvailablePort(), 6);
}

// Setting the start port range updates the start of the range.
TEST(PortAllocator, can_set_start_port_range) {
    const uint16_t min = 10000;
    const uint16_t max = 20000;

    PortAllocator portAllocator(min, max);

    portAllocator.setPortRangeStart(15000);

    EXPECT_EQ(portAllocator.getAvailablePort(), 15000);
}
