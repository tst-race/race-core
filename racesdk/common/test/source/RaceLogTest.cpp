
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

#include <climits>

#include "RaceLog.h"
#include "gtest/gtest.h"

TEST(RaceLogTest, setLogLevel) {
    RaceLog::setLogLevel(RaceLog::LL_DEBUG);
}

TEST(RaceLogTest, setLogLevel_out_of_bounds_level) {
    RaceLog::setLogLevel(RaceLog::LogLevel(-1));
    RaceLog::setLogLevel(RaceLog::LogLevel(4));
    RaceLog::setLogLevel(RaceLog::LogLevel(INT_MIN));
    RaceLog::setLogLevel(RaceLog::LogLevel(INT_MAX));
}

TEST(RaceLogTest, log_out_of_bounds_level) {
    RaceLog::log(RaceLog::LogLevel(-1), "my plugin name", "my message", "my stack trace");
    RaceLog::log(RaceLog::LogLevel(4), "my plugin name", "my message", "my stack trace");
    RaceLog::log(RaceLog::LogLevel(INT_MIN), "my plugin name", "my message", "my stack trace");
    RaceLog::log(RaceLog::LogLevel(INT_MAX), "my plugin name", "my message", "my stack trace");
}

TEST(RaceLogTest, logDebug) {
    RaceLog::logDebug("my plugin name", "my message", "my stack trace");
}

TEST(RaceLogTest, logInfo) {
    RaceLog::logInfo("my plugin name", "my message", "my stack trace");
}

TEST(RaceLogTest, logWarning) {
    RaceLog::logWarning("my plugin name", "my message", "my stack trace");
}

TEST(RaceLogTest, logError) {
    RaceLog::logError("my plugin name", "my message", "my stack trace");
}
