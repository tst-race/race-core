
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

#ifndef __SOURCE_OUTPUT_I_RACE_TEST_APP_OUTPUT_H__
#define __SOURCE_OUTPUT_I_RACE_TEST_APP_OUTPUT_H__

#include <string>

/**
 * @brief Interface for writing output that will be received by the client.
 *
 */
class IRaceTestAppOutput {
public:
    /**
     * @brief Write output to the output interface for the client to view.
     *
     * @param output The output message to be written to output.
     */
    virtual void writeOutput(const std::string &output) = 0;
};

#endif
