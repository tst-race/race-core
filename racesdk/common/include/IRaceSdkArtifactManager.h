
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

#ifndef __I_RACE_SDK_ARTIFACT_MANAGER_H_
#define __I_RACE_SDK_ARTIFACT_MANAGER_H_

#include <string>

#include "RaceEnums.h"
#include "SdkResponse.h"

class IRaceSdkArtifactManager {
public:
    /**
     * @brief Destroy the IRaceSdkArtifactManager object
     */
    virtual ~IRaceSdkArtifactManager() {}

    /**
     * @brief Get the binary directory for the race application
     *
     * @return std::string The path to the application
     */
    virtual std::string getAppPath() = 0;

    /**
     * @brief Request plugin-specific input from the user with the specified prompt message.
     * The response may be cached in persistent storage, in which case the user will not be
     * re-prompted if a cached response exists for the given prompt.
     *
     * The response will be provided in the userInputReceived callback with the handle matching
     * the handle returned in this SdkResponse object.
     *
     * @param key Prompt identifier for the user input request
     * @param prompt Message to be presented to the user
     * @param cache If true, the response will be cached in persistent storage
     * @return SdkResponse indicator of success or failure of the request
     */
    virtual SdkResponse requestPluginUserInput(const std::string &key, const std::string &prompt,
                                               bool cache) = 0;

    /**
     * @brief Request application-wide input from the user associated with the given key.
     * The key identifies a common user input prompt and must be a key supported by the
     * RACE SDK. The response is cached in persistent storage, so the user will not be
     * re-prompted if a cached response exists for the given key.
     *
     * The response will be provided in the userInputReceived callback with the handle matching
     * the handle returned in this SdkResponse object.
     *
     * @param key Prompt identifier for the application-wide user input request
     * @return SdkResponse indicator of success or failure of the request
     */
    virtual SdkResponse requestCommonUserInput(const std::string &key) = 0;

    /**
     * @brief Displays information to the User
     *
     * The task posted to the work queue will display information to the user input prompt, wait an
     * optional amount of time, then notify the SDK of the user acknowledgment.
     *
     * @param data data to display
     * @param displayType type of user display to display data in
     * @return SdkResponse object that contains whether the post was successful
     */
    virtual SdkResponse displayInfoToUser(const std::string &data,
                                          RaceEnums::UserDisplayType displayType) = 0;

    /**
     * @brief Send a message to a registry node
     *
     * @param destination The persona of the registry node
     * @param message The body of the message to send
     * @return SdkResponse object that contains whether the call was valid
     */
    virtual SdkResponse sendAmpMessage(const std::string &destination,
                                       const std::string &message) = 0;
};

#endif