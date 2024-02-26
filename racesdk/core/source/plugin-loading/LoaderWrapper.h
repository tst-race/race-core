
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

#ifndef __LOADING_WRAPPER_H__
#define __LOADING_WRAPPER_H__

#include <RacePluginExports.h>

#include <cctype>
#include <memory>
#include <sstream>

#include "DynamicLibrary.h"
#include "OpenTracingForwardDeclarations.h"
#include "RaceSdk.h"
#include "filesystem.h"
#include "helper.h"

template <class Parent>
class LoaderWrapper : public Parent {
private:
    DynamicLibrary dl;
    using Interface = typename Parent::Interface;
    using SDK = typename Parent::SDK;

public:
    LoaderWrapper(const fs::path &path, RaceSdk &sdk, const std::string &name,
                  const std::string &configPath = "") :
        Parent(sdk, name), dl(path) {
        helper::logDebug("LoaderWrapper: called. path: " + path.string());
        auto create = dl.get<Interface *(SDK *)>(Parent::createFuncName);
        auto destroy = dl.get<void(Interface *)>(Parent::destroyFuncName);
        auto version = dl.get<const RaceVersionInfo>("raceVersion");
        auto pluginId = dl.get<const char *const>("racePluginId");
        auto pluginDesc = dl.get<const char *const>("racePluginDescription");

        std::stringstream debugMessage;
        debugMessage << "LoaderWrapper: Loading plugin: " << path
                     << ". Version: " + versionToString(version) << ". ID: " << pluginId
                     << ". Description: " << pluginDesc;
        helper::logDebug(debugMessage.str());

        if (version != RACE_VERSION) {
            const std::string errorMessage =
                "LoaderWrapper: Mismatched RACE version number. Expected " +
                versionToString(RACE_VERSION) + ". Found: " + versionToString(version);
            helper::logError(errorMessage);
            throw std::runtime_error(errorMessage);
        }
        if (pluginId == nullptr || pluginId[0] == '\0') {
            const std::string errorMessage =
                "LoaderWrapper: Invalid plugin ID: null or emptry string.";
            helper::logError(errorMessage);
            throw std::runtime_error(errorMessage);
        }
        for (const char *c = pluginId; *c != '\0'; ++c) {
            if (!std::isalnum(*c) && *c != '-' && *c != '_') {
                const std::string errorMessage =
                    "LoaderWrapper: Invalid character in plugin ID: " + std::string(c);
                helper::logError(errorMessage);
                throw std::runtime_error(errorMessage);
            }
        }
        if (pluginDesc == nullptr || pluginDesc[0] == '\0') {
            const std::string errorMessage =
                "LoaderWrapper: Invalid plugin description: null or emptry string.";
            helper::logError(errorMessage);
            throw std::runtime_error(errorMessage);
        }

        auto plugin = std::shared_ptr<Interface>(create(this->getSdk()), destroy);
        if (!plugin) {
            const std::string errorMessage = "LoaderWrapper: plugin is null.";
            helper::logError(errorMessage);
            throw std::runtime_error(errorMessage);
        }

        this->mPlugin = plugin;
        this->mId = pluginId;
        this->mDescription = pluginDesc;
        this->mConfigPath = configPath;

        helper::logDebug("LoaderWrapper: returned");
    }
    // Ensure Parent type has a virtual destructor (required for unique_ptr conversion)
    virtual ~LoaderWrapper() override {
        helper::logDebug("LoaderWrapper::~LoaderWrapper: called");
        this->mPlugin.reset();
        helper::logDebug("LoaderWrapper::~LoaderWrapper: returned");
    }

protected:
    static std::string versionToString(const RaceVersionInfo &version) {
        std::stringstream versionString;
        versionString << version.major << "." << version.minor << "." << version.compatibility;
        return versionString.str();
    }
};

#endif
