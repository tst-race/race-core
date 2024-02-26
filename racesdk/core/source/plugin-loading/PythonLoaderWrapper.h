
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

#ifndef __SOURCE_PYTHON_LOADER_WRAPPER_H__
#define __SOURCE_PYTHON_LOADER_WRAPPER_H__

#include "RaceSdk.h"
#include "filesystem.h"  // fs

static const std::string SDK_TYPE_NM = "IRaceSdkNM*";
static const std::string SDK_TYPE_COMMS = "IRaceSdkComms*";
static const std::string PLUGIN_TYPE_NM = "IRacePluginNM*";
static const std::string PLUGIN_TYPE_COMMS = "IRacePluginComms*";

/**
 * @brief Class for handling loading of a Python plugin. Will initialize the Python interpreter and
 * load a plugin for a given plugin ID.
 *
 * @tparam Parent The Parent wrapper class, either NMWrapper or CommsWrapper.
 */
template <class Parent>
class PythonLoaderWrapper : public Parent {
private:
    /**
     * @brief The plugin interface defined in the Parent class. Must be either IRacePluginNM or
     * IRacePluginComms.
     *
     */
    using Interface = typename Parent::Interface;

    /**
     * @brief The RACE SDK interface defined in the Parent class. Must be either IRaceSdkNM or
     * IRaceSdkComms.
     *
     */
    using SDK = typename Parent::SDK;

public:
    /**
     * @brief Construct a new Python Loader Wrapper object.
     *
     * @param sdk The sdk associated with the plugin.
     * @param pluginDef The plugin definition including all info required to load
     */
    PythonLoaderWrapper(RaceSdk &sdk, const PluginDef &pluginDef);

    // Ensure Parent type has a virtual destructor (required for unique_ptr conversion)
    virtual ~PythonLoaderWrapper();
};

#endif
