
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

#include "PythonComponentPlugin.h"

static void *createPythonComponentPlugin(const std::string &createFunction, std::string &pythonPath,
                                         std::string &pythonModule, const std::string &sdkType,
                                         const std::string &pluginType, std::string &name,
                                         void *sdk, const std::string &roleName,
                                         PluginConfig pluginConfig) {
    TRACE_FUNCTION(createFunction, pythonPath, pythonModule, sdkType, pluginType, name, roleName);

    /**
     * NOTE:
     *
     * The approach that is being followed for management of the python
     * objects is the following:
     *
     * - ensuring that the reference counts for newly allocated python objects
     *   are decremented after use
     *
     * - preserving references to objects that are more long lived by disowning
     *   and re-owning references as appropriate.
     *
     * However we have not been able to verify that the actual destruction of
     * the python component object occurs since that would be handled by the
     * garbage collector.
     *
     * Bottom line is that while we feel that references are handled properly,
     * there is an outside possibility that memory is still being held and not
     * being fully reclaimed.
     */

#ifdef __ANDROID__
    // Android cannot get environment variables so Python Home and Path must be set here
    std::wstring pythonHome(L"/data/data/com.twosix.race/python3.7/");
    Py_SetPythonHome(pythonHome.c_str());
    // TODO: Python dependencies have been built/pushed to artifactory manually. When that is
    // updated to build all packages automatically, they should all be installed in the python3.7
    // dir so that the python path doesn't have to change for each new package
    std::wstring pythonPathAndroid(pythonPath.begin(), pythonPath.end());
    Py_SetPath(pythonPathAndroid.c_str());
#endif

    // Py_Initialize() could be called twice - once for loading a unified plugin/network manager
    // plugin and once for the decomposed plugin. It is a no-op if called a
    // second time from
    // https://docs.python.org/3.7/c-api/init.html#c.Py_Initialize
    Py_Initialize();
    RaceLog::logInfo(logPrefix, "Python version: " + std::string(Py_GetVersion()), "");

    // Ensure that the current thread is ready to call the Python C API regardless of
    // the current state of Python, or of the global interpreter lock. This may be
    // called as many times as desired by a thread as long as each call is matched with
    // a call to PyGILState_Release().
    // https://docs.python.org/3.7/c-api/init.html#c.PyGILState_Ensure
    PyGILState_STATE gstate = PyGILState_Ensure();

    // Import the Python plugin module;
    // https://docs.python.org/3.7/c-api/import.html#c.PyImport_ImportModule
    PyObject *new_pModule = PyImport_ImportModule(pythonModule.c_str());
    PythonLoaderHelper::checkForPythonError();
    // Get the Python plugin class from the module.
    // https://docs.python.org/3.7/c-api/object.html#c.PyObject_GetAttrString
    PyObject *new_pluginCreate = PyObject_GetAttrString(new_pModule, createFunction.c_str());
    Py_DECREF(new_pModule);
    if (!new_pluginCreate || !PyCallable_Check(new_pluginCreate)) {
        RaceLog::logError(logPrefix, "Cannot find plugin.", "");
        PythonLoaderHelper::checkForPythonError();
        PyGILState_Release(gstate);
        return nullptr;
    }

    // Set the Python wrapped SDK object as a Python argument.
    PyObject *new_pArgs = PyTuple_New(4);

    // Name
    PyTuple_SetItem(new_pArgs, 0, SWIG_Python_str_FromChar(name.c_str()));

    // Convert/wrap the SDK object for python using SWIG bindings.
    PyTuple_SetItem(new_pArgs, 1, SWIG_NewPointerObj(sdk, SWIG_TypeQuery(sdkType.c_str()), 0));

    // Role Name
    PyTuple_SetItem(new_pArgs, 2, SWIG_Python_str_FromChar(roleName.c_str()));

    // PuginConfig
    PyTuple_SetItem(new_pArgs, 3,
                    SWIG_NewPointerObj(static_cast<void *>(&pluginConfig),
                                       SWIG_TypeQuery(ARG_PLUGIN_CONFIG.c_str()), 0));

    // Construct the Python plugin object.
    // https://docs.python.org/3.7/c-api/object.html#c.PyObject_CallObject
    PyObject *new_instance = PyObject_CallObject(new_pluginCreate, new_pArgs);
    Py_DECREF(new_pluginCreate);
    Py_DECREF(new_pArgs);
    if (new_instance == nullptr) {
        RaceLog::logError(logPrefix, "PyObject_CallObject returned nullptr", "");
        PythonLoaderHelper::checkForPythonError();
        PyGILState_Release(gstate);
        return nullptr;
    }

    // Convert/wrap the Python plugin for C++ using SWIG bindings.
    void *pythonPluginCpp = nullptr;
    swig_type_info *pTypeInfo = SWIG_TypeQuery(pluginType.c_str());
    const int res = SWIG_ConvertPtr(new_instance, &pythonPluginCpp, pTypeInfo,
                                    SWIG_POINTER_DISOWN);  // steal reference
    if (!SWIG_IsOK(res)) {
        RaceLog::logError(logPrefix,
                          "Failed to convert pointer to " + pluginType +
                              ". ptypeinfo = " + std::string{pTypeInfo->name},
                          "");
        Py_DECREF(new_instance);
        PyGILState_Release(gstate);
        return nullptr;
    }

    // This is the matching call to the call to PyGILState_Ensure() above.
    // https://docs.python.org/3.7/c-api/init.html#c.PyGILState_Release
    PyGILState_Release(gstate);

    PythonLoaderHelper::savePythonThread();

    RaceLog::logInfo(logPrefix, "returning", "");
    return pythonPluginCpp;
}

static void destroyPythonTransport(void *obj) {
    PythonLoaderHelper::destroyPythonPlugin(&obj, PLUGIN_TYPE_TRANSPORT);
}

static void destroyPythonUserModel(void *obj) {
    PythonLoaderHelper::destroyPythonPlugin(&obj, PLUGIN_TYPE_USER_MODEL);
}

static void destroyPythonEncoding(void *obj) {
    PythonLoaderHelper::destroyPythonPlugin(&obj, PLUGIN_TYPE_ENCODING);
}

PythonComponentPlugin::PythonComponentPlugin(const std::string &path,
                                             const std::string &pythonModule) :
    path(path), pythonModule(pythonModule) {}

std::shared_ptr<ITransportComponent> PythonComponentPlugin::createTransport(
    std::string name, ITransportSdk *sdk, std::string roleName, PluginConfig pluginConfig) {
    TRACE_METHOD(path, name);
    pluginConfig.pluginDirectory = fs::path(path).string();
    return std::shared_ptr<ITransportComponent>(
        static_cast<ITransportComponent *>(createPythonComponentPlugin(
            FUNC_CREATE_TRANSPORT, path, pythonModule, SDK_TYPE_TRANSPORT, PLUGIN_TYPE_TRANSPORT,
            name, sdk, roleName, pluginConfig)),
        destroyPythonTransport);
}

std::shared_ptr<IUserModelComponent> PythonComponentPlugin::createUserModel(
    std::string name, IUserModelSdk *sdk, std::string roleName, PluginConfig pluginConfig) {
    TRACE_METHOD(path, name);
    pluginConfig.pluginDirectory = fs::path(path).string();
    return std::shared_ptr<IUserModelComponent>(
        static_cast<IUserModelComponent *>(createPythonComponentPlugin(
            FUNC_CREATE_USER_MODEL, path, pythonModule, SDK_TYPE_USER_MODEL, PLUGIN_TYPE_USER_MODEL,
            name, sdk, roleName, pluginConfig)),
        destroyPythonUserModel);
}

std::shared_ptr<IEncodingComponent> PythonComponentPlugin::createEncoding(
    std::string name, IEncodingSdk *sdk, std::string roleName, PluginConfig pluginConfig) {
    TRACE_METHOD(path, name);
    pluginConfig.pluginDirectory = fs::path(path).string();
    return std::shared_ptr<IEncodingComponent>(
        static_cast<IEncodingComponent *>(
            createPythonComponentPlugin(FUNC_CREATE_ENCODING, path, pythonModule, SDK_TYPE_ENCODING,
                                        PLUGIN_TYPE_ENCODING, name, sdk, roleName, pluginConfig)),
        destroyPythonEncoding);
}

std::string PythonComponentPlugin::get_path() {
    return path;
}
