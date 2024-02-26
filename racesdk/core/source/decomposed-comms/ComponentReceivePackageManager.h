
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

#pragma once

#include <memory>
#include <unordered_map>

#include "ComponentManagerTypes.h"
#include "ComponentWrappers.h"
#include "Composition.h"
#include "IRacePluginComms.h"
#include "SdkWrappers.h"
#include "plugin-loading/ComponentPlugin.h"

class ComponentManagerInternal;

class ComponentReceivePackageManager {
public:
    explicit ComponentReceivePackageManager(ComponentManagerInternal &manager);
    virtual ~ComponentReceivePackageManager(){};

    virtual CMTypes::CmInternalStatus onReceive(CMTypes::ComponentWrapperHandle postId,
                                                const LinkID &linkId,
                                                const EncodingParameters &params,
                                                std::vector<uint8_t> &&bytes);

    virtual CMTypes::CmInternalStatus onBytesDecoded(CMTypes::ComponentWrapperHandle postId,
                                                     CMTypes::DecodingHandle handle,
                                                     std::vector<uint8_t> &&bytes,
                                                     EncodingStatus status);

    void teardown();
    void setup();

protected:
    CMTypes::CmInternalStatus receiveSingle(std::vector<uint8_t> &&bytes,
                                            std::vector<std::string> &&connVec);
    CMTypes::CmInternalStatus receiveBatch(std::vector<uint8_t> &&bytes,
                                           std::vector<std::string> &&connVec);
    CMTypes::CmInternalStatus receiveFragmentSingleProducer(CMTypes::Link *link,
                                                            std::vector<uint8_t> &&bytes,
                                                            std::vector<std::string> &&connVec);
    CMTypes::CmInternalStatus receiveFragmentMultipleProducer(CMTypes::Link *link,
                                                              std::vector<uint8_t> &&bytes,
                                                              std::vector<std::string> &&connVec);
    CMTypes::CmInternalStatus receiveFragmentProducer(const std::string &producer, size_t offset,
                                                      CMTypes::Link *link,
                                                      std::vector<uint8_t> &&bytes,
                                                      std::vector<std::string> &&connVec);

    std::vector<uint8_t> readFragment(const std::vector<uint8_t> &buffer, size_t &offset);

    friend std::ostream &operator<<(std::ostream &out,
                                    const ComponentReceivePackageManager &manager);

protected:
    ComponentManagerInternal &manager;
    uint64_t nextDecodingHandle{0};

    std::unordered_map<CMTypes::DecodingHandle, LinkID> pendingDecodings;
};