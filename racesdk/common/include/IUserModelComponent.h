
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

#ifndef __I_USER_MODEL_COMPONENT_H__
#define __I_USER_MODEL_COMPONENT_H__

#include "ComponentTypes.h"
#include "IUserModelSdk.h"
#include "PluginConfig.h"
#include "RacePluginExports.h"

class IUserModelComponent : public IComponentBase {
public:
    virtual ~IUserModelComponent() = default;

    virtual UserModelProperties getUserModelProperties() = 0;

    // Inform the User Model of a new link and any pertinent characteristics (e.g. multicast vs.
    // unicast)
    virtual ComponentStatus addLink(const LinkID &link, const LinkParameters &params) = 0;

    virtual ComponentStatus removeLink(const LinkID &link) = 0;

    // Return a list of (timestamp, action) tuples corresponding to all the actions to perform
    // between start and end in time
    virtual ActionTimeline getTimeline(Timestamp start, Timestamp end) = 0;

    // Inform the User Model about an external event the transport believes could be relevant to it
    virtual ComponentStatus onTransportEvent(const Event &event) = 0;

    // Inform the User Model sendPackage being called
    // If a returned action contains a timestamp of 0, it is encoded for and executed immediately
    virtual ActionTimeline onSendPackage(const LinkID & /* linkId */, int /* bytes */) {
        return {};
    };
};

extern "C" EXPORT IUserModelComponent *createUserModel(const std::string &name, IUserModelSdk *sdk,
                                                       const std::string &roleName,
                                                       const PluginConfig &pluginConfig);
extern "C" EXPORT void destroyUserModel(IUserModelComponent *component);

#endif
