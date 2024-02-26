
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

#ifndef __LOG_EXPECT_H__
#define __LOG_EXPECT_H__

#include <fstream>

#include "../../source/filesystem.h"
#include "../../source/helper.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define LOG_EXPECT(logger, func, ...)                                                           \
    {                                                                                           \
        std::string logPrefix = RaceLog::cppDemangle(typeid(*this).name()) + "::" + func + ":"; \
        logger.log(logPrefix, RaceLog::stringifyValues(#__VA_ARGS__, ##__VA_ARGS__));           \
    }

// This should be defined by cmake when creating the test executable
#ifndef EXPECT_LOG_DIR
#define EXPECT_LOG_DIR "/tmp/test-output"
#endif

class LogExpect {
public:
    LogExpect() {}
    LogExpect(std::string test_suite_name, std::string test_name) {
        fs::path output_directory_path = fs::path(EXPECT_LOG_DIR) / "output" / test_suite_name;
        output_file_path = output_directory_path / test_name;
        fs::path expect_directory_path = fs::path(EXPECT_LOG_DIR) / "expected" / test_suite_name;
        expect_file_path = expect_directory_path / test_name;
    }
    ~LogExpect() {}

    void check() {
        checked = true;

        for (auto &[threadName, output_stream] : output_streams) {
            ASSERT_TRUE(output_stream);
            output_stream.flush();

            auto file_path = output_file_path / threadName;
            auto output_file = std::ifstream(file_path.c_str(), std::ios::binary);
            ASSERT_TRUE(output_file) << "Output file does not exist / cannot be read";
            std::string output((std::istreambuf_iterator<char>(output_file)),
                               std::istreambuf_iterator<char>());

            file_path = expect_file_path / threadName;
            auto expect_file = std::ifstream(file_path.c_str(), std::ios::binary);
            ASSERT_TRUE(expect_file) << "Expected file does not exist / cannot be read";
            std::string expected((std::istreambuf_iterator<char>(expect_file)),
                                 std::istreambuf_iterator<char>());

            EXPECT_EQ(expected, output);
        }
    }

    void log(std::string logPrefix, std::string values) {
        ASSERT_FALSE(checked);
        fs::create_directories(output_file_path);

        std::string threadName = helper::get_thread_name();
        if (threadName.empty()) {
            threadName = "default";
        }
        auto &stream = getFileForThread(threadName);
        ASSERT_TRUE(stream);
        stream << logPrefix << " " << values << "\n";
    }

    std::ofstream &getFileForThread(const std::string &threadName) {
        auto it = output_streams.find(threadName);
        if (it != output_streams.end()) {
            return it->second;
        }

        auto file_path = output_file_path / threadName;
        output_streams[threadName] = std::ofstream(file_path.c_str(), std::ios::binary);
        return output_streams[threadName];
    }

protected:
    fs::path output_file_path;
    fs::path expect_file_path;
    std::unordered_map<std::string, std::ofstream> output_streams;
    bool checked = false;
};

#endif