
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

#ifndef __ICOMPONENT_PLUGIN_H__
#define __ICOMPONENT_PLUGIN_H__

#include <memory.h>

#include "IEncodingComponent.h"
#include "ITransportComponent.h"
#include "IUserModelComponent.h"
#include "PluginConfig.h"

struct IComponentPlugin {
public:
    virtual ~IComponentPlugin() = default;

    virtual std::shared_ptr<ITransportComponent> createTransport(std::string name,
                                                                 ITransportSdk *sdk,
                                                                 std::string roleName,
                                                                 PluginConfig pluginConfig) = 0;
    virtual std::shared_ptr<IUserModelComponent> createUserModel(std::string name,
                                                                 IUserModelSdk *sdk,
                                                                 std::string roleName,
                                                                 PluginConfig pluginConfig) = 0;
    virtual std::shared_ptr<IEncodingComponent> createEncoding(std::string name, IEncodingSdk *sdk,
                                                               std::string roleName,
                                                               PluginConfig pluginConfig) = 0;
    virtual std::string get_path() = 0;
};

#endif
