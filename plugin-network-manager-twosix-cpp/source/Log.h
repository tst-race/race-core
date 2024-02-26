
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

#ifndef __NETWORK_MANAGER_TWOSIX_LOG_H__
#define __NETWORK_MANAGER_TWOSIX_LOG_H__

#include <Defer.h>
#include <RaceLog.h>

#include <sstream>
#include <string>

void logDebug(const std::string &message);
void logInfo(const std::string &message);
void logWarning(const std::string &message);
void logError(const std::string &message);

void logMessage(const std::string &prefix, const std::string &message);

#define TRACE_FUNCTION(...) TRACE_FUNCTION_BASE(PluginNMTwoSixCpp, ##__VA_ARGS__)
#define TRACE_METHOD(...) TRACE_METHOD_BASE(PluginNMTwoSixCpp, ##__VA_ARGS__)

#endif  // __NETWORK_MANAGER_TWOSIX_LOG_H__
