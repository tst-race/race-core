
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

#ifndef __SDK_RESPONSE_H_
#define __SDK_RESPONSE_H_

#include <cstdint>
#include <string>

extern "C" {

typedef uint64_t RaceHandle;
const RaceHandle NULL_RACE_HANDLE = 0;

enum SdkStatus {
    SDK_INVALID = 0,           // default / undefined status
    SDK_OK = 1,                // OK
    SDK_SHUTTING_DOWN = 2,     //  shutting down
    SDK_PLUGIN_MISSING = 3,    // plugin missing
    SDK_INVALID_ARGUMENT = 4,  // invalid argument
    SDK_QUEUE_FULL = 5         // plugin / connection queue is full
};

struct SdkResponseC {
    SdkStatus status;
    double queueUtilization;  // proportion of queue current in use
    RaceHandle handle;
};
}

class SdkResponse : public SdkResponseC {
public:
    SdkResponse();
    // cppcheck-suppress noExplicitConstructor
    SdkResponse(SdkStatus _status);
    SdkResponse(SdkStatus _status, double _queueUtilization, RaceHandle _handle);
};

/**
 * @brief Convert a SdkStatus value to a human readable string. This function is strictly for
 * logging and debugging. The output formatting may change without notice. Do NOT use this for any
 * logical comparisons, etc. The functionality of your plugin should in no way rely on the output of
 * this function.
 *
 * @param sdkStatus The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string sdkStatusToString(SdkStatus sdkStatus);
std::ostream &operator<<(std::ostream &out, SdkStatus sdkStatus);

#endif
