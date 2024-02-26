
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

#include <RaceLog.h>
#include <gmock/gmock.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#ifndef PYTHON_BINDINGS_PATHDEPS
#pragma GCC error "No paths for Python bindings specified"
#endif

std::int32_t main(std::int32_t argc, char **argv) {
    RaceLog::setLogLevel(RaceLog::LL_DEBUG);
    ::testing::InitGoogleMock(&argc, argv);

    // Update PYTHONPATH to enable loading of stub modules
    if (const char *env_p = std::getenv("PYTHONPATH")) {
        const char *ext = PYTHON_BINDINGS_PATHDEPS;

        char *new_p = static_cast<char *>(malloc(strlen(env_p) + strlen(ext) + 2));
        if (new_p) {
            strcpy(new_p, env_p);
            strcat(new_p, ":");
            strcat(new_p, ext);
            setenv("PYTHONPATH", new_p, 1);
            free(new_p);
        }
    }
    return RUN_ALL_TESTS();
}
