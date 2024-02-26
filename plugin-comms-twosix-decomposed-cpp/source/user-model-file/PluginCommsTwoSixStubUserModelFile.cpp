
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

#include "PluginCommsTwoSixStubUserModelFile.h"

#include <time.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>

#include "JsonTypes.h"
#include "log.h"
#include "nlohmann/json.hpp"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ActionEntry, timestamp, relative, json);

PluginCommsTwoSixStubUserModelFile::PluginCommsTwoSixStubUserModelFile(
    IUserModelSdk *sdk, const PluginConfig &pluginConfig) :
    sdk(sdk), pluginConfig(pluginConfig) {
    TRACE_METHOD();

    std::chrono::duration<double> sinceEpoch =
        std::chrono::high_resolution_clock::now().time_since_epoch();
    startTime = sinceEpoch.count();

    sdk->requestPluginUserInput("actionFile", "Location of action file", false);
}

ComponentStatus PluginCommsTwoSixStubUserModelFile::onUserInputReceived(
    RaceHandle handle, bool answered, const std::string &response) {
    TRACE_METHOD(handle, answered, response);

    std::string filename;
    if (answered) {
        filename = response;
    } else {
        filename = pluginConfig.pluginDirectory + "/actions.json";
    }
    loadActionFile(filename);
    return COMPONENT_OK;
}

void PluginCommsTwoSixStubUserModelFile::loadActionFile(std::string filename) {
    TRACE_METHOD();

    try {
        std::ifstream ifs(filename);
        allActions = nlohmann::json::parse(ifs);
        this->sdk->updateState(COMPONENT_STATE_STARTED);
    } catch (std::exception &e) {
        logError(logPrefix + "Failed to parse '" + filename + "': " + e.what());
        this->sdk->updateState(COMPONENT_STATE_FAILED);
    }
}

UserModelProperties PluginCommsTwoSixStubUserModelFile::getUserModelProperties() {
    TRACE_METHOD();
    return {};
}

ComponentStatus PluginCommsTwoSixStubUserModelFile::addLink(const LinkID &link,
                                                            const LinkParameters & /* params */) {
    TRACE_METHOD(link);
    logInfo(logPrefix + "Calling onTimelineUpdated");
    // need to make sure wildcard actions get added to this new link
    sdk->onTimelineUpdated();
    return COMPONENT_OK;
}

ComponentStatus PluginCommsTwoSixStubUserModelFile::removeLink(const LinkID &link) {
    TRACE_METHOD(link);
    return COMPONENT_OK;
}

ActionTimeline PluginCommsTwoSixStubUserModelFile::getTimeline(Timestamp start, Timestamp end) {
    TRACE_METHOD(start, end);

    std::chrono::duration<double> sinceEpoch =
        std::chrono::high_resolution_clock::now().time_since_epoch();
    double now = sinceEpoch.count();

    std::time_t ctime = std::time(nullptr);
    std::tm localtime;
    localtime_r(&ctime, &localtime);

    double secondsSinceStartOfDay =
        localtime.tm_hour * 3600 + localtime.tm_min * 60 + localtime.tm_sec;
    double startOfDay = now - secondsSinceStartOfDay;

    ActionTimeline timeline;
    for (auto &action : allActions) {
        if (!action.relative) {
            continue;
        }

        double timestamp = startTime + action.timestamp;
        if (timestamp >= start && timestamp <= end) {
            Action newAction;
            newAction.actionId = nextActionId++;
            newAction.json = action.json.dump();
            newAction.timestamp = timestamp;
            timeline.push_back(newAction);
        }
    }

    while (startOfDay < end) {
        for (auto &action : allActions) {
            if (action.relative) {
                continue;
            }

            double timestamp = startOfDay + action.timestamp;
            if (timestamp >= start && timestamp <= end) {
                Action newAction;
                newAction.actionId = nextActionId++;
                newAction.json = action.json.dump();
                newAction.timestamp = timestamp;
                timeline.push_back(newAction);
            }
        }

        // advance the day
        startOfDay += 86400;
    }

    std::sort(timeline.begin(), timeline.end(), [](const Action &a, const Action &b) {
        return a.timestamp < b.timestamp || (a.timestamp == b.timestamp && a.actionId < b.actionId);
    });

    return timeline;
}

ComponentStatus PluginCommsTwoSixStubUserModelFile::onTransportEvent(const Event & /* event */) {
    TRACE_METHOD();
    return COMPONENT_OK;
}

#ifndef TESTBUILD
IUserModelComponent *createUserModel(const std::string &usermodel, IUserModelSdk *sdk,
                                     const std::string &roleName,
                                     const PluginConfig &pluginConfig) {
    TRACE_FUNCTION(usermodel, roleName, pluginConfig.pluginDirectory);
    return new PluginCommsTwoSixStubUserModelFile(sdk, pluginConfig);
}
void destroyUserModel(IUserModelComponent *component) {
    TRACE_FUNCTION();
    delete component;
}

const RaceVersionInfo raceVersion = RACE_VERSION;
#endif