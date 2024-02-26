
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

#ifndef __I_ENCODING_COMPONENT_H__
#define __I_ENCODING_COMPONENT_H__

#include "ComponentTypes.h"
#include "IComponentBase.h"
#include "IEncodingSdk.h"
#include "PluginConfig.h"
#include "RacePluginExports.h"

class IEncodingComponent : public IComponentBase {
public:
    virtual ~IEncodingComponent() = default;

    // Global encoding properties. e.g. max encoding time
    virtual EncodingProperties getEncodingProperties() = 0;

    // Parameter specific properties, e.g. how many bytes I can stuff into an image encoded
    // with these params
    virtual SpecificEncodingProperties getEncodingPropertiesForParameters(
        const EncodingParameters &params) = 0;

    // Actually encode these bytes into content - bytes should always be small enough to fit due to
    // Channel's use of the above API
    virtual ComponentStatus encodeBytes(RaceHandle handle, const EncodingParameters &params,
                                        const std::vector<uint8_t> &bytes) = 0;

    virtual ComponentStatus decodeBytes(RaceHandle handle, const EncodingParameters &params,
                                        const std::vector<uint8_t> &bytes) = 0;
};

extern "C" EXPORT IEncodingComponent *createEncoding(const std::string &name, IEncodingSdk *sdk,
                                                     const std::string &roleName,
                                                     const PluginConfig &pluginConfig);
extern "C" EXPORT void destroyEncoding(IEncodingComponent *component);

#endif
