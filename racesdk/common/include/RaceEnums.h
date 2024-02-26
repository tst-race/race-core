
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

#ifndef __RACE_ENUMS_H_
#define __RACE_ENUMS_H_

#include <string>

namespace RaceEnums {

/**
 * @brief The type of node the plugin supports.
 *
 */
enum NodeType { NT_ALL = 0, NT_CLIENT = 1, NT_SERVER = 2, NT_UNDEF = 3 };

/**
 * @brief Convert a NodeType value to a human readable string. This function is strictly for
 * logging and debugging. The output formatting may change without notice. Do NOT use this for
 * any logical comparisons, etc. The functionality of your plugin should in no way rely on the
 * output of this function.
 *
 * @param nodeType The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string nodeTypeToString(NodeType nodeType);
std::ostream &operator<<(std::ostream &out, NodeType nodeType);

/**
 * @brief Convert a node type string to a node type enum. If the plugin node type string is invalid
 * then it will be set to undef.
 *
 * @param nodeTypeString The node type as a string, either "any", "server" or "client".
 * @return NodeType The corresponding node type enum.
 */
NodeType stringToNodeType(const std::string &nodeTypeString);

/**
 * @brief The type of plugin, i.e. network manager or comms.
 *
 */
enum PluginType { PT_NM, PT_COMMS, PT_ARTIFACT_MANAGER };
/**
 * @brief Convert a PluginType value to a human readable string. This function is strictly for
 * logging and debugging. The output formatting may change without notice. Do NOT use this for
 * any logical comparisons, etc. The functionality of your plugin should in no way rely on the
 * output of this function.
 *
 * @param pluginType The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string pluginTypeToString(PluginType pluginType);
std::ostream &operator<<(std::ostream &out, PluginType pluginType);

/**
 * @brief Convert a plugin type string to a plugin type enum. If the plugin type string is
 * invalid then an exception will be thrown.
 *
 * @param pluginTypeString The plugin type as a string, either "network-manager" or "comms".
 * @return PluginType The corresponding plugin type enum.
 */
PluginType stringToPluginType(const std::string &pluginTypeString);

/**
 * @brief The file type of the plugin. Currently either a shared library or Python code.
 *
 */
enum PluginFileType { PFT_SHARED_LIB, PFT_PYTHON };

/**
 * @brief Convert a PluginFileType value to a human readable string. This function is strictly
 * for logging and debugging. The output formatting may change without notice. Do NOT use this
 * for any logical comparisons, etc. The functionality of your plugin should in no way rely on
 * the output of this function.
 *
 * @param pluginFileType The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string pluginFileTypeToString(PluginFileType pluginFileType);
std::ostream &operator<<(std::ostream &out, PluginFileType pluginFileType);

/**
 * @brief Convert a plugin file type string to a plugin file type enum. If the plugin file type
 * string is invalid then an exception will be thrown.
 *
 * @param pluginTypeString The plugin file type as a string, either "network-manager" or "comms".
 * @return PluginDef::PluginType The corresponding plugin file type enum.
 */
PluginFileType stringToPluginFileType(const std::string &pluginFileTypeString);

/**
 * @brief The encryption type used for plugin file storage.
 */
enum StorageEncryptionType { ENC_AES = 0, ENC_NONE = 1 };

/**
 * @brief Convert a StorageEncryptionType value to a human readable string. This function is
 * strictly for logging and debugging. The output formatting may change without notice. Do NOT use
 * this for any logical comparisons, etc. The functionality of your plugin should in no way rely
 * on the output of this function.
 *
 * @param storageEncryptionType The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string storageEncryptionTypeToString(StorageEncryptionType storageEncryptionType);
std::ostream &operator<<(std::ostream &out, StorageEncryptionType storageEncryptionType);

/**
 * @brief Convert a storage encryption type string to a storage encryption type enum. If the storage
 * encryption type string is invalid then an exception will be thrown.
 *
 * @param storageEncryptionTypeString The storage encryption type as a string, either "aes" or
 *        "none".
 * @return StorageEncryptionType The corresponding storage encryption type enum.
 */
StorageEncryptionType stringToStorageEncryptionType(const std::string &storageEncryptionTypeString);

/**
 * @brief The type of information to display to the User
 *
 */
enum UserDisplayType {
    UD_DIALOG = 0,
    UD_QR_CODE = 1,
    UD_TOAST = 2,
    UD_NOTIFICATION = 3,
    UD_UNDEF = 4
};

/**
 * @brief Convert a UserDisplayType value to a human readable string. This function is strictly
 * for logging and debugging. The output formatting may change without notice. Do NOT use this for
 * any logical comparisons, etc. The functionality of your plugin should in no way rely on the
 * output of this function.
 *
 * @param userDisplayType The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string userInfoDisplayTypeToString(UserDisplayType userDisplayType);
std::ostream &operator<<(std::ostream &out, UserDisplayType userDisplayType);

/**
 * @brief Convert a user display type string to a user display type enum. If the plugin user display
 * type string is invalid then it will be set to undef.
 *
 * @param userDisplayTypeString The user display type as a string, either "dialog", "qr_code" or
 * "toast".
 * @return UserDisplayType The corresponding user display type enum.
 */
UserDisplayType stringToUserDisplayType(const std::string &userDisplayTypeString);

/**
 * @brief The action associated with the bootstrap information being displayed by the app.
 *
 */
enum BootstrapActionType {
    BS_PREPARING_BOOTSTRAP = 0,
    BS_PREPARING_CONFIGS = 1,
    BS_ACQUIRING_ARTIFACT = 2,
    BS_CREATING_BUNDLE = 3,
    BS_PREPARING_TRANSFER = 4,
    BS_DOWNLOAD_BUNDLE = 5,
    BS_NETWORK_CONNECT = 6,
    BS_COMPLETE = 7,
    BS_FAILED = 8,
    BS_UNDEF = 9
};

/**
 * @brief Convert a BootstrapActionType value to a human readable string. This function is strictly
 * for logging and debugging. The output formatting may change without notice. Do NOT use this for
 * any logical comparisons, etc. The functionality of your plugin should in no way rely on the
 * output of this function.
 *
 * @param bootstrapActionType The value to convert to a string.
 * @return std::string The string representation of the provided value.
 */
std::string bootstrapActionTypeToString(BootstrapActionType bootstrapActionType);
std::ostream &operator<<(std::ostream &out, BootstrapActionType bootstrapActionType);

/**
 * @brief Convert a bootstrap action type string to a BootstrapActionType enum. If the plugin action
 * type string is invalid then it will be set to undef.
 *
 * @param bootstrapActionType The user display type as a string, either "url" or "daemon_action"
 * @return BootstrapActionType The corresponding bootstrap action type
 */
BootstrapActionType stringToBootstrapActionType(const std::string &bootstrapActionTypeString);

}  // namespace RaceEnums

#endif