
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

#ifndef _LOADER_SDK_C_WRAPPER_H__
#define _LOADER_SDK_C_WRAPPER_H__

#ifdef __cplusplus
extern "C" {
#endif

void race_log_debug(const char *pluginName, const char *message, const char *stackTrace);
void race_log_info(const char *pluginName, const char *message, const char *stackTrace);
void race_log_warning(const char *pluginName, const char *message, const char *stackTrace);
void race_log_error(const char *pluginName, const char *message, const char *stackTrace);

#ifdef __cplusplus
}
#endif

#endif
