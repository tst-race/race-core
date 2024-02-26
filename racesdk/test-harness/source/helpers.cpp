
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

#include "helpers.h"

#include <algorithm>  // std::min_element

std::vector<std::string> helpers::tokenizeMessage(const std::string &message,
                                                  const std::string &delimiter) {
    std::vector<std::string> result;
    if (message.length() == 0) {
        return result;
    }

    std::string::size_type previousDelimiter = 0;
    std::string::size_type nextDelimiter = message.find(delimiter);

    while (nextDelimiter != std::string::npos) {
        const size_t lengtOfSubstring = nextDelimiter - previousDelimiter;
        result.push_back(message.substr(previousDelimiter, lengtOfSubstring));

        previousDelimiter = nextDelimiter + 1;
        nextDelimiter = message.find(delimiter, previousDelimiter);
    }

    result.push_back(message.substr(previousDelimiter));

    return result;
}

LinkID helpers::getFirstLink(const std::vector<LinkID> &linkIds) {
    const auto iter = std::min_element(linkIds.begin(), linkIds.end());
    if (iter == linkIds.end()) {
        return "";
    }

    return *iter;
}

std::vector<std::string> helpers::split(const std::string &value, const std::string &delimiter) {
    if (delimiter.empty()) {
        return {value};
    }

    std::vector<std::string> fragments;
    auto start = 0u;
    auto end = value.find(delimiter);
    while (end != std::string::npos) {
        fragments.push_back(value.substr(start, end - start));
        start = end + delimiter.length();
        end = value.find(delimiter, start);
    }
    fragments.push_back(value.substr(start, end - start));
    return fragments;
}
