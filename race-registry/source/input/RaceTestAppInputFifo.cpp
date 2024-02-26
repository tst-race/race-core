
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

#include "RaceTestAppInputFifo.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "racetestapp/raceTestAppHelpers.h"

RaceTestAppInputFifo::RaceTestAppInputFifo() : fifoFd(-1) {
    const char *fifoName = "/tmp/racetestapp-input";
    fifoFd = open(fifoName, O_RDWR /* O_RDONLY */);
    if (fifoFd == -1) {
        throw std::runtime_error("failed to read fifo");
    }

    // Prevents the app from seeing end-of-file if all the clients close the read end of the
    // pipe.
    int dummyFd = open(fifoName, O_WRONLY);
    if (dummyFd == -1) {
        throw std::runtime_error("failed to open dummy file descriptor to write end of fifo");
    }

    // Ignore the SIGPIPE signal in case the app tries to write to a client fifo that does not
    // have a reader. Otherwise, the process would be killed.
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        throw std::runtime_error("failed to ignore SIGPIPE");
    }
}

std::string RaceTestAppInputFifo::getInputBlocking() {
    static const int bufferSize = 4096;
    char buffer[bufferSize] = {0};
    std::string inputMessage = remainderInput;

    // Read from the fifo until a newline is found.
    while (true) {
        std::string output;
        if (parse(inputMessage, output, remainderInput)) {
            return output;
        }

        // This call will block until there is data to be read.
        const ssize_t readSize = read(fifoFd, buffer, bufferSize);
        if (readSize < 0) {
            if (errno == EINTR) {
                throw std::runtime_error("Intterupted");
            }
            // TODO: better error reporting? Use app output? Return error in returned input with
            // "error:" as prefix? Throw exception?
            std::cerr << "Failed to read fifo: errno: " << errno << std::endl;
            inputMessage.clear();
            continue;
        } else if (readSize > 0) {
            inputMessage.insert(inputMessage.end(), buffer, buffer + readSize);
        }
    }
}

bool RaceTestAppInputFifo::parse(std::string input, std::string &output, std::string &remainder) {
    // parsing next command. The command starts at first brace and ends at the match bracket
    auto beginIndex = input.find('{');
    // Check the input for any commands. Commands are terminated with newline characters.
    if (beginIndex != std::string::npos) {
        int depth = 0;
        for (auto i = beginIndex; i < input.size(); i++) {
            if (input[i] == '{') {
                depth += 1;
            } else if (input[i] == '}') {
                depth -= 1;
                if (depth == 0) {
                    // found the matching brace
                    remainder = input.substr(i + 1);
                    output = input.substr(beginIndex, i - beginIndex + 1);
                    return true;
                }
            }
        }
    }

    return false;
}
