
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

#include <fstream>  // std::ofstream

#include "../../../source/input/RaceTestAppInputFifo.h"
#include "gtest/gtest.h"

const std::string fifoFilePath = "/tmp/racetestapp-input";

inline void createFifo() {
    if (mkfifo(fifoFilePath.c_str(), S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        throw "failed to create fifo";
    }

    std::fstream file(fifoFilePath.c_str());
    ASSERT_TRUE(file.good()) << "Fifo is not readable";
}

TEST(RaceTestAppInputFifo, should_read_what_is_written_to_fifo) {
    createFifo();
    RaceTestAppInputFifo input;

    {
        std::ofstream fifoToWriteTo(fifoFilePath, std::ofstream::out);
        fifoToWriteTo << "{some crazy message to send to the fifo}";
    }

    const std::string fifoInput = input.getInputBlocking();

    EXPECT_EQ(fifoInput, "{some crazy message to send to the fifo}");
}

TEST(RaceTestAppInputFifo, should_read_multiple_messages) {
    createFifo();
    RaceTestAppInputFifo input;

    {
        std::ofstream fifoToWriteTo(fifoFilePath, std::ofstream::out);
        fifoToWriteTo << "{first}zxcv{second}asdf{third}{fourth}\n{last}";
    }

    std::string fifoInput = input.getInputBlocking();
    ASSERT_EQ(fifoInput, "{first}");
    fifoInput = input.getInputBlocking();
    ASSERT_EQ(fifoInput, "{second}");
    fifoInput = input.getInputBlocking();
    ASSERT_EQ(fifoInput, "{third}");
    fifoInput = input.getInputBlocking();
    ASSERT_EQ(fifoInput, "{fourth}");
}

TEST(RaceTestAppInputFifo, stress_test) {
    createFifo();
    RaceTestAppInputFifo input;

    const int32_t numMessagesToWriteToFifo = 8192;
    {
        std::ofstream fifoToWriteTo(fifoFilePath, std::ofstream::out);
        for (long i = 0; i < numMessagesToWriteToFifo; ++i) {
            fifoToWriteTo << "{" << std::to_string(i) << "}";
        }
    }

    for (std::int32_t i = 0; i < numMessagesToWriteToFifo; ++i) {
        std::string fifoInput = input.getInputBlocking();
        ASSERT_EQ(fifoInput, "{" + std::to_string(i) + "}") << "Failed for value " << i;
    }
}
