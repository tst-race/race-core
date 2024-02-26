
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

#include "PythonLoaderWrapper.h"

#include <Python.h>
#include <RaceLog.h>

#include <algorithm>  // std::find
#include <fstream>
#include <memory>  // std::shared_ptr
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "ArtifactManagerWrapper.h"
#include "CommsWrapper.h"
#include "IEncodingComponent.h"
#include "IRacePluginArtifactManager.h"
#include "IRacePluginComms.h"
#include "IRacePluginNM.h"
#include "ITransportComponent.h"
#include "IUserModelComponent.h"
#include "NMWrapper.h"
#include "PythonLoaderHelper.h"
#include "helper.h"

using json = nlohmann::json;

/**
 * @brief Create a network manager or Comms Python plugin.
 *
 * @param sdk Pointer to the RaceSdk instance associated with the plugin.
 * @return void* Pointer to the plugin, or nullptr on failure.
 */
static void *createPythonPlugin(void *sdk, const std::string &pythonModule,
                                const std::string &pythonClass, std::string raceSdkType,
                                std::string racePluginType,
#ifdef __ANDROID__
                                const std::string &androidPythonPath
#else
                                const std::string &
#endif
) {
    static const std::string pluginNameForLogging = "RaceSdkCore: createPythonPlugin";
    RaceLog::logInfo(pluginNameForLogging, "called", "");

    if (sdk == nullptr) {
        RaceLog::logError(pluginNameForLogging, "RaceSdk pointer is nullptr", "");
        return nullptr;
    }

// TODO: Ideally we read configuration to know what plugins we should be using and dynamically
// load them, however, we have hardcoded it to utilize a specific library in the makefile.

// Initialize the Python interpreter. Must be called before any other Python API calls are made.
// Also initializes the GIL (Global Interpreter Lock) as of Python 3.7. The call is idempotent,
// i.e. it's a no-op when called for a second time, so we don't have to worry about subsequent
// Python plugins calling this API again.
// https://docs.python.org/3.7/c-api/init.html#c.Py_Initialize
#ifdef __ANDROID__
    // Android cannot get environment variables so Python Home and Path must be set here
    std::wstring pythonHome(L"/data/data/com.twosix.race/python3.7/");
    Py_SetPythonHome(pythonHome.c_str());
    // TODO: Python dependencies have been built/pushed to artifactory manually. When that is
    // updated to build all packages automatically, they should all be installed in the python3.7
    // dir so that the python path doesn't have to change for each new package
    std::wstring pythonPath(androidPythonPath.begin(), androidPythonPath.end());
    Py_SetPath(pythonPath.c_str());
#endif
    Py_Initialize();

    RaceLog::logInfo(pluginNameForLogging, "Python version: " + std::string(Py_GetVersion()), "");

    // Ensure that the current thread is ready to call the Python C API regardless of the current
    // state of Python, or of the global interpreter lock. This may be called as many times as
    // desired by a thread as long as each call is matched with a call to PyGILState_Release().
    // https://docs.python.org/3.7/c-api/init.html#c.PyGILState_Ensure
    PyGILState_STATE gstate = PyGILState_Ensure();

    // Import the Python plugin module;
    // https://docs.python.org/3.7/c-api/import.html#c.PyImport_ImportModule
    PyObject *new_pModule = PyImport_ImportModule(pythonModule.c_str());
    PythonLoaderHelper::checkForPythonError();

    // Get the Python plugin class from the module.
    // https://docs.python.org/3.7/c-api/object.html#c.PyObject_GetAttrString
    PyObject *new_plugin = PyObject_GetAttrString(new_pModule, pythonClass.c_str());
    Py_DECREF(new_pModule);
    if (!new_plugin || !PyCallable_Check(new_plugin)) {
        RaceLog::logError(pluginNameForLogging, "Cannot find plugin.", "");
        PythonLoaderHelper::checkForPythonError();
        PyGILState_Release(gstate);
        return nullptr;
    }

    // Set the Python wrapped SDK object as a Python argument.
    PyObject *new_pArgs = PyTuple_New(1);

    // Convert/wrap the SDK object for python using SWIG bindings.
    PyTuple_SetItem(new_pArgs, 0, SWIG_NewPointerObj(sdk, SWIG_TypeQuery(raceSdkType.c_str()), 0));

    // Construct the Python plugin object.
    // https://docs.python.org/3.7/c-api/object.html#c.PyObject_CallObject
    PyObject *new_instance = PyObject_CallObject(new_plugin, new_pArgs);
    Py_DECREF(new_plugin);
    Py_DECREF(new_pArgs);
    if (new_instance == nullptr) {
        RaceLog::logError(pluginNameForLogging, "PyObject_CallObject returned nullptr", "");
        PythonLoaderHelper::checkForPythonError();
        PyGILState_Release(gstate);
        return nullptr;
    }

    // Convert/wrap the Python plugin for C++ using SWIG bindings.
    void *pythonPluginCpp = nullptr;
    swig_type_info *pTypeInfo = SWIG_TypeQuery(racePluginType.c_str());

    const int res = SWIG_ConvertPtr(new_instance, &pythonPluginCpp, pTypeInfo,
                                    SWIG_POINTER_DISOWN);  // steal reference
    if (!SWIG_IsOK(res)) {
        RaceLog::logError(pluginNameForLogging,
                          "Failed to convert pointer to " + racePluginType +
                              ". ptypeinfo = " + std::string{pTypeInfo->name},
                          "");
        Py_DECREF(new_instance);
        PyGILState_Release(gstate);
        return nullptr;
    }

    // Release any acquired resources now that we are done calling Python APIs. This is the matching
    // call to the call to PyGILState_Ensure() above.
    // https://docs.python.org/3.7/c-api/init.html#c.PyGILState_Release
    PyGILState_Release(gstate);

    PythonLoaderHelper::savePythonThread();

    RaceLog::logInfo(pluginNameForLogging, "returning", "");
    return pythonPluginCpp;
}

template <class Plugin>
static std::shared_ptr<Plugin> createPythonPluginSharedPtr(void *sdk, const PluginDef &pluginDef,
                                                           const std::string &androidPythonPath);

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

template <>
std::shared_ptr<IRacePluginArtifactManager> createPythonPluginSharedPtr(
    void * /*sdk*/, const PluginDef & /*pluginDef*/, const std::string & /*androidPythonPath*/) {
    std::string message = "PythonLoaderWrapper: Python not supported for ArtifactManager plugins";
    helper::logError(message);
    throw std::runtime_error(message);
}

static void destroyPythonNMPlugin(void *obj) {
    PythonLoaderHelper::destroyPythonPlugin(&obj, PLUGIN_TYPE_NM);
}

template <>
std::shared_ptr<IRacePluginNM> createPythonPluginSharedPtr(void *sdk, const PluginDef &pluginDef,
                                                           const std::string &androidPythonPath) {
    return std::shared_ptr<IRacePluginNM>(static_cast<IRacePluginNM *>(createPythonPlugin(
                                              sdk, pluginDef.pythonModule, pluginDef.pythonClass,
                                              SDK_TYPE_NM, PLUGIN_TYPE_NM, androidPythonPath)),
                                          destroyPythonNMPlugin);
}

static void destroyPythonCommsPlugin(void *obj) {
    PythonLoaderHelper::destroyPythonPlugin(&obj, PLUGIN_TYPE_COMMS);
}

template <>
std::shared_ptr<IRacePluginComms> createPythonPluginSharedPtr(
    void *sdk, const PluginDef &pluginDef, const std::string &androidPythonPath) {
    return std::shared_ptr<IRacePluginComms>(
        static_cast<IRacePluginComms *>(createPythonPlugin(sdk, pluginDef.pythonModule,
                                                           pluginDef.pythonClass, SDK_TYPE_COMMS,
                                                           PLUGIN_TYPE_COMMS, androidPythonPath)),
        destroyPythonCommsPlugin);
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

template <class Parent>
PythonLoaderWrapper<Parent>::PythonLoaderWrapper(RaceSdk &sdk, const PluginDef &pluginDef) :
    Parent(sdk, pluginDef.filePath) {
    helper::logDebug("PythonLoaderWrapper: called");

    this->mPlugin = createPythonPluginSharedPtr<Interface>(this->getSdk(), pluginDef,
                                                           sdk.getRaceConfig().androidPythonPath);
    if (!this->mPlugin) {
        const std::string errorMessage = "PythonLoaderWrapper: failed to create plugin";
        helper::logError(errorMessage);
        throw std::runtime_error(errorMessage);
    }
    this->mId = pluginDef.filePath;
    this->mDescription = pluginDef.filePath;
    this->mConfigPath = pluginDef.configPath;

    helper::logDebug("PythonLoaderWrapper: returned");
}

template <class Parent>
PythonLoaderWrapper<Parent>::~PythonLoaderWrapper() {
    helper::logDebug("PythonLoaderWrapper::~PythonLoaderWrapper: called");
    this->mPlugin.reset();
    helper::logDebug("PythonLoaderWrapper::~PythonLoaderWrapper: returned");
}

// Explicit template instantiation. This allows class function definitions to be defined in the cpp
// file. Furthermore, no values should ever be used for this template other than NMWrapper and
// CommsWrapper, so this adds a compile time check to verify that.
template class PythonLoaderWrapper<NMWrapper>;
template class PythonLoaderWrapper<CommsWrapper>;
template class PythonLoaderWrapper<ArtifactManagerWrapper>;
