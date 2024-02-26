
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

#include "RaceException.h"
#include "gtest/gtest.h"

TEST(RaceException, setMessage) {
    RaceException exception;
    exception.setMessage("some message");
    EXPECT_EQ(exception.getMessage(), "some message");
}

TEST(RaceException, getMessage_defaults_to_empty_string) {
    RaceException exception;
    EXPECT_EQ(exception.getMessage(), "");
}