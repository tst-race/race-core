
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

#ifndef __I_ENCODING_SDK_H__
#define __I_ENCODING_SDK_H__

#include <vector>

#include "IComponentSdkBase.h"

enum EncodingStatus {
    ENCODE_OK,
    ENCODE_FAILED,
};

class IEncodingSdk : public virtual IComponentSdkBase {
public:
    virtual ~IEncodingSdk() = default;
    virtual ChannelResponse onBytesEncoded(RaceHandle handle, const std::vector<uint8_t> &bytes,
                                           EncodingStatus status) = 0;
    virtual ChannelResponse onBytesDecoded(RaceHandle handle, const std::vector<uint8_t> &bytes,
                                           EncodingStatus status) = 0;
};

#endif
