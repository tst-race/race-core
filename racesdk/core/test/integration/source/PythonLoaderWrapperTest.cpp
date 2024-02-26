
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

#include <sys/stat.h>

#include <exception>
#include <fstream>
#include <thread>

#include "../../../source/CommsWrapper.h"
#include "../../../source/NMWrapper.h"
#include "../../../source/plugin-loading/PythonLoaderWrapper.h"
#include "gtest/gtest.h"

/**
 * @brief Fixture class template for the typed test.
 *
 * @tparam T The type, either NMWrapper or CommsWrapper.
 */
template <typename T>
class PythonLoaderWrapperTypedTest : public testing::Test {
public:
    PythonLoaderWrapperTypedTest() {}
};

// Set up the typed test suite with the two types to test.
using MyTypes = ::testing::Types<NMWrapper, CommsWrapper>;
TYPED_TEST_SUITE(PythonLoaderWrapperTypedTest, MyTypes);

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wsuggest-override"
#endif

/**
 * @brief This test is no longer relevant and has been removed. However, leaving the template here
 * for anyone who may want to add typed tests.
 *
 */
TYPED_TEST(PythonLoaderWrapperTypedTest, DISABLEDupdatePythonPath_sets_PYTHONPATH) {}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
