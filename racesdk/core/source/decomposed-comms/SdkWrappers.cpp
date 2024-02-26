
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

#include "SdkWrappers.h"

#include "ComponentManager.h"

ComponentSdkBaseWrapper::ComponentSdkBaseWrapper(ComponentManager &manager, const std::string &id) :
    manager(manager), id(id) {}

std::string ComponentSdkBaseWrapper::getActivePersona() {
    return manager.getActivePersona();
}

ChannelResponse ComponentSdkBaseWrapper::requestPluginUserInput(const std::string &key,
                                                                const std::string &prompt,
                                                                bool cache) {
    return manager.requestPluginUserInput(id, key, prompt, cache);
}

ChannelResponse ComponentSdkBaseWrapper::requestCommonUserInput(const std::string &key) {
    return manager.requestCommonUserInput(id, key);
}

ChannelResponse ComponentSdkBaseWrapper::updateState(ComponentState state) {
    return manager.updateState(id, state);
}

std::string ComponentSdkBaseWrapper::toString() const {
    return "<Sdk Wrapper for " + id + ">";
}

std::ostream &operator<<(std::ostream &out, const ComponentSdkBaseWrapper &wrapper) {
    return out << wrapper.toString();
}

TransportSdkWrapper::TransportSdkWrapper(ComponentManager &sdk, const std::string &id) :
    ComponentSdkBaseWrapper(sdk, id) {}

ChannelResponse ComponentSdkBaseWrapper::makeDir(const std::string &directoryPath) {
    return manager.makeDir(directoryPath);
}

ChannelResponse ComponentSdkBaseWrapper::removeDir(const std::string &directoryPath) {
    return manager.removeDir(directoryPath);
}

std::vector<std::string> ComponentSdkBaseWrapper::listDir(const std::string &directoryPath) {
    return manager.listDir(directoryPath);
}

std::vector<std::uint8_t> ComponentSdkBaseWrapper::readFile(const std::string &filepath) {
    return manager.readFile(filepath);
}

ChannelResponse ComponentSdkBaseWrapper::appendFile(const std::string &filepath,
                                                    const std::vector<std::uint8_t> &data) {
    return manager.appendFile(filepath, data);
}

ChannelResponse ComponentSdkBaseWrapper::writeFile(const std::string &filepath,
                                                   const std::vector<std::uint8_t> &data) {
    return manager.writeFile(filepath, data);
}

ChannelProperties TransportSdkWrapper::getChannelProperties() {
    return manager.getChannelProperties();
}

ChannelResponse TransportSdkWrapper::onLinkStatusChanged(RaceHandle handle, const LinkID &linkId,
                                                         LinkStatus status,
                                                         const LinkParameters &params) {
    return manager.onLinkStatusChanged(handle, linkId, status, params);
}

ChannelResponse TransportSdkWrapper::onPackageStatusChanged(RaceHandle handle,
                                                            PackageStatus status) {
    return manager.onPackageStatusChanged(handle, status);
}

ChannelResponse TransportSdkWrapper::onEvent(const Event &event) {
    return manager.onEvent(event);
}

ChannelResponse TransportSdkWrapper::onReceive(const LinkID &linkId,
                                               const EncodingParameters &params,
                                               const std::vector<uint8_t> &bytes) {
    return manager.onReceive(linkId, params, bytes);
}

UserModelSdkWrapper::UserModelSdkWrapper(ComponentManager &sdk, const std::string &id) :
    ComponentSdkBaseWrapper(sdk, id) {}

ChannelResponse UserModelSdkWrapper::onTimelineUpdated() {
    return manager.onTimelineUpdated();
}

EncodingSdkWrapper::EncodingSdkWrapper(ComponentManager &sdk, const std::string &id) :
    ComponentSdkBaseWrapper(sdk, id) {}

ChannelResponse EncodingSdkWrapper::onBytesEncoded(RaceHandle handle,
                                                   const std::vector<uint8_t> &bytes,
                                                   EncodingStatus status) {
    return manager.onBytesEncoded(handle, bytes, status);
}

ChannelResponse EncodingSdkWrapper::onBytesDecoded(RaceHandle handle,
                                                   const std::vector<uint8_t> &bytes,
                                                   EncodingStatus status) {
    return manager.onBytesDecoded(handle, bytes, status);
}
