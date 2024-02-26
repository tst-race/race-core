
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

#include <stdlib.h>  // putenv

#include "gtest/gtest.h"
#include "racetestapp/raceTestAppHelpers.h"

TEST(getEnvironmentVariable, returns_empty_string_for_non_existant_env_var) {
    EXPECT_EQ(rtah::getEnvironmentVariable("SOME_FAKE_ENV_VAR"), "");
}

TEST(getEnvironmentVariable, returns_existing_env_var_value) {
    const char *env_var = "ENV_VAR_FOR_UNIT_TESTING_RACE_TEST_APP";
    const char *env_value = "MY_TEST_VALUE";
    setenv(env_var, env_value, true);
    EXPECT_EQ(rtah::getEnvironmentVariable(env_var), env_value);
}
