
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

#ifndef __I_RACE_PLUGIN_ARTIFACT_MANAGER_H_
#define __I_RACE_PLUGIN_ARTIFACT_MANAGER_H_

#include <string>

#include "IRaceSdkArtifactManager.h"
#include "PluginConfig.h"
#include "PluginResponse.h"
#include "RacePluginExports.h"

class IRacePluginArtifactManager {
public:
    /**
     * @brief Destroy the IRacePluginArtifactManager object
     */
    virtual ~IRacePluginArtifactManager() {}

    /**
     * @brief Initialize the plugin. Prepare prep work to begin allowing calls
     *        from core.
     *
     * @param pluginConfig Config object containing dynamic config variables (e.g. paths)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse init(const PluginConfig &pluginConfig) = 0;

    /**
     * @brief Acquire the artifact with the given file name and place it at the specified
     * destination path.
     *
     * @param destPath Destination path at which to place the acquired artifact
     * @param fileName Name of the artifact file to be acquired
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse acquireArtifact(const std::string &destPath,
                                           const std::string &fileName) = 0;

    /**
     * @brief Notify the plugin about received user input response
     *
     * @param handle The handle for this callback
     * @param answered True if the response contains an actual answer to the input prompt, otherwise
     * the response is an empty string and not valid
     * @param response The user response answer to the input prompt
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onUserInputReceived(RaceHandle handle, bool answered,
                                               const std::string &response) = 0;

    /**
     * @brief Notify the plugin that the user acknowledged the displayed information
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onUserAcknowledgementReceived(RaceHandle handle) = 0;

    /**
     * @brief Receive a response from a registry node
     *
     * @param message The body of the message that was received
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse receiveAmpMessage(const std::string &message) = 0;
};

extern "C" EXPORT IRacePluginArtifactManager *createPluginArtifactManager(
    IRaceSdkArtifactManager *raceSdk);
extern "C" EXPORT void destroyPluginArtifactManager(IRacePluginArtifactManager *plugin);

#endif