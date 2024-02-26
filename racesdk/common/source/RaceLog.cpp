
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

#include "RaceLog.h"

#include <cxxabi.h>  // needed for abi::__cxa_demangle

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include <algorithm>
#include <atomic>  // std::atomic
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <mutex>
#include <sstream>        // std::stringstream
#include <thread>         // std::this_thread
#include <unordered_map>  // std::unordered_map

/*
 * A sublcass of streambuf that allows for writes to a single ostream to write
 * to both standard-out and to a file at the same time.
 */
class teebuf : public std::streambuf {
private:
    virtual int overflow(int c) override {
        int res = c;
        if (c == EOF) {
            return 0;
        }

        if (levelWrite >= levelCout) {
            res = cout.sputc(c) == EOF ? EOF : res;
        }
        if (file.is_open() && levelWrite >= levelFile) {
            res = file.sputc(c) == EOF ? EOF : res;
        }

        return res;
    }

    virtual std::streamsize xsputn(const char_type *s, std::streamsize count) override {
        std::streamsize out = count;

        if (levelWrite >= levelCout) {
            out = std::min(cout.sputn(s, count), out);
        }
        if (file.is_open() && levelWrite >= levelFile) {
            out = std::min(file.sputn(s, count), out);
        }

        return out;
    }

    virtual int sync() override {
        int res = cout.pubsync();
        if (file.is_open()) {
            res |= file.pubsync();
        }
        return res;
    }

public:
    void open_file(const std::string &name) {
        // cppcheck-suppress ignoredReturnValue
        file.open(name, std::ios_base::out | std::ios_base::app);
    }
    void close_file() {
        file.close();
    }
    void setup_hook(std::ostream &o) {
        o.rdbuf(this);
    }

    void set_level(RaceLog::LogLevel level) {
        levelWrite = level;
    }
    void set_level_cout(RaceLog::LogLevel level) {
        levelCout = level;
    }
    void set_level_file(RaceLog::LogLevel level) {
        levelFile = level;
    }

private:
    std::streambuf &cout = *std::cout.rdbuf();
    std::filebuf file;

    std::atomic<RaceLog::LogLevel> levelWrite = RaceLog::LL_DEBUG;
    std::atomic<RaceLog::LogLevel> levelCout = RaceLog::LL_INFO;
    std::atomic<RaceLog::LogLevel> levelFile = RaceLog::LL_INFO;
};
static teebuf logBuffer;
static std::ostream logStream(&logBuffer);

std::ostream &RaceLog::getLogStream(RaceLog::LogLevel level) {
    logBuffer.set_level(level);
    return logStream;
}

/**
 * @brief Convert a log level value to a human readable string.
 *
 * @param level The log level value to convert.
 * @return const char* A human readable string describing the log
 */
inline const char *logLevelToStr(RaceLog::LogLevel level) {
    static std::unordered_map<RaceLog::LogLevel, const char *> logLevelStrs = {
        {RaceLog::LL_DEBUG, "DEBUG"},
        {RaceLog::LL_INFO, "INFO"},
        {RaceLog::LL_WARNING, "WARNING"},
        {RaceLog::LL_ERROR, "ERROR"}};
    try {
        return logLevelStrs.at(level);
    } catch (const std::out_of_range &) {
        return "INVALID LOG LEVEL";
    }
}

static std::mutex logMutex;

void RaceLog::setLogLevel(LogLevel level) {
    logBuffer.set_level_cout(level);
    logBuffer.set_level_file(level);
}
void RaceLog::setLogLevelStdout(LogLevel level) {
    logBuffer.set_level_cout(level);
    log(level, "RaceLog", "log level stdout changed to: " + std::string(logLevelToStr(level)), "");
}
void RaceLog::setLogLevelFile(LogLevel level) {
    logBuffer.set_level_file(level);
    log(level, "RaceLog", "log level file changed to: " + std::string(logLevelToStr(level)), "");
}

void RaceLog::setLogFile(const std::string &file) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (file.empty()) {
        logBuffer.close_file();
    } else {
        logBuffer.open_file(file);
    }
}

void RaceLog::log(LogLevel level, const std::string &pluginName, const std::string &message,
                  const std::string &stackTrace) {
    std::lock_guard<std::mutex> lock(logMutex);
    logBuffer.set_level(level);

    // C++20 will add time format functionality for <chrono>
    const auto now = std::chrono::system_clock::now();
    std::time_t nowFormatted = std::chrono::system_clock::to_time_t(now);
    const auto nowMs =
        std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    logStream << std::put_time(std::localtime(&nowFormatted), "%F %T") << '.' << std::setfill('0')
              << std::setw(6) << nowMs.count() << ": ";

    logStream << logLevelToStr(level) << ": " << pluginName << ": " << message << "\n";
    if (stackTrace.length() > 0) {
        logStream << stackTrace << "\n";
    }
    logStream << std::flush;

#ifdef __ANDROID__
    std::string androidLog = logLevelToStr(level) + std::string(" ") + pluginName + ": " + message;
    __android_log_print(ANDROID_LOG_DEBUG, "RaceLog", "%s", androidLog.c_str());
#endif
}

std::string RaceLog::get_this_thread_id_prefix() {
    std::stringstream s;
    s << "(thread=" << std::hex << std::this_thread::get_id() << "): ";
    return s.str();
}

void RaceLog::logDebug(const std::string &pluginName, const std::string &message,
                       const std::string &stackTrace) {
    log(LL_DEBUG, pluginName, get_this_thread_id_prefix() + message, stackTrace);
}

void RaceLog::logInfo(const std::string &pluginName, const std::string &message,
                      const std::string &stackTrace) {
    log(LL_INFO, pluginName, get_this_thread_id_prefix() + message, stackTrace);
}

void RaceLog::logWarning(const std::string &pluginName, const std::string &message,
                         const std::string &stackTrace) {
    log(LL_WARNING, pluginName, get_this_thread_id_prefix() + message, stackTrace);
}

void RaceLog::logError(const std::string &pluginName, const std::string &message,
                       const std::string &stackTrace) {
    log(LL_ERROR, pluginName, get_this_thread_id_prefix() + message, stackTrace);
}

// see https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.3/a01696.html for details
std::string RaceLog::cppDemangle(const char *abiName) {
    int status;
    char *demangled = abi::__cxa_demangle(abiName, 0, 0, &status);

    if (status != 0) {
        // demangling failed. return an empty string
        return "";
    } else {
        // demangling succeded. we are responsible for freeing the returned string
        std::string ret = demangled;
        free(demangled);
        return ret;
    }
}
