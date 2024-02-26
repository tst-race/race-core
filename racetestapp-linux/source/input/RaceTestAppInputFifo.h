
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

#ifndef __SOURCE_INPUT_RACE_TEST_APP_INPUT_FIFO_H__
#define __SOURCE_INPUT_RACE_TEST_APP_INPUT_FIFO_H__

#include "IRaceTestAppInput.h"

/**
 * @brief Implementation of the IRaceTestAppInput interface for receiving client input from a fifo.
 *
 */
class RaceTestAppInputFifo : public IRaceTestAppInput {
public:
    /**
     * @brief Construct a new Race Test App Input Fifo object.
     *
     * @throw std::string if construction fails.
     */
    RaceTestAppInputFifo();

    /**
     * @brief Get the input from the fifo. Will block until there is something to read.
     *
     * @return std::string The input message from the fifo.
     */
    std::string getInputBlocking() override;

private:
    bool parse(std::string input, std::string &output, std::string &remainder);

    int fifoFd;
    std::string remainderInput;
};

#endif
