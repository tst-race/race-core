
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

#ifndef __I_TRANSPORT_SDK_H__
#define __I_TRANSPORT_SDK_H__

#include "ChannelProperties.h"
#include "ComponentTypes.h"
#include "IComponentSdkBase.h"
#include "LinkStatus.h"
#include "PackageStatus.h"

class ITransportSdk : public virtual IComponentSdkBase {
public:
    virtual ~ITransportSdk() = default;

    virtual ChannelProperties getChannelProperties() = 0;

    virtual ChannelResponse onLinkStatusChanged(RaceHandle handle, const LinkID &linkId,
                                                LinkStatus status,
                                                const LinkParameters &params) = 0;

    virtual ChannelResponse onPackageStatusChanged(RaceHandle handle, PackageStatus status) = 0;

    virtual ChannelResponse onEvent(const Event &event) = 0;

    virtual ChannelResponse onReceive(const LinkID &linkId, const EncodingParameters &params,
                                      const std::vector<uint8_t> &bytes) = 0;
};

#endif
