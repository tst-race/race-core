
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

#ifndef __PYTHON_LOADER_HELPER_H__
#define __PYTHON_LOADER_HELPER_H__

#include <string.h>

#include "LoaderWrapper.h"
#include "PythonLoaderWrapper.h"
#include "RaceSdk.h"
#include "swigpyrun.h"

class PythonLoaderHelper {
public:
    /**
     * @brief Log a Python error. This function should be passed the resulting `value` from
     *        `PyErr_Fetch`.
     *
     * @param value Pointer to the Python exception object.
     */
    static std::string logPyErr(PyObject *value) {
        RaceLog::logError("logPyErr", "an error occurred while loading the plugin.", "");
        PyObject *valueStr = PyObject_Str(value);
        if (valueStr == nullptr) {
            const std::string errorMessage =
                "logPyErr failed to get error description: an error occurred while trying to "
                "retrieve "
                "the Python error [valueStr].";
            RaceLog::logError("logPyErr", errorMessage, "");
            return errorMessage;
        }
        PyObject *encodedValueStr = PyUnicode_AsEncodedString(valueStr, "utf-8", "~E~");
        if (encodedValueStr == nullptr) {
            const std::string errorMessage =
                "logPyErr failed to get error description: an error occurred while trying to "
                "retrieve "
                "the Python error [encodedValueStr].";
            RaceLog::logError("logPyErr", errorMessage, "");
            return errorMessage;
        }
        const char *bytes = PyBytes_AS_STRING(encodedValueStr);
        if (bytes == nullptr) {
            const std::string errorMessage =
                "logPyErr failed to get error description: an error occurred while trying to "
                "retrieve "
                "the Python error [bytes].";
            RaceLog::logError("logPyErr", errorMessage, "");
            return errorMessage;
        }
        const std::string errorDescription{bytes};
        RaceLog::logError("logPyErr", errorDescription, "");
        return errorDescription;
    }

    /**
     * @brief Check if a Python error occurred. If it did, throw an exception describing the error.
     *
     */
    static void checkForPythonError() {
        PyObject *type = nullptr, *value = nullptr, *traceback = nullptr;
        PyErr_Fetch(&type, &value, &traceback);
        if (value != nullptr) {
            const std::string errorMessage = logPyErr(value);
            throw std::runtime_error(errorMessage);
        }
    }

    static void savePythonThread() {
        // WARNING: calling PyEval_SaveThread() twice will cause the program to crash.
        static bool PyEval_SaveThread_wasCalled = false;
        if (!PyEval_SaveThread_wasCalled) {
            PyEval_SaveThread_wasCalled = true;
            // Release the GIL and reset the thread state to NULL. Note that the Python plugin will
            // deadlock if this API is not called.
            // https://docs.python.org/3.7/c-api/init.html#c.PyEval_SaveThread
            // TODO: we should be storing the result of this call and passing it to
            // PyEval_RestoreThread on shutdown.
            RaceLog::logDebug("savePythonThread", "calling PyEval_SaveThread...", "");
            /*PyThreadState *save = */ PyEval_SaveThread();
            RaceLog::logDebug("savePythonThread", "PyEval_SaveThread returned", "");
        }
    }

    /**
     * @brief Destroy the plugin with the given type
     *
     */
    static void destroyPythonPlugin(void **obj, const std::string &pluginType) {
        Py_Initialize();
        PyGILState_STATE gstate = PyGILState_Ensure();

        swig_type_info *pTypeInfo = SWIG_TypeQuery(pluginType.c_str());
        PyObject *new_pObj = SWIG_NewPointerObj(*obj, pTypeInfo, SWIG_POINTER_OWN);
        Py_DECREF(new_pObj);

        PyGILState_Release(gstate);
    }
};

#endif
