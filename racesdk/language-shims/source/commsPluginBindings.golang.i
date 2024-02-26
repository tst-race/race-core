%module(directors="1") commsPluginBindingsGolang

%include <typemaps.i>

// We need to include CommsPlugin.h in the SWIG generated C++ file
%{
#include <iostream>
#include <vector>
#include <string>

#include "EncPkg.h"
#include "ChannelProperties.h"
#include "ChannelRole.h"
#include "ChannelStatus.h"
#include "ConnectionStatus.h"
#include "ConnectionType.h"
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
%feature("director") IRacePluginComms;

// Tell swig to wrap everything in CommsPlugin.h
%include "stdint.i"
%include "std_string.i"
%include "std_vector.i"

%template(RoleVector) std::vector<ChannelRole>;
%template(StringVector) std::vector<std::string>;
%template(ByteVector) std::vector<uint8_t>;

// %typemap(gotype) (std::vector<std::string>) %{[]string%}

// %typemap(in) (std::vector<std::string>) {
//   printf("swigggg typemap start");
//   $1 = std::vector<std::string>();
//   for (int i = 0; i < $input.len; ++i) {
//     _gostring_ *str = static_cast<_gostring_*>($input.array);
//     $1.push_back(std::string(str[i].p, str[i].n));
//   }
// }

%include "RaceExport.h"

%include "EncPkg.h"
%include "ChannelProperties.h"
%include "ChannelRole.h"
%include "ChannelStatus.h"
%include "ConnectionStatus.h"
%include "ConnectionType.h"
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
