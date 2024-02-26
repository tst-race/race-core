
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

#ifndef __RACE_LOG_H_
#define __RACE_LOG_H_

#include <iomanip>
#include <ios>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <type_traits>

#include "Defer.h"

#ifndef SWIG
template <typename S, typename T>
class is_streamable {
    template <typename SS, typename TT>
    static auto test(int) -> decltype(std::declval<SS &>() << std::declval<TT>(), std::true_type());

    template <typename, typename>
    static auto test(...) -> std::false_type;

public:
    static const bool value = decltype(test<S, T>(0))::value;
};
#endif

class RaceLog {
public:
    enum LogLevel { LL_DEBUG = 0, LL_INFO = 1, LL_WARNING = 2, LL_ERROR = 3 };

    static void log(LogLevel level, const std::string &pluginName, const std::string &message,
                    const std::string &stackTrace);
    static void logDebug(const std::string &pluginName, const std::string &message,
                         const std::string &stackTrace);
    static void logInfo(const std::string &pluginName, const std::string &message,
                        const std::string &stackTrace);
    static void logWarning(const std::string &pluginName, const std::string &message,
                           const std::string &stackTrace);
    static void logError(const std::string &pluginName, const std::string &message,
                         const std::string &stackTrace);

    // Private methods used internally by RaceSDK-Core
    // DO NOT CALL THESE METHODS
    static std::ostream &getLogStream(LogLevel level);
    static void setLogLevel(LogLevel level);
    static void setLogLevelStdout(LogLevel level);
    static void setLogLevelFile(LogLevel level);
    static void setLogFile(const std::string &file);
    static std::string get_this_thread_id_prefix();

    /**
     * @brief demangle an c++ abi name. Useful for converting from a id given by typeid(...).name()
     *
     * @param abiName mangled name
     * @return string containing demangled name
     */
    static std::string cppDemangle(const char *abiName);

    /**
     * @brief These functions converts a comma separated string and a list of arguments into a
     * string of the form 'val1=arg1, val2=arg2, ...'. This is only expected to be used by
     * TRACE_FUNCTION_BASE/TRACE_METHOD_BASE.
     *
     * For example:
     *     RaceLog::stringifyValues("handle, connId, linkId", handle, connId, linkId)
     * could return
     *     "handle=42, connId=example connennction id, linkId=example link id"
     *
     * @param expressionString string containing list of comma separated values
     * @param arg1 The first argument of the argument list
     * @param args The rest of the argument list
     * @return std::string string of the form specified above
     */
    template <typename T, typename... Args>
    static std::string stringifyValuesInternal(const std::string &expressionString, const T &arg1,
                                               const Args &... args) {
        size_t begin = expressionString.find_first_not_of(" \n\r\t");
        size_t end = expressionString.find(",");
        std::string token = expressionString.substr(begin, end - begin);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(16) << token << "=";
        writeArg(ss, arg1);
        if constexpr (sizeof...(args) > 0) {
            ss << ", " << stringifyValuesInternal(expressionString.substr(end + 1), args...);
        }
        return ss.str();
    }

    template <typename... Args>
    static std::string stringifyValues(const std::string &expressionString, const Args &... args) {
        if constexpr (sizeof...(args) > 0) {
            return " with " + stringifyValuesInternal(expressionString, args...);
        } else {
            return "";
        }
    }

private:
    template <typename T,
              typename std::enable_if<is_streamable<std::stringstream, T>::value>::type * = nullptr>
    static void writeArg(std::stringstream &out, const T &arg) {
        out << arg;
    }

    template <typename T, typename std::enable_if<!is_streamable<std::stringstream, T>::value>::type
                              * = nullptr>
    static void writeArg(std::stringstream &out, const T &arg) {
        out << nlohmann::json(arg);
    }
};

/**
 * @brief Create a log prefix based on the class name and the function name. This may only be used
 * from methods.
 */
#define MAKE_LOG_PREFIX() \
    std::string logPrefix = RaceLog::cppDemangle(typeid(*this).name()) + "::" + __func__ + ": "

/**
 * @brief This macro has a number of effects. It creates a log prefix based on the name of the
 * function. It logs using that prefix a string containing the arguments. and it creates a variable
 * that will log when it goes out of scope
 *
 * The string logged is slightly somewhat brittle. No commas may be used in the calling this macro
 * except those used to separate arguments. If additional commas are used, the string printed out be
 * incorrect, as the string parsing used assumes argument names are comma separated. If you want to
 * log the result of a function, it's recommended to assign to a variable beforehand and log that
 * variable.
 *
 * It is recommended to create a plugin-specific macro that pre-sets the plugin name.
 *
 * Example #1:
 *
 * #define TRACE_FUNCTION(...) TRACE_FUNCTION_BASE(MyPluginName, ##__VA_ARGS__)
 *
 * void example_function() {
 *     TRACE_FUNCTION()
 * }
 *
 * Logs:
 *     example_function: called
 *     example_function: returned
 *
 * Example #2:
 *
 * void example_function(RaceHandle handle, ConnectionId connId) {
 *     TRACE_FUNCTION(handle, connId)
 * }
 *
 * Logs:
 *     example_function: called with handle=42, connId=example connection id
 *     example_function: returned
 *
 *
 * @param pluginName Plugin name
 * @param args The arguments to log
 */
#define TRACE_FUNCTION_BASE(pluginName, ...)                                                       \
    std::string logPrefix = std::string(__func__) + ": ";                                          \
    RaceLog::logDebug(                                                                             \
        #pluginName, logPrefix + "called" + RaceLog::stringifyValues(#__VA_ARGS__, ##__VA_ARGS__), \
        "");                                                                                       \
    Defer defer([logPrefix] { RaceLog::logDebug(#pluginName, logPrefix + "returned", ""); })

/**
 * @brief Similar to TRACE_FUNCTION except it gets the class name as well
 *
 * @param pluginName Plugin name
 * @param args The arguments to log
 */
#define TRACE_METHOD_BASE(pluginName, ...)                                                         \
    std::string logPrefix = RaceLog::cppDemangle(typeid(*this).name()) + "::" + __func__ + ": ";   \
    RaceLog::logDebug(                                                                             \
        #pluginName, logPrefix + "called" + RaceLog::stringifyValues(#__VA_ARGS__, ##__VA_ARGS__), \
        "");                                                                                       \
    Defer defer([logPrefix] { RaceLog::logDebug(#pluginName, logPrefix + "returned", ""); })

#endif
