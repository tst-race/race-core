%module(directors="1", threads="1") networkManagerPluginBindings

%{
#include <iostream>
#include <vector>
#include <string>

#include "ClrMsg.h"
#include "EncPkg.h"
#include "ChannelProperties.h"
#include "ChannelRole.h"
#include "ChannelStatus.h"
#include "ConnectionStatus.h"
#include "ConnectionType.h"
#include "DeviceInfo.h"
#include "IRacePluginNM.h"
#include "IRaceSdkCommon.h"
#include "IRaceSdkApp.h"
#include "IRaceSdkNM.h"
#include "LinkProperties.h"
#include "LinkPropertyPair.h"
#include "LinkPropertySet.h"
#include "LinkStatus.h"
#include "LinkType.h"
#include "MessageStatus.h"
#include "PackageStatus.h"
#include "PackageType.h"
#include "SendType.h"
#include "TransmissionType.h"
#include "PackageStatus.h"
#include "PluginConfig.h"
#include "PluginResponse.h"
#include "RaceEnums.h"
#include "PluginStatus.h"
#include "RaceLog.h"
#include "SdkResponse.h"
%}

// Enable cross-language polymorphism in the SWIG wrapper.
// It's pretty slow so not enable by default
%feature("director") IRacePluginNM;

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

%include "std_map.i"
%include "std_string.i"
%include "std_vector.i"
%include "stdint.i"

%ignore ::createPluginNM;
%ignore ::destroyPluginNM;

%template(ChannelRoleVector) std::vector<ChannelRole>;
%template(StringVector) std::vector<std::string>;
%template(ByteVector) std::vector<uint8_t>;
%template(ChannelPropertiesMap) std::map<std::string, ChannelProperties>;
%template(ChannelPropertiesVector) std::vector<ChannelProperties>;

%include "RaceExport.h"

%include "ClrMsg.h"
%include "EncPkg.h"
%include "ChannelProperties.h"
%include "ChannelRole.h"
%include "ChannelStatus.h"
%include "ConnectionStatus.h"
%include "ConnectionType.h"
%include "DeviceInfo.h"
%include "IRaceSdkApp.h"
%include "IRacePluginNM.h"
%include "IRaceSdkCommon.h"
%include "IRaceSdkNM.h"
%include "LinkProperties.h"
%include "LinkPropertyPair.h"
%include "LinkPropertySet.h"
%include "LinkStatus.h"
%include "LinkType.h"
%include "MessageStatus.h"
%include "PackageStatus.h"
%include "PackageType.h"
%include "SendType.h"
%include "TransmissionType.h"
%include "PackageStatus.h"
%include "PluginConfig.h"
%include "PluginResponse.h"
%include "PluginStatus.h"
%include "RaceEnums.h"
%include "RaceLog.h"
%include "SdkResponse.h"
