
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

#include "PluginCommsRustCppWrapper.h"

#include <RaceLog.h>

#include <string>
#include <vector>

PluginCommsRustCppWrapper::PluginCommsRustCppWrapper(IRaceSdkComms *_sdk) :
    sdk(_sdk), plugin(nullptr) {
    if (_sdk == nullptr) {
        RaceLog::logError("C Shim",
                          "sdk pointer provided to "
                          "PluginCommsRustCppWrapper is nullptr",
                          "");
        return;
    }
}

PluginCommsRustCppWrapper::~PluginCommsRustCppWrapper() {
    destroyPlugin();
}

PluginResponse PluginCommsRustCppWrapper::init(const PluginConfig &pluginConfig) {
    if (plugin != nullptr) {
        RaceLog::logError("C Shim",
                          "PluginCommsRustCppWrapper::init() called on "
                          "an already initialized instance.",
                          "");
        return PLUGIN_ERROR;
    }
    plugin = create_plugin(sdk);

    PluginResponse response;
    plugin_init(plugin, &response, pluginConfig.etcDirectory.c_str(),
                pluginConfig.loggingDirectory.c_str(), pluginConfig.auxDataDirectory.c_str(),
                pluginConfig.tmpDirectory.c_str(), pluginConfig.pluginDirectory.c_str());
    return response;
}

PluginResponse PluginCommsRustCppWrapper::shutdown() {
    PluginResponse response;
    plugin_shutdown(plugin, &response);
    return response;
}

PluginResponse PluginCommsRustCppWrapper::sendPackage(RaceHandle handle, ConnectionID connectionId,
                                                      EncPkg pkg, double timeoutTimestamp,
                                                      uint64_t batchId) {
    PluginResponse response;
    const auto cipherText = pkg.getRawData();
    plugin_send_package(plugin, &response, handle, connectionId.c_str(), cipherText.data(),
                        cipherText.size(), timeoutTimestamp, batchId);
    return response;
}

PluginResponse PluginCommsRustCppWrapper::openConnection(RaceHandle handle, LinkType linkType,
                                                         LinkID linkId, std::string linkHints,
                                                         int32_t sendTimeout) {
    PluginResponse response;
    plugin_open_connection(plugin, &response, handle, linkType, linkId.c_str(), linkHints.c_str(),
                           sendTimeout);
    return response;
}

PluginResponse PluginCommsRustCppWrapper::closeConnection(RaceHandle handle,
                                                          ConnectionID connectionId) {
    PluginResponse response;
    plugin_close_connection(plugin, &response, handle, connectionId.c_str());
    return response;
}

PluginResponse PluginCommsRustCppWrapper::destroyLink(RaceHandle handle, LinkID linkId) {
    PluginResponse response;
    plugin_destroy_link(plugin, &response, handle, linkId.c_str());
    return response;
}

PluginResponse PluginCommsRustCppWrapper::createLink(RaceHandle handle, std::string channelGid) {
    PluginResponse response;
    plugin_create_link(plugin, &response, handle, channelGid.c_str());
    return response;
}

PluginResponse PluginCommsRustCppWrapper::loadLinkAddress(RaceHandle handle, std::string channelGid,
                                                          std::string linkAddress) {
    PluginResponse response;
    plugin_load_link_address(plugin, &response, handle, channelGid.c_str(), linkAddress.c_str());
    return response;
}

PluginResponse PluginCommsRustCppWrapper::loadLinkAddresses(
    RaceHandle handle, std::string channelGid, std::vector<std::string> linkAddresses) {
    PluginResponse response;
    std::vector<const char *> linkAddressesC;
    for (auto &linkAddress : linkAddresses) {
        linkAddressesC.push_back(linkAddress.c_str());
    }
    plugin_load_link_addresses(plugin, &response, handle, channelGid.c_str(), linkAddressesC.data(),
                               linkAddressesC.size());
    return response;
}

PluginResponse PluginCommsRustCppWrapper::createLinkFromAddress(RaceHandle handle,
                                                                std::string channelGid,
                                                                std::string linkAddress) {
    PluginResponse response;
    plugin_create_link_from_address(plugin, &response, handle, channelGid.c_str(),
                                    linkAddress.c_str());
    return response;
}

PluginResponse PluginCommsRustCppWrapper::deactivateChannel(RaceHandle handle,
                                                            std::string channelGid) {
    PluginResponse response;
    plugin_deactivate_channel(plugin, &response, handle, channelGid.c_str());
    return response;
}

PluginResponse PluginCommsRustCppWrapper::activateChannel(RaceHandle handle, std::string channelGid,
                                                          std::string roleName) {
    PluginResponse response;
    plugin_activate_channel(plugin, &response, handle, channelGid.c_str(), roleName.c_str());
    return response;
}

PluginResponse PluginCommsRustCppWrapper::onUserInputReceived(RaceHandle handle, bool answered,
                                                              const std::string &userResponse) {
    PluginResponse response;
    plugin_on_user_input_received(plugin, &response, handle, answered, userResponse.c_str());
    return response;
}

PluginResponse PluginCommsRustCppWrapper::flushChannel(RaceHandle handle,
                                                       // cppcheck-suppress passedByValue
                                                       const std::string channelGid,
                                                       uint64_t batchId) {
    PluginResponse response;
    plugin_flush_channel(plugin, &response, handle, channelGid.c_str(), batchId);
    return response;
}

PluginResponse PluginCommsRustCppWrapper::onUserAcknowledgementReceived(RaceHandle handle) {
    PluginResponse response;
    plugin_on_user_acknowledgment_received(plugin, &response, handle);
    return response;
}

void PluginCommsRustCppWrapper::destroyPlugin() {
    if (plugin != nullptr) {
        // TODO: need to validate that this call actually works. Looks like it
        // does not...
        destroy_plugin(plugin);
        plugin = nullptr;
    }
}
