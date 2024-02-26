
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

#include "ComponentLifetimeManager.h"

#include "ComponentManager.h"
#include "MimeTypes.h"

using namespace CMTypes;

ComponentLifetimeManager::ComponentLifetimeManager(
    ComponentManagerInternal &manager, const Composition &composition,
    IComponentPlugin &transportPlugin, IComponentPlugin &usermodelPlugin,
    const std::unordered_map<std::string, IComponentPlugin *> &encodingPlugins) :
    state(CMTypes::State::INITIALIZING),
    manager(manager),
    composition(composition),
    transportPlugin(transportPlugin),
    usermodelPlugin(usermodelPlugin),
    encodingPlugins(encodingPlugins),
    activateHandle{NULL_RACE_HANDLE} {}

ComponentLifetimeManager::~ComponentLifetimeManager() {
    TRACE_METHOD();
    if (transport || usermodel || encodings.size() > 0) {
        helper::logError(logPrefix + "Destroyed but teardown was not called first, aborting");
        std::terminate();
    }
}

CMTypes::CmInternalStatus ComponentLifetimeManager::init(ComponentWrapperHandle postId,
                                                         const PluginConfig & /* pluginConfig */) {
    TRACE_METHOD(postId);
    state = CMTypes::State::UNACTIVATED;
    return OK;
}

CMTypes::CmInternalStatus ComponentLifetimeManager::shutdown(ComponentWrapperHandle postId) {
    TRACE_METHOD(postId);
    if (transport || usermodel || encodings.size() > 0) {
        manager.deactivateChannel(postId, {NULL_RACE_HANDLE}, composition.id);
    }
    state = CMTypes::State::SHUTTING_DOWN;
    return OK;
}

CMTypes::CmInternalStatus ComponentLifetimeManager::deactivateChannel(
    ComponentWrapperHandle postId, ChannelSdkHandle handle, const std::string &channelGid) {
    TRACE_METHOD(postId, handle, channelGid);
    manager.teardown();
    state = CMTypes::State::UNACTIVATED;
    manager.sdk.onChannelStatusChanged(handle.handle, channelGid, CHANNEL_ENABLED, {},
                                       RACE_BLOCKING);
    return OK;
}

CMTypes::CmInternalStatus ComponentLifetimeManager::activateChannel(ComponentWrapperHandle postId,
                                                                    ChannelSdkHandle handle,
                                                                    const std::string &channelGid,
                                                                    const std::string &roleName) {
    TRACE_METHOD(postId, handle, channelGid, roleName);
    activateHandle = handle;
    state = CMTypes::State::CREATING_COMPONENTS;

    componentStates[composition.transport] = COMPONENT_STATE_INIT;
    auto transportSdkWrapper =
        std::make_unique<TransportSdkWrapper>(*manager.manager, composition.transport);
    auto transportComponent = transportPlugin.createTransport(
        composition.transport, transportSdkWrapper.get(), roleName, manager.pluginConfig);
    if (transportComponent == nullptr) {
        helper::logError(logPrefix + "Failed to create transport '" + composition.transport + "'");
        return FATAL;
    }
    transport.emplace(composition.id, composition.transport, std::move(transportComponent),
                      manager.manager);

    idComponentMap[composition.transport] = &*transport;
    wrappers.push_back(std::move(transportSdkWrapper));

    componentStates[composition.usermodel] = COMPONENT_STATE_INIT;
    auto usermodelSdkWrapper =
        std::make_unique<UserModelSdkWrapper>(*manager.manager, composition.usermodel);
    auto usermodelComponent = usermodelPlugin.createUserModel(
        composition.usermodel, usermodelSdkWrapper.get(), roleName, manager.pluginConfig);
    if (usermodelComponent == nullptr) {
        helper::logError(logPrefix + "Failed to create usermodel '" + composition.usermodel + "'");
        return FATAL;
    }
    usermodel.emplace(composition.id, composition.usermodel, std::move(usermodelComponent),
                      manager.manager);
    idComponentMap[composition.usermodel] = &*usermodel;
    wrappers.push_back(std::move(usermodelSdkWrapper));

    for (auto encodingPlugin : encodingPlugins) {
        auto name = encodingPlugin.first;
        componentStates[name] = COMPONENT_STATE_INIT;
        auto encodingSdkWrapper = std::make_unique<EncodingSdkWrapper>(*manager.manager, name);
        auto encodingComponent = encodingPlugin.second->createEncoding(
            name, encodingSdkWrapper.get(), roleName, manager.pluginConfig);
        if (encodingComponent == nullptr) {
            helper::logError(logPrefix + "Failed to create encoding '" + name + "'");
            return FATAL;
        }

        auto nameTup = std::make_tuple(name);
        auto wrapperTup =
            std::make_tuple(composition.id, name, std::move(encodingComponent), manager.manager);

        auto [it, inserted] =
            encodings.emplace(std::piecewise_construct, nameTup, std::move(wrapperTup));
        idComponentMap[name] = &it->second;
        wrappers.push_back(std::move(encodingSdkWrapper));
    }

    // Wait till all components call updateState before continuing
    state = CMTypes::State::WAITING_FOR_COMPONENTS;
    checkActivated();
    return OK;
}

CMTypes::CmInternalStatus ComponentLifetimeManager::updateState(ComponentWrapperHandle postId,
                                                                const std::string &componentId,
                                                                ComponentState updatedState) {
    TRACE_METHOD(postId, componentId);

    componentStates[componentId] = updatedState;

    if (updatedState == COMPONENT_STATE_FAILED) {
        helper::logError(logPrefix + "Component " + componentId + " failed");
        fail(postId);
        return OK;
    } else if (updatedState == COMPONENT_STATE_STARTED) {
        checkActivated();
    }

    return OK;
}

void ComponentLifetimeManager::fail(ComponentWrapperHandle postId) {
    TRACE_METHOD(postId);
    helper::logError(logPrefix + "Tearing down Component Manager after failure");
    manager.teardown();
    state = CMTypes::State::FAILED;
    manager.sdk.onChannelStatusChanged(activateHandle.handle, composition.id, CHANNEL_FAILED, {},
                                       RACE_BLOCKING);
}

void ComponentLifetimeManager::checkActivated() {
    TRACE_METHOD();
    if (state != CMTypes::State::WAITING_FOR_COMPONENTS) {
        return;
    }

    bool all_started = std::all_of(
        componentStates.begin(), componentStates.end(),
        [](auto componentState) { return componentState.second == COMPONENT_STATE_STARTED; });

    if (all_started) {
        state = CMTypes::State::ACTIVATED;
        manager.setup();
        manager.sdk.onChannelStatusChanged(activateHandle.handle, composition.id, CHANNEL_AVAILABLE,
                                           manager.channelProps, RACE_BLOCKING);
    }
}

EncodingComponentWrapper *ComponentLifetimeManager::encodingComponentFromEncodingParams(
    const EncodingParameters &params) {
    for (auto &encoding : encodingsByType) {
        if (mimeTypeMatches(encoding.first, params.type)) {
            return encoding.second;
        }
    }

    return nullptr;
}

void ComponentLifetimeManager::teardown() {
    TRACE_METHOD();

    transport.reset();
    usermodel.reset();
    encodingsByType.clear();
    encodings.clear();
    wrappers.clear();
    componentStates.clear();
    idComponentMap.clear();
}

void ComponentLifetimeManager::setup() {
    for (auto &encoding : encodings) {
        auto props = encoding.second.getEncodingProperties();
        encodingsByType.emplace_back(props.type, &encoding.second);
    }

    TRACE_METHOD();
}

std::ostream &operator<<(std::ostream &out,
                         const std::optional<TransportComponentWrapper> &component) {
    if (component.has_value()) {
        return out << *component;
    } else {
        return out << "nullopt";
    }
}

std::ostream &operator<<(std::ostream &out,
                         const std::optional<UserModelComponentWrapper> &component) {
    if (component.has_value()) {
        return out << *component;
    } else {
        return out << "nullopt";
    }
}

std::ostream &operator<<(std::ostream &out,
                         const std::optional<EncodingComponentWrapper> &component) {
    if (component.has_value()) {
        return out << *component;
    } else {
        return out << "nullopt";
    }
}

std::ostream &operator<<(
    std::ostream &out,
    const std::vector<std::pair<EncodingType, EncodingComponentWrapper *>> &encodingsByType) {
    out << "{";
    for (auto &pair : encodingsByType) {
        out << "{" << pair.first << ", " << *pair.second << "}, ";
    }
    out << "}";
    return out;
}

std::ostream &operator<<(
    std::ostream &out, const std::unordered_map<std::string, EncodingComponentWrapper> &encodings) {
    out << "{";
    for (auto &iter : encodings) {
        out << iter.first << ":" << iter.second << ", ";
    }
    out << "}";
    return out;
}

std::ostream &operator<<(std::ostream &out,
                         const std::vector<std::unique_ptr<ComponentSdkBaseWrapper>> &wrappers) {
    out << "{";
    for (auto &ptr : wrappers) {
        out << *ptr << ", ";
    }
    out << "}";
    return out;
}

std::ostream &operator<<(std::ostream &out,
                         const std::map<std::string, ComponentState> &componentStates) {
    out << "{";
    for (auto &iter : componentStates) {
        out << iter.first << ":" << iter.second << ", ";
    }
    out << "}";
    return out;
}

std::ostream &operator<<(std::ostream &out,
                         const std::map<std::string, ComponentBaseWrapper *> &idComponentMap) {
    out << "{";
    for (auto &iter : idComponentMap) {
        out << iter.first << ":" << *iter.second << ", ";
    }
    out << "}";
    return out;
}

std::ostream &operator<<(std::ostream &out, const ComponentLifetimeManager &manager) {
    return out << "LifetimeManager{"
               << "state:" << manager.state << ", transport: " << manager.transport
               << ", usermodel: " << manager.usermodel
               << ", encodingsByType: " << manager.encodingsByType
               << ", encodings: " << manager.encodings << ", wrappers: " << manager.wrappers
               << ", componentStates: " << manager.componentStates
               << ", idComponentMap: " << manager.idComponentMap << "}";
}