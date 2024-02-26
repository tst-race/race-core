
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

#ifndef __PYTHON_COMPOSITE_PLUGIN_H__
#define __PYTHON_COMPOSITE_PLUGIN_H__

#include <Python.h>
#include <RaceLog.h>
#include <memory.h>

// Need this for Python bindings?
#include <nlohmann/json.hpp>

#include "IComponentPlugin.h"
#include "IEncodingComponent.h"
#include "ITransportComponent.h"
#include "IUserModelComponent.h"
#include "PluginConfig.h"
#include "PythonLoaderHelper.h"
#include "helper.h"

static const std::string SDK_TYPE_TRANSPORT = "ITransportSdk*";
static const std::string SDK_TYPE_ENCODING = "IEncodingSdk*";
static const std::string SDK_TYPE_USER_MODEL = "IUserModelSdk*";

static const std::string PLUGIN_TYPE_TRANSPORT = "ITransportComponent*";
static const std::string PLUGIN_TYPE_ENCODING = "IEncodingComponent*";
static const std::string PLUGIN_TYPE_USER_MODEL = "IUserModelComponent*";

static const std::string FUNC_CREATE_TRANSPORT = "createTransport";
static const std::string FUNC_CREATE_USER_MODEL = "createUserModel";
static const std::string FUNC_CREATE_ENCODING = "createEncoding";

static const std::string ARG_PLUGIN_CONFIG = "PluginConfig*";

struct PythonComponentPlugin : public IComponentPlugin {
public:
    explicit PythonComponentPlugin(const std::string &path, const std::string &pythonModule);
    virtual ~PythonComponentPlugin() {}

    virtual std::shared_ptr<ITransportComponent> createTransport(
        std::string name, ITransportSdk *sdk, std::string roleName,
        PluginConfig pluginConfig) override;
    virtual std::shared_ptr<IUserModelComponent> createUserModel(
        std::string name, IUserModelSdk *sdk, std::string roleName,
        PluginConfig pluginConfig) override;
    virtual std::shared_ptr<IEncodingComponent> createEncoding(std::string name, IEncodingSdk *sdk,
                                                               std::string roleName,
                                                               PluginConfig pluginConfig) override;

    virtual std::string get_path() override;

private:
    std::string path;
    std::string pythonModule;
};
#endif
