
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

#include "racetestapp/raceTestAppHelpers.h"

#include <RaceLog.h>  // RaceLog::logInfo, RaceLog::logError
#include <openssl/err.h>
#include <openssl/evp.h>

#include <algorithm>     // std::find_if
#include <chrono>        // std::chrono::system_clock
#include <iomanip>       // std::setfill, std::setw
#include <random>        // std::default_random_engine, std::random_device
#include <sstream>       // std::stringstream
#include <stdexcept>     // std::invalid_argument
#include <system_error>  // std::system_error

static inline std::string convertToHexString(size_t input, size_t paddedLength = 0) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(paddedLength) << std::hex << input;
    return stream.str();
}

void rtah::logInfo(const std::string &message) {
    RaceLog::logInfo(rtah::appNameForLogging, message, "");
}

void rtah::logError(const std::string &message, const std::string &stackTrace) {
    RaceLog::logError(rtah::appNameForLogging, message, stackTrace);
}

void rtah::logWarning(const std::string &message, const std::string &stackTrace) {
    RaceLog::logWarning(rtah::appNameForLogging, message, stackTrace);
}

void rtah::logDebug(const std::string &message, const std::string &stackTrace) {
    RaceLog::logDebug(rtah::appNameForLogging, message, stackTrace);
}

static void stripLeading(std::string &input) {
    const auto pred = [](int character) { return !std::isspace(character); };
    input.erase(input.begin(), std::find_if(input.begin(), input.end(), pred));
}

static void stripTrailing(std::string &input) {
    const auto pred = [](int character) { return !std::isspace(character); };
    input.erase(std::find_if(input.rbegin(), input.rend(), pred).base(), input.end());
}

void rtah::strip(std::string &input) {
    stripLeading(input);
    stripTrailing(input);
}

std::string rtah::createRandomString(size_t length) {
    static const size_t lengthLimit = 10000000;
    if (length > lengthLimit) {
        throw std::invalid_argument("can not create strings larger than " +
                                    std::to_string(lengthLimit) + " bytes");
    }
    std::string message;
    message.reserve(length);
    // TODO: use ascii characters 9, 32-126
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::default_random_engine rand{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> dist(0, sizeof(alphanum) - 2);
    for (size_t i = 0; i < length; ++i) {
        message.push_back(alphanum[dist(rand)]);
    }
    return message;
}

std::string_view rtah::getRandomStringFromPool(size_t length) {
    static const size_t lengthLimit = 10000000;
    static const std::string pool = createRandomString(lengthLimit);
    thread_local std::default_random_engine rand{std::random_device{}()};
    if (length > lengthLimit) {
        throw std::invalid_argument("can not create strings larger than " +
                                    std::to_string(lengthLimit) + " bytes");
    }

    std::uniform_int_distribution<std::size_t> dist(0, lengthLimit - length);
    size_t begin_index = dist(rand);
    return std::string_view(pool.data() + begin_index, length);
}

std::int64_t rtah::getTimeInMicroseconds() {
    using namespace std::chrono;
    // C++20 adds utc_clock, which would be better to use here
    auto time = system_clock::now();
    auto micro = duration_cast<microseconds>(time.time_since_epoch());
    return micro.count();
}

std::string rtah::getEnvironmentVariable(const std::string &key) {
    const char *envVarValue = getenv(key.c_str());
    if (envVarValue == nullptr) {
        return "";
    }

    return std::string(envVarValue);
}

rtah::race_persona_unset::race_persona_unset(const std::string &_message) : message(_message) {}

const char *rtah::race_persona_unset::what() const noexcept {
    return message.c_str();
}

std::string rtah::getPersona() {
    const std::string personaEnvVarKey = "RACE_PERSONA";
    const std::string persona = getEnvironmentVariable(personaEnvVarKey);
    if (persona.length() == 0) {
        throw race_persona_unset(std::string(
            "Failed to get persona. Please set the persona in an environment variable named: \"" +
            personaEnvVarKey + "\""));
    }
    return persona;
}

ClrMsg rtah::makeClrMsg(const std::string &msg, const std::string &from, const std::string &to) {
    constexpr std::int32_t INITIAL_TTL = 10;
    return ClrMsg(msg, from, to, rtah::getTimeInMicroseconds(), INITIAL_TTL);
}

std::vector<std::string> rtah::tokenizeString(const std::string &input,
                                              const std::string &delimiter) {
    std::vector<std::string> result;
    if (input.length() == 0) {
        return result;
    }

    std::string::size_type previousDelimiter = 0;
    std::string::size_type nextDelimiter = input.find(delimiter);

    while (nextDelimiter != std::string::npos) {
        const size_t lengtOfSubstring = nextDelimiter - previousDelimiter;
        result.push_back(input.substr(previousDelimiter, lengtOfSubstring));

        previousDelimiter = nextDelimiter + delimiter.length();
        nextDelimiter = input.find(delimiter, previousDelimiter);
    }

    result.push_back(input.substr(previousDelimiter));

    return result;
}

static void handleOpensslError() {
    unsigned long err;
    while ((err = ERR_get_error()) != 0) {
        rtah::logError(std::string("Error with OpenSSL call: ") + ERR_error_string(err, NULL));
    }
    throw std::logic_error("Error with OpenSSL call");
}

std::string rtah::getMessageSignature(const ClrMsg &msg) {
    constexpr int SIGNATURE_SIZE = 20;
    const EVP_MD *md = EVP_sha1();
    std::uint8_t m[SIGNATURE_SIZE];
    int res;

    // Verify hash size to ensure no writes ococur outside of MsgSignature's memory
    if (std::size_t(EVP_MD_size(md)) != SIGNATURE_SIZE) {
        throw std::logic_error(
            "Unexpected size for hash. Expected: " + std::to_string(SIGNATURE_SIZE) +
            ", Received: " + std::to_string(EVP_MD_size(md)));
    }

    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (ctx == nullptr)
        handleOpensslError();

    res = EVP_DigestInit_ex(ctx.get(), md, NULL);
    if (res != 1)
        handleOpensslError();

    const std::string &message = msg.getMsg();
    res = EVP_DigestUpdate(ctx.get(), message.data(), message.size());
    if (res != 1)
        handleOpensslError();

    const std::string &from = msg.getFrom();
    res = EVP_DigestUpdate(ctx.get(), from.data(), from.size());
    if (res != 1)
        handleOpensslError();

    const std::string &to = msg.getTo();
    res = EVP_DigestUpdate(ctx.get(), to.data(), to.size());
    if (res != 1)
        handleOpensslError();

    auto sentTime = msg.getTime();
    res = EVP_DigestUpdate(ctx.get(), &sentTime, sizeof(sentTime));
    if (res != 1)
        handleOpensslError();

    res = EVP_DigestFinal_ex(ctx.get(), m, NULL);
    if (res != 1)
        handleOpensslError();

    // Log hash for debugging
    std::stringstream s;
    s << std::hex << std::setfill('0');
    for (int i = 0; i < SIGNATURE_SIZE; ++i) {
        s << std::setw(2) << static_cast<int>(m[i]);
    }

    return s.str();
}

void rtah::outputMessage(IRaceTestAppOutput &output, const ClrMsg &message, bool isReceive) {
    // Clip messages that are greater than the size limit.
    static constexpr std::uint32_t outputMessageSizeLimit = 256;
    std::string msgText = message.getMsg();
    std::size_t length = msgText.length();
    if (msgText.size() > outputMessageSizeLimit) {
        msgText.resize(outputMessageSizeLimit - 3);
        msgText += "...";
    }

    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%F %T");
    std::string timeNow = ss.str();

    std::string messageAction = "";
    std::string messageRcvTime = "";
    if (isReceive) {
        const ReceivedMessage receivedMessage = static_cast<const ReceivedMessage &>(message);
        messageAction = "Received";
        messageRcvTime = ", recv-time: " + std::to_string(receivedMessage.receivedTime);
    } else {
        messageAction = "Sending";
    }

    // clang-format off
    std::string messageToLog = timeNow + ": INFO: " +
                       messageAction + " message: " +
                       "checksum: " + rtah::getMessageSignature(message) + ", " +
                       "size: " + std::to_string(length) + ", " +
                       "nonce: " + std::to_string(message.getNonce()) + ", " +
                       "from: " + message.getFrom() + ", " +
                       "to: " + message.getTo() + ", " +
                       "test-id: " + testIdFromClrMsg(message) + ", " +
                       "sent-time: " + std::to_string(message.getTime()) + ", " +
                       "traceid: " + convertToHexString(message.getTraceId()) +
                       messageRcvTime + ", " +
                       "message: " + msgText;
    // clang-format on
    output.writeOutput(messageToLog);
}

void rtah::outputMessage(IRaceTestAppOutput &output, const ReceivedMessage &message) {
    rtah::outputMessage(output, static_cast<const ClrMsg &>(message), true);
}

std::string rtah::testIdFromClrMsg(const ClrMsg &msg) {
    std::string message = msg.getMsg();
    size_t pos = message.find(" ");
    if (pos != std::string::npos) {
        return std::string(message.c_str(), pos);
    }

    return "";
}