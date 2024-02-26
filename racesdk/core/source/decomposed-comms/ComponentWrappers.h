
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

#ifndef __COMPONENT_WRAPPERS_H__
#define __COMPONENT_WRAPPERS_H__

#include "ComponentManagerTypes.h"
#include "Handler.h"
#include "IEncodingComponent.h"
#include "ITransportComponent.h"
#include "IUserModelComponent.h"
#include "helper.h"

class ComponentManager;

class ComponentBaseWrapper {
public:
    virtual void onUserInputReceived(CMTypes::UserComponentHandle handle, bool answered,
                                     const std::string &response) = 0;

    virtual std::string toString() const = 0;
};

std::ostream &operator<<(std::ostream &out, const ComponentBaseWrapper &wrapper);

template <typename ComponentType>
class TemplatedComponentWrapper : public ComponentBaseWrapper {
public:
    explicit TemplatedComponentWrapper(const std::string &channelName,
                                       const std::string &componentName,
                                       std::shared_ptr<ComponentType> component,
                                       ComponentManager *manager);
    virtual ~TemplatedComponentWrapper() {}

    virtual void onUserInputReceived(CMTypes::UserComponentHandle handle, bool answered,
                                     const std::string &response) override;
    virtual std::string toString() const override;

    virtual void waitForCallbacks();

protected:
    template <typename ReturnType, typename T, typename... Args>
    ReturnType post_sync(const std::string &logPrefix, T &&function, Args &&... args);

    template <typename T, typename... Args>
    void post(const std::string &logPrefix, T &&function, Args &&... args);

protected:
    Handler handler;
    std::string channelName;
    std::string componentName;
    std::shared_ptr<ComponentType> component;
    std::atomic<uint64_t> nextPostId = 0;
    ComponentManager *manager;
};

template <typename ComponentType>
std::ostream &operator<<(std::ostream &out,
                         const TemplatedComponentWrapper<ComponentType> &wrapper) {
    return out << wrapper.toString();
}

class TransportComponentWrapper : public TemplatedComponentWrapper<ITransportComponent> {
public:
    explicit TransportComponentWrapper(const std::string &channelName,
                                       const std::string &componentName,
                                       std::shared_ptr<ITransportComponent> component,
                                       ComponentManager *manager);
    virtual ~TransportComponentWrapper() {}

    virtual TransportProperties getTransportProperties();
    virtual LinkProperties getLinkProperties(const LinkID &linkId);

    virtual void createLink(CMTypes::LinkSdkHandle handle, const LinkID &linkId);
    virtual void loadLinkAddress(CMTypes::LinkSdkHandle handle, const LinkID &linkId,
                                 const std::string &linkAddress);
    virtual void loadLinkAddresses(CMTypes::LinkSdkHandle handle, const LinkID &linkId,
                                   const std::vector<std::string> &linkAddress);
    virtual void createLinkFromAddress(CMTypes::LinkSdkHandle handle, const LinkID &linkId,
                                       const std::string &linkAddress);
    virtual void destroyLink(CMTypes::LinkSdkHandle handle, const LinkID &linkId);

    virtual std::vector<EncodingParameters> getActionParams(const Action &action);

    virtual void enqueueContent(const EncodingParameters &params, const Action &action,
                                const std::vector<uint8_t> &content);
    virtual void dequeueContent(const Action &action);

    virtual void doAction(const std::vector<CMTypes::PackageFragmentHandle> &handles,
                          const Action &action);
};

class UserModelComponentWrapper : public TemplatedComponentWrapper<IUserModelComponent> {
public:
    explicit UserModelComponentWrapper(const std::string &channelName,
                                       const std::string &componentName,
                                       std::shared_ptr<IUserModelComponent> component,
                                       ComponentManager *manager);
    virtual ~UserModelComponentWrapper() {}

    virtual UserModelProperties getUserModelProperties();

    virtual void addLink(const LinkID &link, const LinkParameters &params);

    virtual void removeLink(const LinkID &link);

    virtual ActionTimeline getTimeline(Timestamp start, Timestamp end);

    virtual void onTransportEvent(const Event &event);

    virtual ActionTimeline onSendPackage(const LinkID &linkId, int bytes);
};

class EncodingComponentWrapper : public TemplatedComponentWrapper<IEncodingComponent> {
public:
    explicit EncodingComponentWrapper(const std::string &channelName,
                                      const std::string &componentName,
                                      std::shared_ptr<IEncodingComponent> component,
                                      ComponentManager *manager);
    virtual ~EncodingComponentWrapper() {}

    virtual EncodingProperties getEncodingProperties();

    virtual SpecificEncodingProperties getEncodingPropertiesForParameters(
        const EncodingParameters &params);

    virtual void encodeBytes(CMTypes::EncodingHandle handle, const EncodingParameters &params,
                             const std::vector<uint8_t> &bytes);

    virtual void decodeBytes(CMTypes::DecodingHandle handle, const EncodingParameters &params,
                             const std::vector<uint8_t> &bytes);
};

#endif