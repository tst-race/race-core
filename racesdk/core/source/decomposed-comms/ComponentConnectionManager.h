
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
#include "SdkWrappers.h"
#include "plugin-loading/ComponentPlugin.h"

class ComponentManagerInternal;

class ComponentConnectionManager {
public:
    explicit ComponentConnectionManager(ComponentManagerInternal &manager);
    CMTypes::CmInternalStatus openConnection(CMTypes::ComponentWrapperHandle postId,
                                             CMTypes::ConnectionSdkHandle handle, LinkType linkType,
                                             const LinkID &linkId, const std::string &linkHints,
                                             int32_t sendTimeout);

    CMTypes::CmInternalStatus closeConnection(CMTypes::ComponentWrapperHandle postId,
                                              CMTypes::ConnectionSdkHandle handle,
                                              const ConnectionID &connectionId);

    void teardown();
    void setup();

public:
    std::unordered_map<ConnectionID, std::unique_ptr<CMTypes::Connection>> connections;

protected:
    ComponentManagerInternal &manager;
};