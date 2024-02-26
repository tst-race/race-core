
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

#ifndef __RACE_PLUGIN_EXPORTS_H_
#define __RACE_PLUGIN_EXPORTS_H_

#include "RaceExport.h"

struct RaceVersionInfo {
    int major, minor, compatibility;
};

constexpr bool operator==(const RaceVersionInfo &a, const RaceVersionInfo &b) {
    return a.major == b.major && a.minor == b.minor && a.compatibility == b.compatibility;
}
constexpr bool operator!=(const RaceVersionInfo &a, const RaceVersionInfo &b) {
    return a.major != b.major || a.minor != b.minor || a.compatibility != b.compatibility;
}
constexpr bool operator<(const RaceVersionInfo &a, const RaceVersionInfo &b) {
    return a.major != b.major ?
               a.major < b.major :
               a.minor != b.minor ? a.minor < b.minor : a.compatibility < b.compatibility;
}
constexpr bool operator<=(const RaceVersionInfo &a, const RaceVersionInfo &b) {
    return a.major != b.major ?
               a.major < b.major :
               a.minor != b.minor ? a.minor < b.minor : a.compatibility <= b.compatibility;
}
constexpr bool operator>(const RaceVersionInfo &a, const RaceVersionInfo &b) {
    return a.major != b.major ?
               a.major > b.major :
               a.minor != b.minor ? a.minor > b.minor : a.compatibility > b.compatibility;
}
constexpr bool operator>=(const RaceVersionInfo &a, const RaceVersionInfo &b) {
    return a.major != b.major ?
               a.major > b.major :
               a.minor != b.minor ? a.minor > b.minor : a.compatibility >= b.compatibility;
}

#define RACE_VERSION (RaceVersionInfo{2, 4, 0})

extern "C" EXPORT const RaceVersionInfo raceVersion;
extern "C" EXPORT const char *const racePluginId;
extern "C" EXPORT const char *const racePluginDescription;

#endif
