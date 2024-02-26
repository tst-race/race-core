
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

#ifndef __RACE_EXCEPTIONS_H_
#define __RACE_EXCEPTIONS_H_

/**
 * @brief Parsing exception base class. This class should not be used directly. Use one of its
 * child classes instead.
 *
 */
class parsing_exception : public std::exception {
public:
    explicit parsing_exception(const std::string &_message) : message(_message) {}

    const char *what() const noexcept override {
        return message.c_str();
    }

private:
    std::string message;
};

/**
 * @brief Parsing error exception. If this exception is thrown then a critical error has
 * occurred and the program should exit.
 *
 */
class parsing_error : public parsing_exception {
public:
    explicit parsing_error(const std::string &_message) : parsing_exception(_message) {}
};

#endif