
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

#include "DecomposedPluginLoader.h"

#include "CompositeWrapper.h"
#include "decomposed-comms/ComponentManager.h"
#include "helper.h"

DecomposedPluginLoader::DecomposedPluginLoader(const std::string &pluginPath) :
    pluginPath(pluginPath) {}

void DecomposedPluginLoader::loadComponentsForPlugin(const PluginDef &pluginToLoad) {
    TRACE_METHOD(pluginToLoad.filePath, pluginToLoad.sharedLibraryPath);

    std::shared_ptr<IComponentPlugin> plugin = nullptr;
    std::string fullPluginPath{};

    if (pluginToLoad.fileType == RaceEnums::PFT_SHARED_LIB) {
        fullPluginPath = pluginPath + pluginToLoad.filePath + "/" + pluginToLoad.sharedLibraryPath;
        helper::logDebug(logPrefix + "Loading component shared library plugin from " +
                         fullPluginPath);
        plugin = std::make_unique<ComponentPlugin>(const_cast<std::string &>(fullPluginPath));
    } else if (pluginToLoad.fileType == RaceEnums::PFT_PYTHON) {
        fullPluginPath = pluginPath + pluginToLoad.filePath;
        plugin = std::make_unique<PythonComponentPlugin>(const_cast<std::string &>(fullPluginPath),
                                                         pluginToLoad.pythonModule);
        helper::logDebug(logPrefix + "Loading component python plugin from " + fullPluginPath);
    } else {
        throw std::runtime_error("Unknown plugin type ");
    }

    for (auto &transport : pluginToLoad.transports) {
        auto it = transports.find(transport);
        if (it != transports.end()) {
            helper::logError(logPrefix + "transport " + transport +
                             " already exists. Previous transport supplied by " +
                             it->second->get_path() + ". New transport supplied by " +
                             fullPluginPath);

            throw std::runtime_error("Multiple definitions of transport" + transport);
        }
        transports[transport] = plugin.get();
    }
    for (auto &usermodel : pluginToLoad.usermodels) {
        auto it = usermodels.find(usermodel);
        if (it != usermodels.end()) {
            helper::logError(logPrefix + "usermodel " + usermodel +
                             " already exists. Previous usermodel supplied by " +
                             it->second->get_path() + ". New usermodel supplied by " +
                             fullPluginPath);

            throw std::runtime_error("Multiple definitions of usermodel" + usermodel);
        }
        usermodels[usermodel] = plugin.get();
    }
    for (auto &encoding : pluginToLoad.encodings) {
        auto it = encodings.find(encoding);
        if (it != encodings.end()) {
            helper::logError(logPrefix + "encoding " + encoding +
                             " already exists. Previous encoding supplied by " +
                             it->second->get_path() + ". New encoding supplied by " +
                             fullPluginPath);

            throw std::runtime_error("Multiple definitions of encoding" + encoding);
        }
        encodings[encoding] = plugin.get();
    }
    plugins.emplace_back(std::move(plugin));
}

void DecomposedPluginLoader::loadComponents(std::vector<PluginDef> componentPlugins) {
    TRACE_METHOD();
    for (auto &plugin : componentPlugins) {
        loadComponentsForPlugin(plugin);
    }

    helper::logDebug(logPrefix + "Loaded plugins containing:");
    helper::logDebug(logPrefix + "Transports:");
    for (auto &kv : transports) {
        helper::logDebug(logPrefix + "    " + kv.first);
    }
    helper::logDebug(logPrefix + "User Models:");
    for (auto &kv : usermodels) {
        helper::logDebug(logPrefix + "    " + kv.first);
    }
    helper::logDebug(logPrefix + "Encodings:");
    for (auto &kv : encodings) {
        helper::logDebug(logPrefix + "    " + kv.first);
    }
}

IPluginLoader::PluginList<CommsWrapper> DecomposedPluginLoader::compose(
    std::vector<Composition> compositions, RaceSdk &sdk) {
    TRACE_METHOD();
    IPluginLoader::PluginList<CommsWrapper> commsWrappers;
    for (auto &composition : compositions) {
        auto description = composition.description();
        helper::logDebug(logPrefix + "Creating composition: " + description);

        auto &transport = *transports.at(composition.transport);
        auto &usermodel = *usermodels.at(composition.usermodel);
        std::unordered_map<std::string, IComponentPlugin *> compositeEncodings;
        for (auto &encoding : composition.encodings) {
            compositeEncodings[encoding] = encodings.at(encoding);
        }

        commsWrappers.emplace_back(std::make_unique<CompositeWrapper>(
            sdk, composition, description, transport, usermodel, compositeEncodings));
    }
    return commsWrappers;
}