
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

#ifndef __SOURCE_OUTPUT_RACE_TEST_APP_OUTPUT_FILE_H__
#define __SOURCE_OUTPUT_RACE_TEST_APP_OUTPUT_FILE_H__

#include <fstream>
#include <mutex>

#include "racetestapp/IRaceTestAppOutput.h"

/**
 * @brief Implementation of the IRaceTestAppOutput interface used for writing output to log file.
 *
 */
class RaceTestAppOutputLog : public IRaceTestAppOutput {
public:
    /**
     * @brief Constructor
     *
     */
    explicit RaceTestAppOutputLog(const std::string &logDir);

    virtual ~RaceTestAppOutputLog();

    /**
     * @brief Write output to log file.
     *
     * @param output The output message to be written to a log file.
     */
    // cppcheck-suppress virtualCallInConstructor ; false positive that should be fixed in later
    // version of cppcheck
    void writeOutput(const std::string &output) override;

private:
    /**
     * @brief Output stream to the log file.
     *
     */
    std::ofstream outputFile;

    /**
     * @brief Mutex to prevent threading from mixing logs.
     *
     */
    std::mutex logMutex;
};

#endif
