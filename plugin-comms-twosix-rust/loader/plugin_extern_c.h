
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

#ifndef _LOADER_PLUGIN_EXTERN_C_H__
#define _LOADER_PLUGIN_EXTERN_C_H__

#include <IRacePluginComms.h>
#include <IRaceSdkComms.h>
#include <string.h>  // strncpy

#include <cstdint>
#include <iostream>

#include "LinkPropertiesC.h"

/**
 * The following functions are c compatible wrappers for the plugin methods from
 * racesdk/common/include/IRacePluginComms.h
 *
 * They should not be used or implemented directly. Rather they are used only be used indirectly
 * through the PluginCommsRustCppWrapper and implemted through the wrapper in pluginwrapper
 */
extern "C" {
void *create_plugin(void *sdk);

void destroy_plugin(void *plugin);

void plugin_init(void *plugin, PluginResponse *response, const char *etc_directory,
                 const char *logging_directory, const char *aux_data_directory,
                 const char *tmp_directory, const char *plugin_directory);

void plugin_shutdown(void *plugin, PluginResponse *response);

void plugin_send_package(void *plugin, PluginResponse *response, uint64_t handle,
                         const char *connectionId, const void *cipherText, size_t cipherTextSize,
                         double timeoutTimestamp, uint64_t batch_id);

void plugin_open_connection(void *plugin, PluginResponse *response, uint64_t handle,
                            std::int32_t linkType, const char *linkId, const char *linkHints,
                            int32_t sendTimeout);

void plugin_destroy_link(void *plugin, PluginResponse *response, uint64_t handle,
                         const char *linkId);

void plugin_create_link(void *plugin, PluginResponse *response, uint64_t handle,
                        const char *channelGid);

void plugin_create_link_from_address(void *plugin, PluginResponse *response, uint64_t handle,
                                     const char *channelGid, const char *linkAddress);

void plugin_load_link_address(void *plugin, PluginResponse *response, uint64_t handle,
                              const char *channelGid, const char *linkAddress);

void plugin_load_link_addresses(void *plugin, PluginResponse *response, uint64_t handle,
                                const char *channelGid, const char **linkAddresses,
                                size_t linkAddressesSize);

void plugin_activate_channel(void *plugin, PluginResponse *response, uint64_t handle,
                             const char *channelGid, const char *roleName);

void plugin_deactivate_channel(void *plugin, PluginResponse *response, uint64_t handle,
                               const char *channelGid);

void plugin_close_connection(void *plugin, PluginResponse *response, uint64_t handle,
                             const char *connectionId);

void plugin_on_user_input_received(void *plugin, PluginResponse *response, uint64_t handle,
                                   bool answered, const char *userResponse);

void plugin_flush_channel(void *plugin, PluginResponse *response, uint64_t handle,
                          const char *channelGid, uint64_t batchId);

void plugin_on_user_acknowledgment_received(void *plugin, PluginResponse *response,
                                            uint64_t handle);
}

#endif
