
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

#pragma once

#include "ComponentManagerTypes.h"
#include "ComponentWrappers.h"
#include "Composition.h"
#include "IRacePluginComms.h"
#include "SdkWrappers.h"
#include "plugin-loading/IComponentPlugin.h"

class ComponentManagerInternal;

class ComponentLifetimeManager {
public:
    ComponentLifetimeManager(
        ComponentManagerInternal &manager, const Composition &composition,
        IComponentPlugin &transportPlugin, IComponentPlugin &usermodelPlugin,
        const std::unordered_map<std::string, IComponentPlugin *> &encodingPlugins);
    ~ComponentLifetimeManager();

    CMTypes::CmInternalStatus init(CMTypes::ComponentWrapperHandle postId,
                                   const PluginConfig &pluginConfig);
    CMTypes::CmInternalStatus shutdown(CMTypes::ComponentWrapperHandle postId);
    CMTypes::CmInternalStatus deactivateChannel(CMTypes::ComponentWrapperHandle postId,
                                                CMTypes::ChannelSdkHandle handle,
                                                const std::string &channelGid);
    CMTypes::CmInternalStatus activateChannel(CMTypes::ComponentWrapperHandle postId,
                                              CMTypes::ChannelSdkHandle handle,
                                              const std::string &channelGid,
                                              const std::string &roleName);
    CMTypes::CmInternalStatus updateState(CMTypes::ComponentWrapperHandle postId,
                                          const std::string &componentId, ComponentState state);

    EncodingComponentWrapper *encodingComponentFromEncodingParams(const EncodingParameters &params);

    void fail(CMTypes::ComponentWrapperHandle postId);
    void teardown();
    void setup();

protected:
    void checkActivated();

public:
    CMTypes::State state;

    ComponentManagerInternal &manager;
    Composition composition;

    IComponentPlugin &transportPlugin;
    IComponentPlugin &usermodelPlugin;
    std::unordered_map<std::string, IComponentPlugin *> encodingPlugins;

    std::map<std::string, ComponentState> componentStates;
    CMTypes::ChannelSdkHandle activateHandle;

    std::unordered_map<std::string, EncodingComponentWrapper> encodings;
    std::optional<TransportComponentWrapper> transport;
    std::optional<UserModelComponentWrapper> usermodel;
    std::vector<std::unique_ptr<ComponentSdkBaseWrapper>> wrappers;

    std::vector<std::pair<EncodingType, EncodingComponentWrapper *>> encodingsByType;

    std::map<std::string, ComponentBaseWrapper *> idComponentMap;
};

std::ostream &operator<<(std::ostream &out, const ComponentLifetimeManager &manager);
