
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

#ifndef __DOWNLOADER_WRAPPER_H__
#define __DOWNLOADER_WRAPPER_H__

#include <memory>
#include <string>

#include "IRacePluginArtifactManager.h"
#include "IRaceSdkArtifactManager.h"
#include "RaceSdk.h"

class ArtifactManagerWrapper : public IRaceSdkArtifactManager {
protected:
    RaceSdk &raceSdk;

    std::shared_ptr<IRacePluginArtifactManager> mPlugin;
    std::string mId;
    std::string mDescription;
    std::string mConfigPath;

    explicit ArtifactManagerWrapper(RaceSdk &sdk, const std::string &name);

    using Interface = IRacePluginArtifactManager;
    using SDK = IRaceSdkArtifactManager;

    static constexpr const char *createFuncName = "createPluginArtifactManager";
    static constexpr const char *destroyFuncName = "destroyPluginArtifactManager";

    SDK *getSdk() {
        return this;
    }

public:
    ArtifactManagerWrapper(std::shared_ptr<IRacePluginArtifactManager> plugin, std::string id,
                           std::string description, RaceSdk &sdk,
                           const std::string &configPath = "");
    virtual ~ArtifactManagerWrapper() {}

    /**
     * @brief Get the ID of the wrapped plugin
     *
     * @return The ID of the wrapped plugin
     */
    virtual const std::string &getId() const {
        return mId;
    }

    /**
     * @brief Get the config path of the wrapped IRacePluginNM. The config path is the relative
     * location of the path containing configuration files to be used by the plugin.
     *
     * @return const std::string & The id of the wrapped plugin
     */
    const std::string &getConfigPath() const {
        return !mConfigPath.empty() ? mConfigPath : mId;
    }

    /**
     * @brief Get the description string of the wrapped IRacePluginNM
     *
     * @return const std::string & The description of the wrapped plugin
     */
    const std::string &getDescription() const {
        return mDescription;
    }

    /**
     * @brief Initialize the plugin. Prepare prep work to begin allowing calls
     *        from core.
     *
     * @param pluginConfig Config object containing dynamic config variables (e.g. paths)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse init(const PluginConfig &pluginConfig);

    /**
     * @brief Acquire the artifact with the given file name and place it at the specified
     * destination path.
     *
     * @param destPath Destination path at which to place the acquired artifact
     * @param fileName Name of the artifact file to be acquired
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse acquireArtifact(const std::string &destPath,
                                           const std::string &fileName);

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
                                               const std::string &response);

    /**
     * @brief Notify the plugin that the user acknowledged the displayed information
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onUserAcknowledgementReceived(RaceHandle handle);

    virtual PluginResponse receiveAmpMessage(const std::string &message);

    // IRaceSdkArtifactManager

    virtual std::string getAppPath() override;
    virtual SdkResponse requestPluginUserInput(const std::string &key, const std::string &prompt,
                                               bool cache) override;
    virtual SdkResponse requestCommonUserInput(const std::string &key) override;
    virtual SdkResponse displayInfoToUser(const std::string &data,
                                          RaceEnums::UserDisplayType displayType) override;

    /**
     * @brief Send a message to a registry node
     *
     * @param destination The persona of the registry node
     * @param message The body of the message to send
     * @return SdkResponse object that contains whether the call was valid
     */
    virtual SdkResponse sendAmpMessage(const std::string &destination,
                                       const std::string &message) override;
};

#endif