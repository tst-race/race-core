
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

#ifndef __COMMS_TWOSIX_USER_MODEL_H__
#define __COMMS_TWOSIX_USER_MODEL_H__

#include <IUserModelComponent.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>

#include "JsonTypes.h"

// The action as specified in the input file
struct ActionEntry {
    double timestamp;
    bool relative;
    nlohmann::json json;
};

class PluginCommsTwoSixStubUserModelFile : public IUserModelComponent {
public:
    explicit PluginCommsTwoSixStubUserModelFile(IUserModelSdk *sdk,
                                                const PluginConfig &pluginConfig);

    virtual ComponentStatus onUserInputReceived(RaceHandle handle, bool answered,
                                                const std::string &response) override;

    virtual UserModelProperties getUserModelProperties() override;

    virtual ComponentStatus addLink(const LinkID &link, const LinkParameters &params) override;

    virtual ComponentStatus removeLink(const LinkID &link) override;

    virtual ActionTimeline getTimeline(Timestamp start, Timestamp end) override;

    virtual ComponentStatus onTransportEvent(const Event &event) override;

protected:
    void loadActionFile(std::string);

    IUserModelSdk *sdk;
    PluginConfig pluginConfig;

    std::vector<ActionEntry> allActions;
    double startTime;

    uint64_t nextActionId{0};
};

#endif  // __COMMS_TWOSIX_USER_MODEL_H__