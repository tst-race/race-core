
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

#include <stdlib.h>  // unsetenv, putenv

#include "gtest/gtest.h"
#include "racetestapp/raceTestAppHelpers.h"

TEST(getPersona, should_throw_if_persona_not_set) {
    unsetenv("RACE_PERSONA");  // Cheating so that we can actually test this thing.
    EXPECT_THROW(rtah::getPersona(), rtah::race_persona_unset);
}

TEST(getPersona, should_get_the_persona) {
    const char *env_var = "RACE_PERSONA";
    const char *env_value = "MY_TEST_PERSONA";
    setenv(env_var, env_value, true);  // Cheating so that we can actually test this thing.
    EXPECT_EQ(rtah::getPersona(), env_value);
}
