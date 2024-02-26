
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

#include "racetestapp/RaceTestAppOutputLog.h"

#include <RaceLog.h>  // RaceLog::logError
#include <fcntl.h>    // open, O_WRONLY
#include <signal.h>
#include <string.h>  // strerror
#include <unistd.h>  // close

#include <iostream>  // std::cerr
#include <sstream>   // std::stringstream
#include <system_error>

static const std::string logFileName = "/racetestapp.log";
static const std::string stdoutFileName = "/racetestapp.stdout.log";
static const std::string stderrFileName = "/racetestapp.stderr.log";

constexpr int trap_signals[]{
    SIGABRT, SIGFPE, SIGILL, SIGSEGV, SIGBUS, SIGIOT, SIGSYS,
};

void handle_signal(int signum);

void handle_signal(int signum) {
    std::stringstream errorMessage;
    // stderr goes to a file, so write to stderr for simplicity
    errorMessage << "Signal caught: " << strsignal(signum) << " (" << signum << ")" << std::endl;
    std::cerr << errorMessage.str();

    RaceLog::logError("RaceTestApp", errorMessage.str(), "");

    // Go to the original handler to force core dump
    signal(signum, SIG_DFL);
    raise(signum);
}

RaceTestAppOutputLog::~RaceTestAppOutputLog(){};

RaceTestAppOutputLog::RaceTestAppOutputLog(const std::string &logDir) :
    outputFile((logDir + logFileName).c_str(), std::ofstream::out | std::ofstream::app) {
    // NOTE: Placing this code here for now, since this will be where it is likely to be most needed
    // (others are raceclient/raceserver, or really anything linking in the SDK). It may be better
    // served moving this to the SDK so the same stdout/stderr catchall is applied to all RACE
    // binaries. This was prevented in the past when we were redirecting them to the same file
    // because we needed a file descriptor for dup2. Now that we open new files it should perhaps be
    // reconsidered.

    if (freopen((logDir + stdoutFileName).c_str(), "a", stdout) == NULL) {
        const std::string errorMessage =
            "failed to redirect stdout to log file: " + std::string(strerror(errno));
        std::cerr << errorMessage << std::endl;
        RaceTestAppOutputLog::writeOutput(errorMessage);
        throw std::system_error(EBADF, std::generic_category(), errorMessage);
    }

    if (freopen((logDir + stderrFileName).c_str(), "a", stderr) == NULL) {
        const std::string errorMessage =
            "failed to redirect stderr to log file: " + std::string(strerror(errno));
        std::cerr << errorMessage << std::endl;
        RaceTestAppOutputLog::writeOutput(errorMessage);
        throw std::system_error(EBADF, std::generic_category(), errorMessage);
    }

    for (int signum : trap_signals) {
        signal(signum, &handle_signal);
    }
}

void RaceTestAppOutputLog::writeOutput(const std::string &output) {
    std::lock_guard lock(logMutex);
    outputFile << output << std::endl;
    outputFile.flush();
}
