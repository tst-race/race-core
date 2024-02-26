
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

#ifndef __SOURCE_ENCODING_PLUGINCOMMSTWOSIXBASE64ENCODING_H__
#define __SOURCE_ENCODING_PLUGINCOMMSTWOSIXBASE64ENCODING_H__

#include <IEncodingComponent.h>

#include <string>

class PluginCommsTwoSixBase64Encoding : public IEncodingComponent {
public:
    explicit PluginCommsTwoSixBase64Encoding(IEncodingSdk *sdk);

    virtual ComponentStatus onUserInputReceived(RaceHandle handle, bool answered,
                                                const std::string &response) override;

    virtual EncodingProperties getEncodingProperties() override;

    virtual SpecificEncodingProperties getEncodingPropertiesForParameters(
        const EncodingParameters &params) override;

    virtual ComponentStatus encodeBytes(RaceHandle handle, const EncodingParameters &params,
                                        const std::vector<uint8_t> &bytes) override;

    virtual ComponentStatus decodeBytes(RaceHandle handle, const EncodingParameters &params,
                                        const std::vector<uint8_t> &bytes) override;

public:
    static const char *name;

private:
    IEncodingSdk *sdk;
};

#endif
