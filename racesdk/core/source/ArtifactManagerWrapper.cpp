
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

#include "ArtifactManagerWrapper.h"

#include "helper.h"

ArtifactManagerWrapper::ArtifactManagerWrapper(RaceSdk &sdk, const std::string & /* name */) :
    raceSdk(sdk) {}

ArtifactManagerWrapper::ArtifactManagerWrapper(std::shared_ptr<IRacePluginArtifactManager> plugin,
                                               std::string id, std::string description,
                                               RaceSdk &sdk, const std::string &configPath) :
    raceSdk(sdk),
    mPlugin(std::move(plugin)),
    mId(std::move(id)),
    mDescription(std::move(description)),
    mConfigPath(configPath) {}

PluginResponse ArtifactManagerWrapper::init(const PluginConfig &pluginConfig) {
    TRACE_METHOD();
    PluginResponse response = mPlugin->init(pluginConfig);
    return response;
}

PluginResponse ArtifactManagerWrapper::acquireArtifact(const std::string &destPath,
                                                       const std::string &fileName) {
    TRACE_METHOD();
    PluginResponse response = mPlugin->acquireArtifact(destPath, fileName);
    return response;
}

PluginResponse ArtifactManagerWrapper::onUserInputReceived(RaceHandle handle, bool answered,
                                                           const std::string &response) {
    TRACE_METHOD();
    PluginResponse pluginResponse = mPlugin->onUserInputReceived(handle, answered, response);
    return pluginResponse;
}

PluginResponse ArtifactManagerWrapper::onUserAcknowledgementReceived(RaceHandle handle) {
    TRACE_METHOD();
    PluginResponse response = mPlugin->onUserAcknowledgementReceived(handle);
    return response;
}

PluginResponse ArtifactManagerWrapper::receiveAmpMessage(const std::string &message) {
    TRACE_METHOD();
    PluginResponse response = mPlugin->receiveAmpMessage(message);
    return response;
}

std::string ArtifactManagerWrapper::getAppPath() {
    TRACE_METHOD();
    return raceSdk.getAppPath(getId());
}

SdkResponse ArtifactManagerWrapper::requestPluginUserInput(const std::string &key,
                                                           const std::string &prompt, bool cache) {
    TRACE_METHOD();
    SdkResponse response = raceSdk.requestPluginUserInput(getId(), false, key, prompt, cache);
    return response;
}

SdkResponse ArtifactManagerWrapper::requestCommonUserInput(const std::string &key) {
    TRACE_METHOD();
    SdkResponse response = raceSdk.requestCommonUserInput(getId(), false, key);
    return response;
}

SdkResponse ArtifactManagerWrapper::displayInfoToUser(const std::string &data,
                                                      RaceEnums::UserDisplayType displayType) {
    TRACE_METHOD();
    SdkResponse response = raceSdk.displayInfoToUser(getId(), data, displayType);
    return response;
}

SdkResponse ArtifactManagerWrapper::sendAmpMessage(const std::string &destination,
                                                   const std::string &message) {
    TRACE_METHOD();
    SdkResponse response = raceSdk.sendAmpMessage(getId(), destination, message);
    return response;
}
