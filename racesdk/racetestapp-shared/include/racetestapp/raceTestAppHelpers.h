
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

#ifndef __SOURCE_HELPERS_H__
#define __SOURCE_HELPERS_H__

#include <ClrMsg.h>  // ClrMsg

#include <string>  // std::string
#include <vector>  // std::vector

#include "racetestapp/IRaceTestAppOutput.h"
#include "racetestapp/ReceivedMessage.h"  // ReceivedMessage

// Race Test App Helpers
namespace rtah {

static const std::string appNameForLogging = "RaceTestApp";

void logInfo(const std::string &message);
void logError(const std::string &message, const std::string &stackTrace = "");
void logWarning(const std::string &message, const std::string &stackTrace = "");
void logDebug(const std::string &message, const std::string &stackTrace = "");
void strip(std::string &input);

/**
 * @brief Create a random string of a given length.
 *
 * @param length The length of string to create.
 * @return std::string The random string.
 * @throw If length is greater than 10,000,000 then std::invalid_argument will be thrown.
 */
std::string createRandomString(size_t length);

/**
 * @brief Return a string_view of the given length
 *
 * @param length The length of string to create.
 * @return std::string_view The random string.
 * @throw If length is greater than 10,000,000 then std::invalid_argument will be thrown.
 */
std::string_view getRandomStringFromPool(size_t length);

/**
 * @brief Get the current time in microsends.
 *
 * @return std::int64_t The current time in microsends.
 */
std::int64_t getTimeInMicroseconds();

/**
 * @brief Get an environment variable as a string.
 *
 * @param key The name of the environment variable to get the value of.
 * @return std::string The value of the environment variable, or emptry string if it does not exist.
 */
std::string getEnvironmentVariable(const std::string &key);

class race_persona_unset : public std::exception {
public:
    explicit race_persona_unset(const std::string &message);
    const char *what() const noexcept override;

private:
    std::string message;
};

/**
 * @brief Get the active persona from an environment variable.
 *
 * @return std::string The active persona.
 * @throw rtah::race_persona_unset if the environment variable is not set.
 */
std::string getPersona();

/**
 * @brief Create a ClrMsg with the given parameters. Set the time to the current time and set the
 * nonce to some default value (currently 10).
 *
 * @param msg The message to place in the ClrMsg.
 * @param from The sender to place in the ClrMsg.
 * @param to The recipient to place in the ClrMsg.
 * @return ClrMsg The created ClrMsg.
 */
ClrMsg makeClrMsg(const std::string &msg, const std::string &from, const std::string &to);

/**
 * @brief Tokenize a string into a vector of strings by splitting it on a given delimiter.
 *
 * @param input The input string to tokenize.
 * @param delimiter The delimiter to split the string on.
 * @return std::vector<std::string> A vector of tokens from the input.
 */
std::vector<std::string> tokenizeString(const std::string &input,
                                        const std::string &delimiter = " ");

/**
 * @brief Get a short signature string representing the message contents of the ClrMsg.
 *        This uses a truncated SHA1 hash to minimize accidental collisions while
 *        keeping the signature convinient for manual checking.
 *
 * @param msg the ClrMsg to get a signature of
 * @return string with the signature of the message
 */
std::string getMessageSignature(const ClrMsg &msg);

/**
 * @brief Format and output ClrMsg to the output interface.
 *
 * @param output The output interface to write to.
 * @param message The message to format and write to output.
 */
void outputMessage(IRaceTestAppOutput &output, const ClrMsg &message, bool isReceive = false);

/**
 * @brief Format and output ReceivedMessage to the output interface.
 *
 * @param output The output interface to write to.
 * @param message The message to format and write to output.
 */
void outputMessage(IRaceTestAppOutput &output, const ReceivedMessage &message);

/**
 * @brief Get the test id from a clear message
 *
 * @param msg The clear message to get the test id from
 * @return The test id
 */
std::string testIdFromClrMsg(const ClrMsg &msg);

}  // namespace rtah

#endif
