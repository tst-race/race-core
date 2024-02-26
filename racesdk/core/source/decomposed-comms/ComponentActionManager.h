
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

class ComponentActionManager {
public:
    explicit ComponentActionManager(ComponentManagerInternal &manager);

    CMTypes::CmInternalStatus onTimelineUpdated(CMTypes::ComponentWrapperHandle postId);

    CMTypes::CmInternalStatus onSendPackage(double now, const ConnectionID &connId,
                                            const EncPkg &pkg);

    CMTypes::CmInternalStatus onLinkStatusChanged(CMTypes::ComponentWrapperHandle postId,
                                                  CMTypes::LinkSdkHandle handle,
                                                  const LinkID &linkId, LinkStatus status,
                                                  const LinkParameters &params);

    void teardown();
    void setup();

    double getMaxEncodingTime();
    void joinActionThread();

protected:
    void fetchTimeline();
    void updateTimeline(ActionTimeline &usermodelActions, Timestamp start);

    std::unique_ptr<CMTypes::ActionInfo> createActionInfo(Action &&action);
    void insertActions(ActionTimeline &usermodelActions);
    void updateGlobalTimeline(ActionTimeline &usermodelActions, Timestamp start);
    void updateLinkTimelines();
    void removeDeletedActions();
    void updateActionTimestamp();
    void updateEncodeTimestamp();

    void runActionThread();
    bool actionThreadLogic(Timestamp now);
    void doAction();
    void encodeActions(Timestamp now);

    virtual double currentTime();

public:
    double maxEncodingTime = 0;
    double timelineLength = 600;
    double timelineFetchPeriod = 300;

    Timestamp nextFetchTime;
    Timestamp nextActionTime;
    Timestamp nextEncodeTime;

    Timestamp lastEncodeTime;

    // unique_ptr is used to keep the address the same, even if the item in the deque is moved.
    std::deque<std::unique_ptr<CMTypes::ActionInfo>> actions;

    // thread responsible for updating action queue and notifying the transport
    std::thread actionThread;
    std::condition_variable_any actionThreadSignaler;

protected:
    ComponentManagerInternal &manager;
};

std::ostream &operator<<(std::ostream &out, const ComponentActionManager &cam);
