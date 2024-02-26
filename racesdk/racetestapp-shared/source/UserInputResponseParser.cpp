
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

#include "racetestapp/UserInputResponseParser.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "racetestapp/raceTestAppHelpers.h"

using json = nlohmann::json;

UserInputResponseParser::UserInputResponseParser(const std::string &filePath) :
    mFilePath(filePath) {}

auto UserInputResponseParser::getResponse(const std::string &pluginId, const std::string &prompt)
    -> UserResponse {
    rtah::logDebug("UserInputResponseParser: " + mFilePath);
    std::ifstream fileStream(mFilePath);
    return getResponse(fileStream, pluginId, prompt);
}

auto UserInputResponseParser::getResponse(std::istream &input, const std::string &pluginId,
                                          const std::string &prompt) -> UserResponse {
    if (not input.good() or not input) {
        throw parsing_exception("User input response file stream in bad state");
    }

    json responseJson;
    try {
        input >> responseJson;
    } catch (std::exception &error) {
        throw parsing_exception("User input response file failed to parse JSON: " +
                                std::string(error.what()));
    } catch (...) {
        throw parsing_exception(
            "User input response file failed to parse JSON. Verify that the file contains valid "
            "json.");
    }

    if (not responseJson.contains(pluginId)) {
        throw parsing_exception("No response group found for plugin ID: " + pluginId);
    }
    json pluginResponses = responseJson[pluginId];

    if (not pluginResponses.contains(prompt)) {
        throw parsing_exception("No response found for prompt: " + prompt);
    }

    UserResponse userResponse;

    json userResponseJson = pluginResponses[prompt];
    if (userResponseJson.is_string()) {
        userResponse.answered = true;
        userResponse.response = userResponseJson.get<std::string>();
    } else if (userResponseJson.is_object()) {
        try {
            if (userResponseJson.contains("answered")) {
                userResponse.answered = userResponseJson["answered"].get<bool>();
            } else {
                userResponse.answered = true;
            }

            if (userResponseJson.contains("delayMs")) {
                userResponse.delay_ms = userResponseJson["delayMs"].get<int32_t>();
            }

            if (userResponseJson.contains("response")) {
                userResponse.response = userResponseJson["response"].get<std::string>();
            }
        } catch (std::exception &error) {
            throw parsing_exception("Invalid response value for prompt: " +
                                    std::string(error.what()));
        }
    } else {
        throw parsing_exception("Invalid response value for prompt: " + prompt);
    }

    return userResponse;
}