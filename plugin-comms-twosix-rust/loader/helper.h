
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

#ifndef _LOADER_HELPER_H__
#define _LOADER_HELPER_H__

#include "ChannelPropertiesC.h"
#include "plugin_extern_c.h"

namespace helper {

void convertLinkPropertySetCToClass(const LinkPropertySetC &input, LinkPropertySet &output);

void convertLinkPropertyPairCToClass(const LinkPropertyPairC &input, LinkPropertyPair &output);

void convertLinkPropertiesCToClass(const LinkPropertiesC &input, LinkProperties &output);

void convertChannelPropertiesCToClass(const ChannelPropertiesC &input, ChannelProperties &output);

void convertChannelRoleCToClass(const ChannelRoleC &input, ChannelRole &output);

void convertLinkPropertySetToLinkPropertySetC(const LinkPropertySet &input,
                                              LinkPropertySetC &output);

void convertLinkPropertyPairToLinkPropertyPairC(const LinkPropertyPair &input,
                                                LinkPropertyPairC &output);

void convertChannelPropertiesToChannelPropertiesC(const ChannelProperties &input,
                                                  ChannelPropertiesC &output);

void convertChannelRoleToChannelRoleC(const ChannelRole &input, ChannelRoleC &output);
}  // namespace helper

#endif
