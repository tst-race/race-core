
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

class ComponentPackageManager {
public:
    explicit ComponentPackageManager(ComponentManagerInternal &manager);

    PluginResponse sendPackage(CMTypes::ComponentWrapperHandle postId, double now,
                               CMTypes::PackageSdkHandle handle, const ConnectionID &connectionId,
                               EncPkg &&pkg, double timeoutTimestamp, uint64_t batchId);

    CMTypes::CmInternalStatus onLinkStatusChanged(CMTypes::ComponentWrapperHandle postId,
                                                  CMTypes::LinkSdkHandle handle,
                                                  const LinkID &linkId, LinkStatus status,
                                                  const LinkParameters &params);

    CMTypes::CmInternalStatus onBytesEncoded(CMTypes::ComponentWrapperHandle postId,
                                             CMTypes::EncodingHandle handle,
                                             std::vector<uint8_t> &&bytes, EncodingStatus status);

    CMTypes::CmInternalStatus onPackageStatusChanged(CMTypes::ComponentWrapperHandle postId,
                                                     CMTypes::PackageFragmentHandle handle,
                                                     PackageStatus status);

    void encodeForAction(CMTypes::ActionInfo *actionInfo);
    void updatedActions();
    std::vector<CMTypes::PackageFragmentHandle> getPackageHandlesForAction(
        CMTypes::ActionInfo *actionInfo);
    void actionDone(CMTypes::ActionInfo *actionInfo);

    void teardown();
    void setup();

protected:
    bool isLastFragment(CMTypes::PackageFragmentInfo *fragment);
    bool isPackageFinished(CMTypes::PackageInfo *packageInfo);
    bool isActionEncoded(CMTypes::ActionInfo *actionInfo);
    bool isActionAbleToFit(CMTypes::ActionInfo *action, const EncPkg &pkg);
    bool isPackageAbleToFit(CMTypes::Link *link, const EncPkg &pkg);
    bool isTimeToEncode(double now, CMTypes::ActionInfo *actionInfo);

    size_t spaceAvailableInAction(CMTypes::ActionInfo *actionInfo);
    bool generateFragmentsForPackage(double now, CMTypes::Link *link,
                                     CMTypes::PackageInfo *packageInfo);
    void removeMarkedFragments(CMTypes::Link *link);
    void generateFragmentsForAllPackages();

    friend std::ostream &operator<<(std::ostream &out, const ComponentPackageManager &manager);

public:
    std::unordered_map<CMTypes::EncodingHandle, CMTypes::EncodingInfo *> pendingEncodings;

protected:
    ComponentManagerInternal &manager;
    uint64_t nextEncodingHandle{0};
    uint64_t nextFragmentHandle{0};

    // Mapping of SDK handles to all packages being processed by the comms plugin. This mapping is
    // necessary in order to remove packages from link package queues when onPackageStatusChanged
    // gets called.
    std::unordered_map<CMTypes::PackageFragmentHandle, CMTypes::PackageFragmentInfo *> fragments;
};