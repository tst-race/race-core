
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

#ifndef __COMMS_TWOSIX_COMMON_JSON_TYPES_H__
#define __COMMS_TWOSIX_COMMON_JSON_TYPES_H__

#include <nlohmann/json.hpp>
#include <string>

enum ActionType {
    ACTION_UNDEF,
    ACTION_FETCH,
    ACTION_POST,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ActionType, {
                                             {ACTION_UNDEF, nullptr},
                                             {ACTION_FETCH, "fetch"},
                                             {ACTION_POST, "post"},
                                         });

struct ActionJson {
    std::string linkId;
    ActionType type;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ActionJson, linkId, type);

struct EncodingParamsJson {
    int maxBytes;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EncodingParamsJson, maxBytes);

#endif  // __COMMS_TWOSIX_COMMON_JSON_TYPES_H__