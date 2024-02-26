
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

#ifndef __JSON_IO_H_
#define __JSON_IO_H_

#include <IRaceSdkNM.h>

#include <nlohmann/json.hpp>

#include "Log.h"

namespace JsonIO {

inline nlohmann::json loadJson(IRaceSdkNM &sdk, const std::string &path) {
    TRACE_FUNCTION(path);
    try {
        auto bytes = sdk.readFile(path);
        return nlohmann::json::parse(bytes.begin(), bytes.end());
    } catch (nlohmann::json::parse_error &e) {
        logError("Failed to parse json from " + path + ": " + e.what());
        return {};
    }
}

static const int jsonIndentLevel = 4;

inline bool writeJson(IRaceSdkNM &sdk, const std::string &path, nlohmann::json json) {
    TRACE_FUNCTION(path);
    std::string str = json.dump(jsonIndentLevel);
    SdkResponse response = sdk.writeFile(path, {str.begin(), str.end()});
    if (response.status != SDK_OK) {
        logError("Failed to write json to " + path);
        return false;
    }
    return true;
}

}  // namespace JsonIO

#endif
