
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

#include <stdint.h>

#include <string>
#include <vector>

#include "ChannelPropertiesC.h"
#include "ChannelStatus.h"
#include "ConnectionStatus.h"
#include "LinkPropertiesC.h"
#include "LinkStatus.h"
#include "PackageStatus.h"
#include "PluginResponse.h"
#include "RaceEnums.h"
#include "SdkResponse.h"

/**
 * The following functions are c wrappers for the sdk function from
 * racesdk/common/include/IRaceSdkComms.h
 *
 * They should not be used directly. Rather they are used only be used indirectly through the rust
 * shims
 */
extern "C" {

void sdk_get_entropy(void *sdk, void *buffer, uint32_t numBytes);

char *sdk_get_active_persona(void *sdk);

SdkResponseC sdk_async_error(void *sdk, uint64_t handle, PluginResponse status);

ChannelPropertiesC sdk_get_channel_properties(void *sdk, const char *channelGid);

SdkResponseC sdk_on_package_status_changed(void *sdk, uint64_t handle, PackageStatus status,
                                           int32_t timeout);

SdkResponseC sdk_on_connection_status_changed(void *sdk, uint64_t handle, const char *connId,
                                              ConnectionStatus status,
                                              const struct LinkPropertiesC *props, int32_t timeout);

SdkResponseC sdk_on_channel_status_changed(void *sdk, uint64_t handle, const char *channelGid,
                                           ChannelStatus status, ChannelPropertiesC *props,
                                           int32_t timeout);

SdkResponseC sdk_on_link_status_changed(void *sdk, uint64_t handle, const char *linkId,
                                        LinkStatus status, LinkPropertiesC *props, int32_t timeout);

SdkResponseC sdk_update_link_properties(void *sdk, const char *linkId,
                                        const struct LinkPropertiesC *props, int32_t timeout);

SdkResponseC sdk_receive_enc_pkg(void *sdk, const void *cipherText, size_t cipherTextSize,
                                 const char **connIDs, int32_t timeout);

char *sdk_generate_connection_id(void *sdk, const char *linkId);

char *sdk_generate_link_id(void *sdk, const char *channelGid);

SdkResponseC sdk_make_dir(void *sdk, const char *dirpath);

SdkResponseC sdk_remove_dir(void *sdk, const char *dirpath);

char **sdk_list_dir(void *sdk, const char *dirpath, size_t *vectorLength);

uint8_t *sdk_read_file(void *sdk, const char *filename, size_t *data_length);

SdkResponseC sdk_write_file(void *sdk, const char *filename, const uint8_t *data,
                            size_t dataLength);

SdkResponseC sdk_append_file(void *sdk, const char *filename, const uint8_t *data,
                             size_t dataLength);

SdkResponseC sdk_request_plugin_user_input(void *sdk, const char *key, const char *prompt,
                                           bool cache);

SdkResponseC sdk_request_common_user_input(void *sdk, const char *key);

SdkResponseC sdk_display_info_to_user(void *sdk, const char *data,
                                      RaceEnums::UserDisplayType displayType);

SdkResponseC sdk_display_bootstrap_info_to_user(void *sdk, const char *data,
                                                RaceEnums::UserDisplayType displayType,
                                                RaceEnums::BootstrapActionType actionType);

SdkResponseC sdk_unblock_queue(void *sdk, const char *connId);

/**
 * @brief release memory allocated by the c wrappers sdk_get_active_persona,
 * sdk_generate_connection_id, or sdk_generate_link_id. These are also used by
 * ChannelPropertiesC functions (in rust) in addition to the sdk functions
 */

void sdk_release_string(char *cstring);
void sdk_delete_string_array(char **string_array, size_t array_length);
void sdk_release_buffer(uint8_t *buffer);
}

#endif
