%module(directors="1", threads="1") commsPluginBindings

// We need to include CommsPlugin.h in the SWIG generated C++ file
%{
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

#include "EncPkg.h"
#include "ChannelProperties.h"
#include "ChannelRole.h"
#include "ChannelStatus.h"
#include "ComponentTypes.h"
#include "ConnectionStatus.h"
#include "ConnectionType.h"
#include "IComponentBase.h"
#include "IComponentSdkBase.h"
#include "IEncodingComponent.h"
#include "IEncodingSdk.h"
#include "ITransportComponent.h"
#include "ITransportSdk.h"
#include "IUserModelComponent.h"
#include "IUserModelSdk.h"
#include "IRacePluginComms.h"
#include "IRaceSdkCommon.h"
#include "IRaceSdkComms.h"
#include "LinkProperties.h"
#include "LinkPropertyPair.h"
#include "LinkPropertySet.h"
#include "LinkStatus.h"
#include "LinkType.h"
#include "PackageStatus.h"
#include "PackageType.h"
#include "SendType.h"
#include "TransmissionType.h"
#include "PackageStatus.h"
#include "PluginConfig.h"
#include "PluginResponse.h"
#include "RaceEnums.h"
#include "RaceLog.h"
#include "SdkResponse.h"
%}

// Enable cross-language polymorphism in the SWIG wrapper.
// It's pretty slow so not enable by default
%feature("director");

// Catch any unhandled Python error and throw a C++ exception with the error message
%feature("director:except") {
    if ($error != NULL) {
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);

        // $symname will get the name of the current Python method
        std::string what = "$symname: ";

        // Add the error type name
        PyObject *ptypename = PyObject_GetAttrString(ptype, "__name__");
        what += PyUnicode_AsUTF8(ptypename);
        Py_XDECREF(ptypename);

        // Add the error message (__repr__ of the error), if the error object exists
        if (pvalue != NULL) {
            PyObject *pvalstr = PyObject_Str(pvalue);
            what += ": ";
            what += PyUnicode_AsUTF8(pvalstr);
            Py_XDECREF(pvalstr);
        }

        // Give object references back to Python interpreter
        PyErr_Restore(ptype, pvalue, ptraceback);
        throw std::runtime_error(what);
    }
}

// Tell swig to wrap everything in CommsPlugin.h
%include "std_string.i"
%include "stdint.i"
%include "std_vector.i"
%include "std_unordered_map.i"

%ignore ::createPluginComms;
%ignore ::destroyPluginComms;
%ignore ::createEncoding;
%ignore ::destroyEncoding;
%ignore ::createTransport;
%ignore ::destroyTransport;
%ignore ::createUserModel;
%ignore ::destroyUserModel;

%template(RaceHandleVector) std::vector<unsigned long>;
%template(RoleVector) std::vector<ChannelRole>;
%template(StringVector) std::vector<std::string>;
%template(ByteVector) std::vector<uint8_t>;
%template(EncodingParamVector) std::vector<EncodingParameters>;
%template(ActionVector) std::vector<Action>;

%typemap(in) std::vector<std::string> * (std::vector<std::string> temp) {
    PyObject *iterator = PyObject_GetIter($input);
    PyObject *item;

    while ((item = PyIter_Next(iterator))) {
        const char * eType = PyUnicode_AsUTF8(item);
        temp.push_back(eType);
        Py_DECREF(item);
    }

    Py_DECREF(iterator);
    $1 = &temp;
}

%typemap(in) std::unordered_map<std::string, std::vector<EncodingType>> * (std::unordered_map<std::string, std::vector<EncodingType>> temp) {
    PyObject *keys = PyDict_Keys($input);
    PyObject *k, *v;
    Py_ssize_t pos = 0;

    while (PyDict_Next($input, &pos, &k, &v)) {
        if (!PyUnicode_Check(k)) {
            continue;
        }
        const char *key = PyUnicode_AsUTF8(k);
        int val_size = PyList_Size(v);
        std::vector<EncodingType> vec;
        for (int j = 0; j < val_size; j++) {
            const char * eType = PyUnicode_AsUTF8(PyList_GetItem(v, j));
            vec.push_back(eType);
        }
        temp[key] = vec;
    }
    $1 = &temp;
}

%include "RaceExport.h"

%include "EncPkg.h"
%include "ChannelProperties.h"
%include "ChannelRole.h"
%include "ChannelStatus.h"
%include "ComponentTypes.h"
%include "ConnectionStatus.h"
%include "ConnectionType.h"
%include "IComponentBase.h"
%include "IComponentSdkBase.h"
%include "IEncodingComponent.h"
%include "IEncodingSdk.h"
%include "ITransportComponent.h"
%include "ITransportSdk.h"
%include "IUserModelComponent.h"
%include "IUserModelSdk.h"
%include "IRacePluginComms.h"
%include "IRaceSdkCommon.h"
%include "IRaceSdkComms.h"
%include "LinkProperties.h"
%include "LinkPropertyPair.h"
%include "LinkPropertySet.h"
%include "LinkStatus.h"
%include "LinkType.h"
%include "PackageStatus.h"
%include "PackageType.h"
%include "SendType.h"
%include "TransmissionType.h"
%include "PackageStatus.h"
%include "PluginConfig.h"
%include "PluginResponse.h"
%include "RaceEnums.h"
%include "RaceLog.h"
%include "SdkResponse.h"
