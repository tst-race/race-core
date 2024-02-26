
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

#include "ComponentActionManager.h"

#include "ComponentManager.h"

using namespace CMTypes;

ComponentActionManager::ComponentActionManager(ComponentManagerInternal &manager) :
    nextFetchTime(0),
    nextActionTime(std::numeric_limits<double>::infinity()),
    nextEncodeTime(std::numeric_limits<double>::infinity()),
    lastEncodeTime(0),
    manager(manager) {}

void ComponentActionManager::teardown() {
    TRACE_METHOD();

    actions.clear();

    nextFetchTime = 0;
    nextActionTime = std::numeric_limits<double>::infinity();
    nextEncodeTime = std::numeric_limits<double>::infinity();
    lastEncodeTime = 0;
}

double ComponentActionManager::getMaxEncodingTime() {
    return maxEncodingTime;
}

void ComponentActionManager::setup() {
    TRACE_METHOD();

    auto transportProps = manager.getTransport()->getTransportProperties();
    maxEncodingTime = 0;
    for (auto &action : transportProps.supportedActions) {
        double encodingTime = 0;
        for (EncodingType encodingType : action.second) {
            EncodingParameters params;
            params.type = encodingType;
            auto encoding = manager.encodingComponentFromEncodingParams(params);
            if (encoding == nullptr) {
                std::string message =
                    logPrefix + "Failed to find encoding for params. Encoding type: " + params.type;
                helper::logError(message);
                throw std::out_of_range(message);
            }
            encodingTime += encoding->getEncodingProperties().encodingTime;
        }
        maxEncodingTime = std::max(maxEncodingTime, encodingTime);
    }

    // add 0.1 for ComponentManager overhead. This isn't based on anything. Is there a better
    // approach?
    maxEncodingTime += 0.1;

    auto usermodelProperties = manager.getUserModel()->getUserModelProperties();
    timelineLength = usermodelProperties.timelineLength;
    timelineFetchPeriod = usermodelProperties.timelineFetchPeriod;

    fetchTimeline();
    actionThread = std::thread(&ComponentActionManager::runActionThread, this);
}

CMTypes::CmInternalStatus ComponentActionManager::onLinkStatusChanged(
    CMTypes::ComponentWrapperHandle /* postId */, CMTypes::LinkSdkHandle /* handle */,
    const LinkID &linkId, LinkStatus status, const LinkParameters & /* params */) {
    TRACE_METHOD(linkId, status);
    try {
        if (status == LINK_DESTROYED) {
            auto link = manager.getLink(linkId);

            // remove action from list of global actions
            std::unordered_set<ActionInfo *> set{link->actionQueue.begin(),
                                                 link->actionQueue.begin()};
            actions.erase(
                std::remove_if(actions.begin(), actions.end(),
                               [set](auto &actionInfo) { return set.count(actionInfo.get()) > 0; }),
                actions.end());
            link->actionQueue.clear();
        }
    } catch (std::out_of_range & /* e */) {
        // Nothing to do if the link doesn't exist
    }

    return OK;
}

CMTypes::CmInternalStatus ComponentActionManager::onSendPackage(double now,
                                                                const ConnectionID &connId,
                                                                const EncPkg &pkg) {
    auto linkId = manager.getConnection(connId)->linkId;
    auto usermodelActions = manager.getUserModel()->onSendPackage(linkId, pkg.getSize());

    if (!usermodelActions.empty()) {
        for (auto &action : usermodelActions) {
            if (action.timestamp < now + maxEncodingTime) {
                action.timestamp = now + maxEncodingTime;
            }
        }

        insertActions(usermodelActions);
        updateLinkTimelines();
        updateActionTimestamp();
        updateEncodeTimestamp();
        actionThreadSignaler.notify_all();
    }

    return OK;
}

CMTypes::CmInternalStatus ComponentActionManager::onTimelineUpdated(
    ComponentWrapperHandle /* postId */) {
    fetchTimeline();
    return OK;
}

void ComponentActionManager::fetchTimeline() {
    TRACE_METHOD();
    Timestamp start = currentTime() + maxEncodingTime;
    Timestamp end = start + timelineLength;
    auto usermodelActions = manager.getUserModel()->getTimeline(start, end);
    nextFetchTime = start + timelineFetchPeriod;
    updateTimeline(usermodelActions, start);
}

void ComponentActionManager::updateTimeline(ActionTimeline &usermodelActions, Timestamp start) {
    updateGlobalTimeline(usermodelActions, start);
    updateLinkTimelines();
    removeDeletedActions();
    updateActionTimestamp();
    updateEncodeTimestamp();

    actionThreadSignaler.notify_all();
}

std::unique_ptr<ActionInfo> ComponentActionManager::createActionInfo(Action &&action) {
    MAKE_LOG_PREFIX();
    std::unique_ptr<ActionInfo> info = std::make_unique<ActionInfo>();
    info->action = action;
    info->toBeRemoved = false;

    auto params = manager.getTransport()->getActionParams(info->action);
    for (auto &param : params) {
        if (param.encodePackage) {
            if (param.linkId.empty()) {
                helper::logError(logPrefix +
                                 "Encoding params with encodePackage = True must set linkId");
            } else if (info->linkId.empty()) {
                info->linkId = param.linkId;
            } else if (info->linkId != param.linkId) {
                throw std::runtime_error(
                    "Actions associated with multiple link ids are not supported");
            }

            // TODO: if this is for cover trafic, we will never select the actual link. Does that
            // matter, or can we let the transport decide?
            info->wildcardLink |= (param.linkId == "*");
        }

        auto props =
            manager.encodingComponentFromEncodingParams(param)->getEncodingPropertiesForParameters(
                param);

        info->encoding.push_back({
            std::move(param),
            props,
            {NULL_RACE_HANDLE},
            EncodingState::UNENCODED,
            info.get(),
        });
    }

    if (info->wildcardLink) {
        info->linkId.clear();
    }

    return info;
}

void ComponentActionManager::insertActions(ActionTimeline &usermodelActions) {
    std::deque<std::unique_ptr<ActionInfo>> newActions;

    auto oldIt = actions.begin();
    auto newIt = usermodelActions.begin();
    ActionCompare lt;
    while (true) {
        bool end1 = (oldIt == actions.end());
        bool end2 = (newIt == usermodelActions.end());
        if (end1 && end2) {
            break;
        } else if (end1 || (!end2 && lt(*newIt, (*oldIt)->action))) {
            // action in new actions but not in old actions
            newActions.push_back(createActionInfo(std::move(*newIt)));
            newIt++;
        } else if (end2 || lt((*oldIt)->action, *newIt)) {
            newActions.push_back(std::move(*oldIt));
            oldIt++;
        } else {
            // actions id / timestamp are the same
            newActions.push_back(std::move(*oldIt));
            newIt++;
            oldIt++;
        }
    }

    actions = std::move(newActions);
}

void ComponentActionManager::updateGlobalTimeline(ActionTimeline &usermodelActions,
                                                  Timestamp start) {
    std::deque<std::unique_ptr<ActionInfo>> newActions;

    auto oldIt = actions.begin();

    // advance iterator until after start. We remove actions that the usermodel didn't return,
    // unless they happened before start.
    while (oldIt != actions.end() && (*oldIt)->action.timestamp < start) {
        newActions.push_back(std::move(*oldIt));
        oldIt++;
    }

    auto newIt = usermodelActions.begin();
    ActionCompare lt;
    while (true) {
        bool end1 = (oldIt == actions.end());
        bool end2 = (newIt == usermodelActions.end());
        if (end1 && end2) {
            break;
        } else if (end1 || (!end2 && lt(*newIt, (*oldIt)->action))) {
            // action in new actions but not in old actions
            newActions.push_back(createActionInfo(std::move(*newIt)));
            newIt++;
        } else if (end2 || lt((*oldIt)->action, *newIt)) {
            (*oldIt)->toBeRemoved = true;
            newActions.push_back(std::move(*oldIt));
            oldIt++;
        } else {
            // actions id / timestamp are the same
            newActions.push_back(std::move(*oldIt));
            newIt++;
            oldIt++;
        }
    }

    actions = std::move(newActions);
}

void ComponentActionManager::removeDeletedActions() {
    std::deque<std::unique_ptr<ActionInfo>> newActions;
    for (auto &action : actions) {
        if (!action->toBeRemoved) {
            newActions.push_back(std::move(action));
        }
    }
    actions = std::move(newActions);

    updateLinkTimelines();
}

void ComponentActionManager::updateLinkTimelines() {
    std::unordered_map<LinkID, std::deque<ActionInfo *>> linkQueues;
    auto links = manager.getLinks();

    for (auto &info : actions) {
        if (info->wildcardLink) {
            for (auto &link : links) {
                linkQueues[link->linkId].push_back(info.get());
            }
        } else if (!info->linkId.empty()) {
            linkQueues[info->linkId].push_back(info.get());
        }
    }

    for (auto &link : links) {
        link->actionQueue = std::move(linkQueues[link->linkId]);
    }

    manager.updatedActions();
}
void ComponentActionManager::updateActionTimestamp() {
    if (actions.empty()) {
        nextActionTime = std::numeric_limits<double>::infinity();
    } else {
        nextActionTime = actions.front()->action.timestamp;
    }
}

void ComponentActionManager::updateEncodeTimestamp() {
    auto it = actions.begin();

    while (it != actions.end() && (*it)->action.timestamp <= lastEncodeTime) {
        it++;
    }

    nextEncodeTime = std::numeric_limits<double>::infinity();
    if (it != actions.end()) {
        nextEncodeTime = (*it)->action.timestamp - maxEncodingTime;
    }
}

void ComponentActionManager::runActionThread() {
    TRACE_METHOD();
    std::unique_lock<std::recursive_mutex> lock(manager.dataMutex);
    while (true) {
        // wait until we have something to do
        // possible things to do:
        // 1. stop the thread
        // 2. fetch timeline
        // 3. do action
        // 4. encode cover traffic for an action if it has no other content
        //
        // Each iteration of the while loop will perform one of these, and then check again for
        // any other work
        double wait_until = fmin(fmin(nextFetchTime, nextActionTime), nextEncodeTime);
        double start = currentTime();
        helper::logDebug(logPrefix + "Sleeping for " + std::to_string(wait_until - start) +
                         " seconds");
        std::chrono::high_resolution_clock::time_point tp{
            std::chrono::microseconds{long(1000000 * wait_until)}};
        actionThreadSignaler.wait_until(lock, tp);
        helper::logDebug(logPrefix + "Woke up after " + std::to_string(currentTime() - start) +
                         " seconds");

        double now = currentTime();
        if (actionThreadLogic(now)) {
            return;
        }
    }
}

bool ComponentActionManager::actionThreadLogic(Timestamp now) {
    MAKE_LOG_PREFIX();
    if (manager.getState() != ACTIVATED) {
        helper::logDebug(logPrefix + "Stopping action thread");
        return true;
    } else if (now >= nextFetchTime) {
        helper::logDebug(logPrefix + "Fetching actions");
        fetchTimeline();
    } else if (now >= nextActionTime) {
        helper::logDebug(logPrefix + "Doing action");
        doAction();
    } else if (now >= nextEncodeTime) {
        helper::logDebug(logPrefix + "Encoding for action (if necessary)");
        encodeActions(now);
    }

    return false;
}

void ComponentActionManager::doAction() {
    MAKE_LOG_PREFIX();
    auto info = actions.front().get();
    auto handles = manager.getPackageHandlesForAction(info);
    manager.getTransport()->doAction(handles, info->action);
    manager.actionDone(info);

    if (info->wildcardLink) {
        for (auto link : manager.getLinks()) {
            if (link->actionQueue.front()->action.actionId == info->action.actionId) {
                link->actionQueue.pop_front();
            }
        }
    } else if (!info->linkId.empty()) {
        try {
            auto link = manager.getLink(info->linkId);
            link->actionQueue.pop_front();
        } catch (std::out_of_range & /* e */) {
            // Link doesn't exist. This should never happen as the action should be deleted if the
            // link is removed.
            helper::logError(logPrefix + "Link doesnt exist for action");
        }
    }
    actions.pop_front();
    updateActionTimestamp();
}

void ComponentActionManager::encodeActions(Timestamp now) {
    auto it = actions.begin();

    // advance past actions we've already encoded for
    while (it != actions.end() && (*it)->action.timestamp < nextEncodeTime) {
        it++;
    }

    // encode content for actions that are coming soon
    while (it != actions.end() && (*it)->action.timestamp < now + maxEncodingTime) {
        manager.encodeForAction(it->get());
        lastEncodeTime = (*it)->action.timestamp;
        it++;
    }

    // set next encode time based on timestamp of the next action, or inf if there is none
    nextEncodeTime = std::numeric_limits<double>::infinity();
    if (it != actions.end()) {
        nextEncodeTime = (*it)->action.timestamp - maxEncodingTime;
    }
}

void ComponentActionManager::joinActionThread() {
    {
        std::unique_lock<std::recursive_mutex> lock(manager.dataMutex);
        actionThreadSignaler.notify_all();
    }

    if (actionThread.joinable()) {
        actionThread.join();
    }
}

double ComponentActionManager::currentTime() {
    return helper::currentTime();
}

static std::string printActions(const std::deque<std::unique_ptr<CMTypes::ActionInfo>> &deque) {
    std::stringstream ss;
    ss << "[";
    for (auto &e : deque) {
        ss << *e << ", ";
    }
    ss << "]";
    return ss.str();
}

std::ostream &operator<<(std::ostream &out, const ComponentActionManager &cam) {
    return out << "ActionManager{"
               << "maxEncodingTime: " << cam.maxEncodingTime
               << ", nextFetchTime: " << cam.nextFetchTime
               << ", nextActionTime: " << cam.nextActionTime
               << ", nextEncodeTime: " << cam.nextEncodeTime
               << ", lastEncodeTime: " << cam.lastEncodeTime
               << ", actions: " << printActions(cam.actions) << "}";
}