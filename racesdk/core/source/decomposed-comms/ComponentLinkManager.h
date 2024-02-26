
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

#include <memory>
#include <unordered_map>

#include "ComponentManagerTypes.h"
#include "ComponentWrappers.h"
#include "Composition.h"
#include "IRacePluginComms.h"
#include "LinkProperties.h"
#include "SdkWrappers.h"
#include "plugin-loading/ComponentPlugin.h"

class ComponentManagerInternal;

class ComponentLinkManager {
public:
    explicit ComponentLinkManager(ComponentManagerInternal &manager);
    CMTypes::CmInternalStatus destroyLink(CMTypes::ComponentWrapperHandle postId,
                                          CMTypes::LinkSdkHandle handle, const LinkID &linkId);

    CMTypes::CmInternalStatus createLink(CMTypes::ComponentWrapperHandle postId,
                                         CMTypes::LinkSdkHandle handle,
                                         const std::string &channelGid);

    CMTypes::CmInternalStatus loadLinkAddress(CMTypes::ComponentWrapperHandle postId,
                                              CMTypes::LinkSdkHandle handle,
                                              const std::string &channelGid,
                                              const std::string &linkAddress);

    CMTypes::CmInternalStatus loadLinkAddresses(CMTypes::ComponentWrapperHandle postId,
                                                CMTypes::LinkSdkHandle handle,
                                                const std::string &channelGid,
                                                const std::vector<std::string> &linkAddresses);

    CMTypes::CmInternalStatus createLinkFromAddress(CMTypes::ComponentWrapperHandle postId,
                                                    CMTypes::LinkSdkHandle handle,
                                                    const std::string &channelGid,
                                                    const std::string &linkAddress);

    CMTypes::CmInternalStatus onLinkStatusChanged(CMTypes::ComponentWrapperHandle postId,
                                                  CMTypes::LinkSdkHandle handle,
                                                  const LinkID &linkId, LinkStatus status,
                                                  const LinkParameters &params);

    void teardown();
    void setup();

public:
    std::unordered_map<LinkID, std::unique_ptr<CMTypes::Link>> links;

protected:
    ComponentManagerInternal &manager;
};