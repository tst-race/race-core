
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

#include "SdkResponse.h"

SdkResponse::SdkResponse() : SdkResponse(SDK_INVALID, 0.0, NULL_RACE_HANDLE) {}
SdkResponse::SdkResponse(SdkStatus _status) : SdkResponse(_status, 0.0, NULL_RACE_HANDLE) {}
SdkResponse::SdkResponse(SdkStatus _status, double _queueUtilization, RaceHandle _handle) {
    // can't use initializers because these are in the c-compatible base class which does not have a
    // constructor
    status = _status;
    queueUtilization = _queueUtilization;
    handle = _handle;
}

std::string sdkStatusToString(SdkStatus sdkStatus) {
    switch (sdkStatus) {
        case SDK_INVALID:
            return "SDK_INVALID";
        case SDK_OK:
            return "SDK_OK";
        case SDK_SHUTTING_DOWN:
            return "SDK_SHUTTING_DOWN";
        case SDK_PLUGIN_MISSING:
            return "SDK_PLUGIN_MISSING";
        case SDK_INVALID_ARGUMENT:
            return "SDK_INVALID_ARGUMENT";
        case SDK_QUEUE_FULL:
            return "SDK_QUEUE_FULL";
        default:
            return "ERROR: INVALID SDK STATUS: " + std::to_string(sdkStatus);
    }
}

std::ostream &operator<<(std::ostream &out, SdkStatus sdkStatus) {
    return out << sdkStatusToString(sdkStatus);
}
