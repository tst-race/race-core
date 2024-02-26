
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

#ifndef __I_TRANSPORT_COMPONENT_H__
#define __I_TRANSPORT_COMPONENT_H__

#include "ComponentTypes.h"
#include "ITransportSdk.h"
#include "LinkProperties.h"
#include "PluginConfig.h"
#include "RacePluginExports.h"

class ITransportComponent : public IComponentBase {
public:
    virtual ~ITransportComponent() = default;

    virtual TransportProperties getTransportProperties() = 0;
    virtual LinkProperties getLinkProperties(const LinkID &linkId) = 0;

    // Link management
    virtual ComponentStatus createLink(RaceHandle handle, const LinkID &linkId) = 0;
    virtual ComponentStatus loadLinkAddress(RaceHandle handle, const LinkID &linkId,
                                            const std::string &linkAddress) = 0;
    virtual ComponentStatus loadLinkAddresses(RaceHandle handle, const LinkID &linkId,
                                              const std::vector<std::string> &linkAddress) = 0;
    virtual ComponentStatus createLinkFromAddress(RaceHandle handle, const LinkID &linkId,
                                                  const std::string &linkAddress) = 0;
    virtual ComponentStatus destroyLink(RaceHandle handle, const LinkID &linkId) = 0;

    // Message handling
    // Get params necessary for properly encoding content for this action
    virtual std::vector<EncodingParameters> getActionParams(const Action &action) = 0;

    // Enqueue this content for use by this action
    virtual ComponentStatus enqueueContent(const EncodingParameters &params, const Action &action,
                                           const std::vector<uint8_t> &content) = 0;

    virtual ComponentStatus dequeueContent(const Action &action) = 0;

    // Actually execute this action
    virtual ComponentStatus doAction(const std::vector<RaceHandle> &handles,
                                     const Action &action) = 0;
};

extern "C" EXPORT ITransportComponent *createTransport(const std::string &name, ITransportSdk *sdk,
                                                       const std::string &roleName,
                                                       const PluginConfig &pluginConfig);
extern "C" EXPORT void destroyTransport(ITransportComponent *component);

#endif
