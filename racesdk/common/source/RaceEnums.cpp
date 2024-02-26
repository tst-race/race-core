
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

#include "RaceEnums.h"

#include <algorithm>
#include <stdexcept>

/**
 * @brief Convert a string to lowercase.
 *
 * @param input The string to convert.
 * @return std::string The lower case version of the input string.
 */
static std::string stringToLowerCase(const std::string &input) {
    std::string output = input;
    std::transform(output.begin(), output.end(), output.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return output;
}

std::string RaceEnums::nodeTypeToString(RaceEnums::NodeType nodeType) {
    switch (nodeType) {
        case RaceEnums::NT_ALL:
            return "all";
        case RaceEnums::NT_CLIENT:
            return "client";
        case RaceEnums::NT_SERVER:
            return "server";
        case RaceEnums::NT_UNDEF:
            return "undefined";
        default:
            return "ERROR: INVALID PLUGIN FILE TYPE: " + std::to_string(nodeType);
    }
}

std::ostream &RaceEnums::operator<<(std::ostream &out, RaceEnums::NodeType nodeType) {
    return out << RaceEnums::nodeTypeToString(nodeType);
}

RaceEnums::NodeType RaceEnums::stringToNodeType(const std::string &nodeTypeString) {
    std::string pluginNodeType = stringToLowerCase(nodeTypeString);
    if (pluginNodeType == "client") {
        return RaceEnums::NodeType::NT_CLIENT;
    } else if (pluginNodeType == "server") {
        return RaceEnums::NodeType::NT_SERVER;
    } else if (pluginNodeType == "any") {
        return RaceEnums::NodeType::NT_ALL;
    }
    return RaceEnums::NodeType::NT_UNDEF;
}

std::string RaceEnums::pluginTypeToString(RaceEnums::PluginType pluginType) {
    switch (pluginType) {
        case RaceEnums::PT_NM:
            return "PT_NM";
        case RaceEnums::PT_COMMS:
            return "PT_COMMS";
        case RaceEnums::PT_ARTIFACT_MANAGER:
            return "PT_ARTIFACT_MANAGER";
        default:
            return "ERROR: INVALID PLUGIN TYPE: " + std::to_string(pluginType);
    }
}

std::ostream &RaceEnums::operator<<(std::ostream &out, RaceEnums::PluginType pluginType) {
    return out << RaceEnums::pluginTypeToString(pluginType);
}

RaceEnums::PluginType RaceEnums::stringToPluginType(const std::string &pluginTypeString) {
    std::string pluginType = stringToLowerCase(pluginTypeString);
    if (pluginType == "network-manager") {
        return RaceEnums::PluginType::PT_NM;
    } else if (pluginType == "comms") {
        return RaceEnums::PluginType::PT_COMMS;
    } else if (pluginType == "artifactmanager") {
        return RaceEnums::PluginType::PT_ARTIFACT_MANAGER;
    }
    throw std::invalid_argument("RaceEnums::stringToPluginType: invalid plugin type " +
                                pluginTypeString);
}

std::string RaceEnums::pluginFileTypeToString(RaceEnums::PluginFileType pluginFileType) {
    switch (pluginFileType) {
        case RaceEnums::PFT_SHARED_LIB:
            return "PFT_SHARED_LIB";
        case RaceEnums::PFT_PYTHON:
            return "PFT_PYTHON";
        default:
            return "ERROR: INVALID PLUGIN FILE TYPE: " + std::to_string(pluginFileType);
    }
}

std::ostream &RaceEnums::operator<<(std::ostream &out, RaceEnums::PluginFileType pluginFileType) {
    return out << RaceEnums::pluginFileTypeToString(pluginFileType);
}

RaceEnums::PluginFileType RaceEnums::stringToPluginFileType(
    const std::string &pluginFileTypeString) {
    std::string pluginFileType = stringToLowerCase(pluginFileTypeString);
    if (pluginFileType == "shared_library") {
        return RaceEnums::PluginFileType::PFT_SHARED_LIB;
    } else if (pluginFileType == "python") {
        return RaceEnums::PluginFileType::PFT_PYTHON;
    }

    throw std::invalid_argument("RaceEnums::stringToPluginFileType: invalid plugin file type " +
                                pluginFileTypeString);
}

std::string RaceEnums::storageEncryptionTypeToString(
    RaceEnums::StorageEncryptionType storageEncryptionType) {
    switch (storageEncryptionType) {
        case RaceEnums::ENC_AES:
            return "AES";
        case RaceEnums::ENC_NONE:
            return "NONE";
        default:
            return "ERROR: INVALID STORAGE ENCRYPTION TYPE: " +
                   std::to_string(storageEncryptionType);
    }
}

std::ostream &RaceEnums::operator<<(std::ostream &out,
                                    RaceEnums::StorageEncryptionType storageEncryptionType) {
    return out << RaceEnums::storageEncryptionTypeToString(storageEncryptionType);
}

RaceEnums::StorageEncryptionType RaceEnums::stringToStorageEncryptionType(
    const std::string &storageEncryptionTypeString) {
    std::string storageEncryptionType = stringToLowerCase(storageEncryptionTypeString);
    if (storageEncryptionType == "aes") {
        return RaceEnums::StorageEncryptionType::ENC_AES;
    } else if (storageEncryptionType == "none") {
        return RaceEnums::StorageEncryptionType::ENC_NONE;
    }

    throw std::invalid_argument(
        "RaceEnums::stringToStorageEncryptionType: invalid storage encryption type " +
        storageEncryptionTypeString);
}

std::string RaceEnums::userInfoDisplayTypeToString(RaceEnums::UserDisplayType userDisplayType) {
    switch (userDisplayType) {
        case RaceEnums::UD_DIALOG:
            return "DIALOG";
        case RaceEnums::UD_QR_CODE:
            return "QR_CODE";
        case RaceEnums::UD_TOAST:
            return "TOAST";
        case RaceEnums::UD_NOTIFICATION:
            return "NOTIFICATION";
        case RaceEnums::UD_UNDEF:
            return "UNDEF";
        default:
            return "ERROR: INVALID USER INFO DISPLAY TYPE: " + std::to_string(userDisplayType);
    }
}

std::ostream &RaceEnums::operator<<(std::ostream &out, RaceEnums::UserDisplayType userDisplayType) {
    return out << RaceEnums::userInfoDisplayTypeToString(userDisplayType);
}

RaceEnums::UserDisplayType RaceEnums::stringToUserDisplayType(
    const std::string &userDisplayTypeString) {
    std::string userDisplayType = stringToLowerCase(userDisplayTypeString);
    if (userDisplayType == "dialog") {
        return RaceEnums::UserDisplayType::UD_DIALOG;
    } else if (userDisplayType == "qr_code") {
        return RaceEnums::UserDisplayType::UD_QR_CODE;
    } else if (userDisplayType == "toast") {
        return RaceEnums::UserDisplayType::UD_NOTIFICATION;
    } else if (userDisplayType == "notification") {
        return RaceEnums::UserDisplayType::UD_TOAST;
    } else if (userDisplayType == "undef") {
        return RaceEnums::UserDisplayType::UD_UNDEF;
    }

    throw std::invalid_argument(
        "RaceEnums::stringToUserDisplayType: invalid user info display type " +
        userDisplayTypeString);
}

std::string RaceEnums::bootstrapActionTypeToString(
    RaceEnums::BootstrapActionType bootstrapActionType) {
    switch (bootstrapActionType) {
        case RaceEnums::BS_PREPARING_BOOTSTRAP:
            return "BS_PREPARING_BOOTSTRAP";
        case RaceEnums::BS_PREPARING_CONFIGS:
            return "BS_PREPARING_CONFIGS";
        case RaceEnums::BS_ACQUIRING_ARTIFACT:
            return "BS_ACQUIRING_ARTIFACT";
        case RaceEnums::BS_CREATING_BUNDLE:
            return "BS_CREATING_BUNDLE";
        case RaceEnums::BS_PREPARING_TRANSFER:
            return "BS_PREPARING_TRANSFER";
        case RaceEnums::BS_DOWNLOAD_BUNDLE:
            return "BS_DOWNLOAD_BUNDLE";
        case RaceEnums::BS_NETWORK_CONNECT:
            return "BS_NETWORK_CONNECT";
        case RaceEnums::BS_COMPLETE:
            return "BS_COMPLETE";
        case RaceEnums::BS_FAILED:
            return "BS_FAILED";
        case RaceEnums::BS_UNDEF:
            return "BS_UNDEF";
        default:
            return "ERROR: INVALID BOOTSTRAP ACTION TYPE: " + std::to_string(bootstrapActionType);
    }
}

std::ostream &RaceEnums::operator<<(std::ostream &out,
                                    RaceEnums::BootstrapActionType bootstrapActionType) {
    return out << RaceEnums::bootstrapActionTypeToString(bootstrapActionType);
}

RaceEnums::BootstrapActionType RaceEnums::stringToBootstrapActionType(
    const std::string &bootstrapActionTypeString) {
    std::string bootstrapActionType = stringToLowerCase(bootstrapActionTypeString);
    if (bootstrapActionType == "bs_preparing_bootstrap") {
        return RaceEnums::BootstrapActionType::BS_PREPARING_BOOTSTRAP;
    } else if (bootstrapActionType == "bs_preparing_configs") {
        return RaceEnums::BootstrapActionType::BS_PREPARING_CONFIGS;
    } else if (bootstrapActionType == "bs_acquiring_artifact") {
        return RaceEnums::BootstrapActionType::BS_ACQUIRING_ARTIFACT;
    } else if (bootstrapActionType == "bs_creating_bundle") {
        return RaceEnums::BootstrapActionType::BS_CREATING_BUNDLE;
    } else if (bootstrapActionType == "bs_preparing_transfer") {
        return RaceEnums::BootstrapActionType::BS_PREPARING_TRANSFER;
    } else if (bootstrapActionType == "bs_download_bundle") {
        return RaceEnums::BootstrapActionType::BS_DOWNLOAD_BUNDLE;
    } else if (bootstrapActionType == "bs_network_connect") {
        return RaceEnums::BootstrapActionType::BS_NETWORK_CONNECT;
    } else if (bootstrapActionType == "bs_complete") {
        return RaceEnums::BootstrapActionType::BS_COMPLETE;
    } else if (bootstrapActionType == "bs_failed") {
        return RaceEnums::BootstrapActionType::BS_FAILED;
    } else if (bootstrapActionType == "bs_undef") {
        return RaceEnums::BootstrapActionType::BS_UNDEF;
    }

    throw std::invalid_argument(
        "RaceEnums::stringToBootstrapActionType: invalid user info display type " +
        bootstrapActionType);
}