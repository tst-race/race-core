
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

#include "sdk_c_wrapper.h"

#include <IRaceSdkComms.h>
#include <RaceLog.h>

#include <cstdint>
#include <cstring>  // memcpy strdup
#include <iostream>

#include "helper.h"
#include "plugin_extern_c.h"

extern "C" void sdk_get_entropy(void *sdk, void *buffer, uint32_t numBytes) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_get_entropy is NULL!", "");
        return;
    }
    if (buffer == nullptr) {
        RaceLog::logError("C Shim", "buffer passed to sdk_get_entropy is NULL!", "");
        return;
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    std::memcpy(buffer, actualSdk->getEntropy(numBytes).data(), numBytes);
}

extern "C" char *sdk_get_active_persona(void *sdk) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_get_active_persona is NULL!", "");
        return nullptr;
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    const auto persona = actualSdk->getActivePersona();
    // Memory for the new string generated by strdup is obtained with malloc, and can be freed with
    // free.
    return strdup(persona.c_str());
}

extern "C" SdkResponseC sdk_async_error(void *sdk, uint64_t handle, PluginResponse status) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_async_error is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    return actualSdk->asyncError(handle, status);
}

extern "C" ChannelPropertiesC sdk_get_channel_properties(void *sdk, const char *channelGid) {
    ChannelProperties props;
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_get_channel_properties is NULL!", "");
    } else {
        const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
        props = actualSdk->getChannelProperties(channelGid);
    }

    ChannelPropertiesC propsC;
    helper::convertChannelPropertiesToChannelPropertiesC(props, propsC);
    return propsC;
}

extern "C" SdkResponseC sdk_on_package_status_changed(void *sdk, uint64_t handle,
                                                      PackageStatus status, int32_t timeout) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_on_package_status_changed is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    return actualSdk->onPackageStatusChanged(handle, status, timeout);
}

extern "C" SdkResponseC sdk_on_connection_status_changed(void *sdk, uint64_t handle,
                                                         const char *connId,
                                                         ConnectionStatus status,
                                                         const struct LinkPropertiesC *propertiesC,
                                                         int32_t timeout) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_on_connection_status_changed is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (propertiesC == nullptr) {
        RaceLog::logError("C Shim", "props passed to sdk_on_connection_status_changed is NULL!",
                          "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);

    LinkProperties linkProps;
    helper::convertLinkPropertiesCToClass(*propertiesC, linkProps);
    return actualSdk->onConnectionStatusChanged(handle, connId, status, linkProps, timeout);
}

extern "C" SdkResponseC sdk_on_channel_status_changed(void *sdk, uint64_t handle,
                                                      const char *channelGid, ChannelStatus status,
                                                      ChannelPropertiesC *props, int32_t timeout) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_on_channel_status_changed is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (channelGid == nullptr) {
        RaceLog::logError("C Shim", "channelGid passed to sdk_on_channel_status_changed is NULL!",
                          "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (props == nullptr) {
        RaceLog::logError("C Shim", "props passed to sdk_on_channel_status_changed is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    ChannelProperties channelProps;
    helper::convertChannelPropertiesCToClass(*props, channelProps);
    return actualSdk->onChannelStatusChanged(handle, channelGid, status, channelProps, timeout);
}

extern "C" SdkResponseC sdk_on_link_status_changed(void *sdk, uint64_t handle, const char *linkId,
                                                   LinkStatus status, LinkPropertiesC *props,
                                                   int32_t timeout) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_on_link_status_changed is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (linkId == nullptr) {
        RaceLog::logError("C Shim", "linkId passed to sdk_on_link_status_changed is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (props == nullptr) {
        RaceLog::logError("C Shim", "props passed to sdk_on_link_status_changed is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    LinkProperties linkProps;
    helper::convertLinkPropertiesCToClass(*props, linkProps);
    return actualSdk->onLinkStatusChanged(handle, linkId, status, linkProps, timeout);
}

extern "C" SdkResponseC sdk_update_link_properties(void *sdk, const char *linkId,
                                                   const struct LinkPropertiesC *props,
                                                   int32_t timeout) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_update_link_properties is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (props == nullptr) {
        RaceLog::logError("C Shim", "props passed to sdk_update_link_properties is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);

    LinkProperties linkProps;
    helper::convertLinkPropertiesCToClass(*props, linkProps);
    return actualSdk->updateLinkProperties(linkId, linkProps, timeout);
}

extern "C" SdkResponseC sdk_receive_enc_pkg(void *sdk, const void *cipherText,
                                            size_t cipherTextSize, const char **connIDs,
                                            int32_t timeout) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_receive_enc_pkg is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (cipherText == nullptr) {
        RaceLog::logError("C Shim", "cipherText passed to sdk_receive_enc_pkg are NULL", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (connIDs == nullptr || connIDs[0] == nullptr) {
        RaceLog::logError("C Shim", "connIDs passed to sdk_receive_enc_pkg are NULL", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);

    // Copy the C string into an EncPkg object.
    const std::uint8_t *ctext = static_cast<const std::uint8_t *>(cipherText);
    EncPkg encPkg({ctext, ctext + cipherTextSize});

    std::vector<std::string> connectionsIdsVector;
    int connIdPointerIndex = 0;
    auto connIdPointer = connIDs[connIdPointerIndex];
    while (connIdPointer != nullptr) {
        connectionsIdsVector.emplace_back(connIdPointer);
        connIdPointer = connIDs[++connIdPointerIndex];
    }

    return actualSdk->receiveEncPkg(encPkg, connectionsIdsVector, timeout);
}

extern "C" char *sdk_generate_connection_id(void *sdk, const char *linkId) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_generate_connection_id is NULL!", "");
        return nullptr;
    }
    if (linkId == nullptr) {
        RaceLog::logError("C Shim", "linkId passed to sdk_generate_connection_id is NULL!", "");
        return nullptr;
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    const auto connId = actualSdk->generateConnectionId(linkId);
    // Memory for the new string generated by strdup is obtained with malloc, and can be freed with
    // free.
    return strdup(connId.c_str());
}

extern "C" SdkResponseC sdk_make_dir(void *sdk, const char *filename) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_make_dir is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (filename == nullptr) {
        RaceLog::logError("C Shim", "filename passed to sdk_make_dir is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    return actualSdk->makeDir(filename);
}

extern "C" SdkResponseC sdk_remove_dir(void *sdk, const char *filename) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_remove_dir is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (filename == nullptr) {
        RaceLog::logError("C Shim", "filename passed to sdk_remove_dir is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    return actualSdk->removeDir(filename);
}

extern "C" char **sdk_list_dir(void *sdk, const char *filename, size_t *vectorLength) {
    *vectorLength = 0;
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_list_dir is NULL!", "");
        return nullptr;
    }
    if (filename == nullptr) {
        RaceLog::logError("C Shim", "filename passed to sdk_list_dir is NULL!", "");
        return nullptr;
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    const std::vector<std::string> contents = actualSdk->listDir(filename);

    if (contents.size() == 0) {
        return nullptr;
    }

    *vectorLength = contents.size();
    char **listedDirs = new char *[contents.size()];
    for (size_t index = 0; index < contents.size(); ++index) {
        // Add one for the trailing null character.
        const auto cStringSize = contents[index].size() + 1;
        listedDirs[index] = new char[cStringSize];
        memcpy(listedDirs[index], contents[index].c_str(), cStringSize);
    }

    return listedDirs;
}

extern "C" std::uint8_t *sdk_read_file(void *sdk, const char *filename, size_t *dataLength) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_read_file is NULL!", "");
        *dataLength = 0;
        return nullptr;
    }
    if (filename == nullptr) {
        RaceLog::logError("C Shim", "filename passed to sdk_read_file is NULL!", "");
        *dataLength = 0;
        return nullptr;
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    const auto data = actualSdk->readFile(filename);
    // RaceLog::logDebug("data returned: " + std::to_string(data));
    const auto buffer = new std::uint8_t[data.size()];
    memcpy(buffer, data.data(), data.size());
    *dataLength = data.size();
    return buffer;
}

extern "C" SdkResponseC sdk_write_file(void *sdk, const char *filename, const std::uint8_t *data,
                                       size_t dataLength) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_write_file is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (filename == nullptr) {
        RaceLog::logError("C Shim", "filename passed to sdk_write_file is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (data == nullptr) {
        RaceLog::logError("C Shim", "data passed to sdk_write_file is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    std::vector<std::uint8_t> buffer({data, data + dataLength});
    return actualSdk->writeFile(filename, buffer);
}

extern "C" SdkResponseC sdk_append_file(void *sdk, const char *filename, const uint8_t *data,
                                        size_t dataLength) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_write_file is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (filename == nullptr) {
        RaceLog::logError("C Shim", "filename passed to sdk_write_file is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (data == nullptr) {
        RaceLog::logError("C Shim", "data passed to sdk_write_file is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    const std::uint8_t *casted = static_cast<const std::uint8_t *>(data);
    std::vector<std::uint8_t> buffer({casted, casted + dataLength});
    return actualSdk->appendFile(filename, buffer);
}

extern "C" char *sdk_generate_link_id(void *sdk, const char *channelGid) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_generate_link_id is NULL!", "");
        return nullptr;
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    const auto linkId = actualSdk->generateLinkId(channelGid);
    // Memory for the new string generated by strdup is obtained with malloc, and can be freed with
    // free.
    return strdup(linkId.c_str());
}

extern "C" SdkResponseC sdk_request_plugin_user_input(void *sdk, const char *key,
                                                      const char *prompt, bool cache) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_request_plugin_user_input is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (key == nullptr) {
        RaceLog::logError("C Shim", "key passed to sdk_request_plugin_user_input is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (prompt == nullptr) {
        RaceLog::logError("C Shim", "prompt passed to sdk_request_plugin_user_input is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    return actualSdk->requestPluginUserInput(key, prompt, cache);
}

extern "C" SdkResponseC sdk_request_common_user_input(void *sdk, const char *key) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_request_common_user_input is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (key == nullptr) {
        RaceLog::logError("C Shim", "key passed to sdk_request_common_user_input is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    return actualSdk->requestCommonUserInput(key);
}

extern "C" SdkResponseC sdk_display_info_to_user(void *sdk, const char *data,
                                                 RaceEnums::UserDisplayType displayType) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_display_info_to_user is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (data == nullptr) {
        RaceLog::logError("C Shim", "data passed to sdk_display_info_to_user is NULL!", "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    return actualSdk->displayInfoToUser(data, displayType);
}

extern "C" SdkResponseC sdk_display_bootstrap_info_to_user(
    void *sdk, const char *data, RaceEnums::UserDisplayType displayType,
    RaceEnums::BootstrapActionType actionType) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_display_bootstrap_info_to_user is NULL!",
                          "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (data == nullptr) {
        RaceLog::logError("C Shim", "data passed to sdk_display_bootstrap_info_to_user is NULL!",
                          "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    return actualSdk->displayBootstrapInfoToUser(data, displayType, actionType);
}

SdkResponseC sdk_unblock_queue(void *sdk, const char *connId) {
    if (sdk == nullptr) {
        RaceLog::logError("C Shim", "sdk passed to sdk_display_bootstrap_info_to_user is NULL!",
                          "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }
    if (connId == nullptr) {
        RaceLog::logError("C Shim", "connId passed to sdk_display_bootstrap_info_to_user is NULL!",
                          "");
        return SdkResponse(SDK_INVALID_ARGUMENT);
    }

    const auto actualSdk = static_cast<IRaceSdkComms *>(sdk);
    return actualSdk->unblockQueue(connId);
}

extern "C" void sdk_release_string(char *cstring) {
    free(cstring);
}

extern "C" void sdk_delete_string_array(char **string_array, size_t array_length) {
    if (string_array != nullptr) {
        for (size_t index = 0; index < array_length; ++index) {
            delete[] string_array[index];
        }
        delete[] string_array;
    }
}

extern "C" void sdk_release_buffer(uint8_t *buffer) {
    delete[] buffer;
}