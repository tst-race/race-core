
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

#ifndef __JSON_CONFIG_H_
#define __JSON_CONFIG_H_

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "helper.h"

class JsonConfig {
public:
    explicit JsonConfig();
    explicit JsonConfig(const std::string &configPath);

    class race_config_parsing_exception : public std::exception {
    public:
        explicit race_config_parsing_exception(const std::string &msg_) : msg(msg_) {}
        const char *what() const noexcept override {
            return msg.c_str();
        }

    private:
        std::string msg;
    };

protected:
    // The parsed JSON content
    nlohmann::json configJson;

    /**
     * @brief Parse a JSON structure from the given file
     *
     */
    void initializeFromConfig(const std::string &configPath);

    /**
     * @brief Dump a string representation of the JSON structure
     * within the given file
     *
     */
    std::string readConfigFile(const std::string &configPath);

    /**
     * @brief Parse a JSON structure from the given string
     *
     */
    void parseConfigString(const std::string &config);

    /**
     * @brief Convenience routine to convert a string to a boolean
     *
     */
    bool to_bool(std::string str);
};

#endif
