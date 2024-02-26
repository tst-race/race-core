
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

#include "ComponentWrappers.h"

#include "ComponentManager.h"

std::ostream &operator<<(std::ostream &out, const ComponentBaseWrapper &wrapper) {
    return out << wrapper.toString();
}

template <typename ComponentType>
template <typename T, typename... Args>
void TemplatedComponentWrapper<ComponentType>::post(const std::string &logPrefix, T &&function,
                                                    Args &&... args) {
    std::string postId = std::to_string(nextPostId++);
    helper::logDebug(logPrefix + "Posting postId: " + postId);

    try {
        auto workFunc = [logPrefix, postId, function, this](auto &&... args) mutable {
            helper::logDebug(logPrefix + "Calling postId: " + postId);
            ComponentStatus response;
            try {
                response = std::mem_fn(function)(*component, std::move(args)...);
            } catch (std::exception &e) {
                helper::logError(logPrefix + "Threw exception: " + std::string(e.what()));
                response = COMPONENT_FATAL;
            } catch (...) {
                helper::logError(logPrefix + "Threw unknown exception");
                response = COMPONENT_FATAL;
            }
            helper::logDebug(logPrefix + "Returned " + componentStatusToString(response) +
                             ", postId: " + postId);

            if (response != COMPONENT_OK) {
                helper::logError(logPrefix + "Returned " + componentStatusToString(response) +
                                 ", postId: " + postId);
                PluginResponse errorStatus = PLUGIN_ERROR;
                if (response == COMPONENT_FATAL) {
                    errorStatus = PLUGIN_FATAL;
                    // mark the manager as failed so future calls also fail
                    manager->markFailed();
                }
                manager->sdk.asyncError(NULL_RACE_HANDLE, errorStatus);
            }

            return std::make_optional(response);
        };
        auto [success, queueSize, future] =
            handler.post("", 0, -1, std::bind(std::move(workFunc), std::forward<Args>(args)...));

        // this really shouldn't happen...
        if (success == Handler::PostStatus::INVALID_STATE) {
            helper::logError(logPrefix + "Post " + postId + " failed with error INVALID_STATE");
        } else if (success == Handler::PostStatus::QUEUE_FULL) {
            helper::logError(logPrefix + "Post " + postId + " failed with error QUEUE_FULL");
        } else if (success == Handler::PostStatus::HANDLER_FULL) {
            helper::logError(logPrefix + "Post " + postId + " failed with error HANDLER_FULL");
        }

        (void)queueSize;
        (void)future;
    } catch (std::out_of_range &error) {
        helper::logError("default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    }
}

template <typename ComponentType>
template <typename ReturnType, typename T, typename... Args>
ReturnType TemplatedComponentWrapper<ComponentType>::post_sync(const std::string &logPrefix,
                                                               T &&function, Args &&... args) {
    std::string postId = std::to_string(nextPostId++);
    helper::logDebug(logPrefix + "Posting postId: " + postId);

    try {
        auto workFunc = [logPrefix, postId, function, this](auto &&... args) mutable {
            helper::logDebug(logPrefix + "Calling postId: " + postId);
            try {
                return std::make_optional(std::mem_fn(function)(*component, std::move(args)...));
            } catch (std::exception &e) {
                helper::logError(logPrefix + "Threw exception: " + std::string(e.what()));
                manager->markFailed();
                manager->sdk.asyncError(NULL_RACE_HANDLE, PLUGIN_FATAL);
            } catch (...) {
                helper::logError(logPrefix + "Threw unknown exception");
                manager->markFailed();
                manager->sdk.asyncError(NULL_RACE_HANDLE, PLUGIN_FATAL);
            }

            return std::make_optional(ReturnType{});
        };
        auto [success, queueSize, future] =
            handler.post("", 0, -1, std::bind(std::move(workFunc), std::forward<Args>(args)...));

        // this really shouldn't happen...
        if (success == Handler::PostStatus::INVALID_STATE) {
            helper::logError(logPrefix + "Post " + postId + " failed with error INVALID_STATE");
        } else if (success == Handler::PostStatus::QUEUE_FULL) {
            helper::logError(logPrefix + "Post " + postId + " failed with error QUEUE_FULL");
        } else if (success == Handler::PostStatus::HANDLER_FULL) {
            helper::logError(logPrefix + "Post " + postId + " failed with error HANDLER_FULL");
        }

        (void)queueSize;
        future.wait();
        return future.get();
    } catch (std::out_of_range &error) {
        helper::logError("default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
        return {};
    }
}

////////////////////////////////////////////////////////////
// Base
////////////////////////////////////////////////////////////

template <typename ComponentType>
TemplatedComponentWrapper<ComponentType>::TemplatedComponentWrapper(
    const std::string &channelName, const std::string &componentName,
    std::shared_ptr<ComponentType> component, ComponentManager *manager) :
    handler(channelName + "-" + componentName + "-thread", 1 << 20, 1 << 20),
    component(std::move(component)),
    manager(manager) {
    handler.create_queue("wait queue", std::numeric_limits<int>::min());
    handler.start();
}

template <typename ComponentType>
void TemplatedComponentWrapper<ComponentType>::onUserInputReceived(
    CMTypes::UserComponentHandle handle, bool answered, const std::string &response) {
    TRACE_METHOD(handle, answered, response);
    post(logPrefix, &IComponentBase::onUserInputReceived, handle.handle, answered, response);
}

template <>
std::string TemplatedComponentWrapper<IEncodingComponent>::toString() const {
    return "<EncodingComponentWrapper>";
}

template <>
std::string TemplatedComponentWrapper<ITransportComponent>::toString() const {
    return "<TransportComponentWrapper>";
}

template <>
std::string TemplatedComponentWrapper<IUserModelComponent>::toString() const {
    return "<UserModelComponentWrapper>";
}

// template <typename ComponentType>
// void TemplatedComponentWrapper<ComponentType>::waitForCallbacks()

template <typename ComponentType>
void TemplatedComponentWrapper<ComponentType>::waitForCallbacks() {
    auto [success, queueSize, future] =
        handler.post("wait queue", 0, -1, [=] { return std::make_optional(true); });
    (void)success;
    (void)queueSize;
    future.wait();
}

////////////////////////////////////////////////////////////
// Transport
////////////////////////////////////////////////////////////

TransportComponentWrapper::TransportComponentWrapper(const std::string &channelName,
                                                     const std::string &componentName,
                                                     std::shared_ptr<ITransportComponent> component,
                                                     ComponentManager *manager) :
    TemplatedComponentWrapper(channelName, componentName, component, manager) {}

TransportProperties TransportComponentWrapper::getTransportProperties() {
    TRACE_METHOD();
    return post_sync<TransportProperties>(logPrefix, &ITransportComponent::getTransportProperties);
}

LinkProperties TransportComponentWrapper::getLinkProperties(const LinkID &linkId) {
    TRACE_METHOD(linkId);
    return post_sync<LinkProperties>(logPrefix, &ITransportComponent::getLinkProperties, linkId);
}

void TransportComponentWrapper::createLink(CMTypes::LinkSdkHandle handle, const LinkID &linkId) {
    TRACE_METHOD(handle, linkId);
    post(logPrefix, &ITransportComponent::createLink, handle.handle, linkId);
}

void TransportComponentWrapper::loadLinkAddress(CMTypes::LinkSdkHandle handle, const LinkID &linkId,
                                                const std::string &linkAddress) {
    TRACE_METHOD(handle, linkId, linkAddress);
    post(logPrefix, &ITransportComponent::loadLinkAddress, handle.handle, linkId, linkAddress);
}

void TransportComponentWrapper::loadLinkAddresses(CMTypes::LinkSdkHandle handle,
                                                  const LinkID &linkId,
                                                  const std::vector<std::string> &linkAddress) {
    TRACE_METHOD(handle, linkId, linkAddress.size());
    post(logPrefix, &ITransportComponent::loadLinkAddresses, handle.handle, linkId, linkAddress);
}

void TransportComponentWrapper::createLinkFromAddress(CMTypes::LinkSdkHandle handle,
                                                      const LinkID &linkId,
                                                      const std::string &linkAddress) {
    TRACE_METHOD(handle, linkId, linkAddress);
    post(logPrefix, &ITransportComponent::createLinkFromAddress, handle.handle, linkId,
         linkAddress);
}

void TransportComponentWrapper::destroyLink(CMTypes::LinkSdkHandle handle, const LinkID &linkId) {
    TRACE_METHOD(handle, linkId);
    post(logPrefix, &ITransportComponent::destroyLink, handle.handle, linkId);
}

std::vector<EncodingParameters> TransportComponentWrapper::getActionParams(const Action &action) {
    TRACE_METHOD(action);
    return post_sync<std::vector<EncodingParameters>>(
        logPrefix, &ITransportComponent::getActionParams, action);
}

void TransportComponentWrapper::enqueueContent(const EncodingParameters &params,
                                               const Action &action,
                                               const std::vector<uint8_t> &content) {
    TRACE_METHOD(params, action, content.size());
    post(logPrefix, &ITransportComponent::enqueueContent, params, action, content);
}

void TransportComponentWrapper::dequeueContent(const Action &action) {
    TRACE_METHOD(action);
    post(logPrefix, &ITransportComponent::dequeueContent, action);
}

void TransportComponentWrapper::doAction(const std::vector<CMTypes::PackageFragmentHandle> &handles,
                                         const Action &action) {
    std::vector<RaceHandle> raceHandles(handles.size());
    std::transform(handles.begin(), handles.end(), raceHandles.begin(),
                   [](const auto &handle) { return handle.handle; });
    nlohmann::json handlesJson = raceHandles;
    TRACE_METHOD(handlesJson, action);
    post(logPrefix, &ITransportComponent::doAction, raceHandles, action);
}

////////////////////////////////////////////////////////////
// UserModel
////////////////////////////////////////////////////////////

UserModelComponentWrapper::UserModelComponentWrapper(const std::string &channelName,
                                                     const std::string &componentName,
                                                     std::shared_ptr<IUserModelComponent> component,
                                                     ComponentManager *manager) :
    TemplatedComponentWrapper(channelName, componentName, component, manager) {}

UserModelProperties UserModelComponentWrapper::getUserModelProperties() {
    TRACE_METHOD();
    return post_sync<UserModelProperties>(logPrefix, &IUserModelComponent::getUserModelProperties);
}

void UserModelComponentWrapper::addLink(const LinkID &linkId, const LinkParameters &params) {
    TRACE_METHOD(linkId, params);
    post(logPrefix, &IUserModelComponent::addLink, linkId, params);
}

void UserModelComponentWrapper::removeLink(const LinkID &linkId) {
    TRACE_METHOD(linkId);
    post(logPrefix, &IUserModelComponent::removeLink, linkId);
}

ActionTimeline UserModelComponentWrapper::getTimeline(Timestamp start, Timestamp end) {
    TRACE_METHOD(start, end);
    return post_sync<ActionTimeline>(logPrefix, &IUserModelComponent::getTimeline, start, end);
}

void UserModelComponentWrapper::onTransportEvent(const Event &event) {
    TRACE_METHOD(event);
    post(logPrefix, &IUserModelComponent::onTransportEvent, event);
}

ActionTimeline UserModelComponentWrapper::onSendPackage(const LinkID &linkId, int bytes) {
    TRACE_METHOD(linkId, bytes);
    return post_sync<ActionTimeline>(logPrefix, &IUserModelComponent::onSendPackage, linkId, bytes);
}

////////////////////////////////////////////////////////////
// Encoding
////////////////////////////////////////////////////////////

EncodingComponentWrapper::EncodingComponentWrapper(const std::string &channelName,
                                                   const std::string &componentName,
                                                   std::shared_ptr<IEncodingComponent> component,
                                                   ComponentManager *manager) :
    TemplatedComponentWrapper(channelName, componentName, component, manager) {}

EncodingProperties EncodingComponentWrapper::getEncodingProperties() {
    TRACE_METHOD();
    return post_sync<EncodingProperties>(logPrefix, &IEncodingComponent::getEncodingProperties);
}

SpecificEncodingProperties EncodingComponentWrapper::getEncodingPropertiesForParameters(
    const EncodingParameters &params) {
    TRACE_METHOD(params);
    return post_sync<SpecificEncodingProperties>(
        logPrefix, &IEncodingComponent::getEncodingPropertiesForParameters, params);
}

void EncodingComponentWrapper::encodeBytes(CMTypes::EncodingHandle handle,
                                           const EncodingParameters &params,
                                           const std::vector<uint8_t> &bytes) {
    TRACE_METHOD(handle, params, bytes.size());
    post(logPrefix, &IEncodingComponent::encodeBytes, handle.handle, params, bytes);
}

void EncodingComponentWrapper::decodeBytes(CMTypes::DecodingHandle handle,
                                           const EncodingParameters &params,
                                           const std::vector<uint8_t> &bytes) {
    TRACE_METHOD(handle, params, bytes.size());
    post(logPrefix, &IEncodingComponent::decodeBytes, handle.handle, params, bytes);
}