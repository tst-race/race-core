
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

#ifndef __SDK_WRAPPERS_H__
#define __SDK_WRAPPERS_H__

#include "IEncodingComponent.h"
#include "ITransportComponent.h"
#include "IUserModelComponent.h"

class ComponentManager;

class ComponentSdkBaseWrapper : public virtual IComponentSdkBase {
public:
    ComponentSdkBaseWrapper(ComponentManager &manager, const std::string &id);

    virtual std::string getActivePersona() override;

    virtual ChannelResponse requestPluginUserInput(const std::string &key,
                                                   const std::string &prompt, bool cache) override;
    virtual ChannelResponse requestCommonUserInput(const std::string &key) override;

    virtual ChannelResponse updateState(ComponentState state) override;

    virtual std::string toString() const;

    virtual ChannelResponse makeDir(const std::string &directoryPath) override;
    virtual ChannelResponse removeDir(const std::string &directoryPath) override;
    virtual std::vector<std::string> listDir(const std::string &directoryPath) override;
    virtual std::vector<std::uint8_t> readFile(const std::string &filepath) override;
    virtual ChannelResponse appendFile(const std::string &filepath,
                                       const std::vector<std::uint8_t> &data) override;
    virtual ChannelResponse writeFile(const std::string &filepath,
                                      const std::vector<std::uint8_t> &data) override;

protected:
    ComponentManager &manager;
    std::string id;
};

std::ostream &operator<<(std::ostream &out, const ComponentSdkBaseWrapper &wrapper);

class TransportSdkWrapper : public ComponentSdkBaseWrapper, public ITransportSdk {
public:
    TransportSdkWrapper(ComponentManager &sdk, const std::string &id);

    virtual ChannelProperties getChannelProperties() override;

    virtual ChannelResponse onLinkStatusChanged(RaceHandle handle, const LinkID &linkId,
                                                LinkStatus status,
                                                const LinkParameters &params) override;

    virtual ChannelResponse onPackageStatusChanged(RaceHandle handle,
                                                   PackageStatus status) override;

    virtual ChannelResponse onEvent(const Event &event) override;

    virtual ChannelResponse onReceive(const LinkID &linkId, const EncodingParameters &params,
                                      const std::vector<uint8_t> &bytes) override;
};

class UserModelSdkWrapper : public ComponentSdkBaseWrapper, public IUserModelSdk {
public:
    UserModelSdkWrapper(ComponentManager &sdk, const std::string &id);

    virtual ChannelResponse onTimelineUpdated() override;
};

class EncodingSdkWrapper : public ComponentSdkBaseWrapper, public IEncodingSdk {
public:
    EncodingSdkWrapper(ComponentManager &sdk, const std::string &id);

    virtual ChannelResponse onBytesEncoded(RaceHandle handle, const std::vector<uint8_t> &bytes,
                                           EncodingStatus status) override;
    virtual ChannelResponse onBytesDecoded(RaceHandle handle, const std::vector<uint8_t> &bytes,
                                           EncodingStatus status) override;
};

#endif