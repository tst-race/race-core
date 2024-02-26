
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

#ifndef __SOURCE_INPUT_RACE_TEST_APP_INPUT_STDIN_H__
#define __SOURCE_INPUT_RACE_TEST_APP_INPUT_STDIN_H__

#include "IRaceTestAppInput.h"

/**
 * @brief Implementation of IRaceTestAppInput interface for receiving client input from standard
 * input.
 *
 */
class RaceTestAppInputStdin : public IRaceTestAppInput {
public:
    /**
     * @brief Get client input from standard input. Function will block until input is received.
     *
     * @return std::string The input received from the client.
     */
    std::string getInputBlocking() override;
};

#endif
