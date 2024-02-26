
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

#ifndef __MOCK_USER_MODEL_H__
#define __MOCK_USER_MODEL_H__

#include "IUserModelComponent.h"
#include "LogExpect.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockUserModel : public IUserModelComponent {
public:
    MockUserModel(LogExpect &logger, IUserModelSdk &sdk) : logger(logger), sdk(sdk) {
        using ::testing::_;
        ON_CALL(*this, getUserModelProperties()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getUserModelProperties");
            return UserModelProperties{};
        });
        ON_CALL(*this, addLink(_, _))
            .WillByDefault([this](const LinkID &link, const LinkParameters &params) {
                LOG_EXPECT(this->logger, "addLink", link, params);
                return ComponentStatus::COMPONENT_OK;
            });
        ON_CALL(*this, removeLink(_)).WillByDefault([this](const LinkID &link) {
            LOG_EXPECT(this->logger, "removeLink", link);
            return ComponentStatus::COMPONENT_OK;
        });
        ON_CALL(*this, getTimeline(_, _))
            .WillByDefault([this](Timestamp startTime, Timestamp endTime) {
                auto start = "<Timestamp>";
                auto range = endTime - startTime;
                LOG_EXPECT(this->logger, "getTimeline", start, range);
                return ActionTimeline{};
            });
        ON_CALL(*this, onTransportEvent(_)).WillByDefault([this](const Event &event) {
            LOG_EXPECT(this->logger, "onTransportEvent", event);
            return ComponentStatus::COMPONENT_OK;
        });
        ON_CALL(*this, onUserInputReceived(_, _, _))
            .WillByDefault([this](RaceHandle handle, bool answered, const std::string &response) {
                LOG_EXPECT(this->logger, "onUserInputReceived", handle, answered, response);
                return ComponentStatus::COMPONENT_OK;
            });
    }

    MOCK_METHOD(UserModelProperties, getUserModelProperties, (), (override));
    MOCK_METHOD(ComponentStatus, addLink, (const LinkID &link, const LinkParameters &params),
                (override));
    MOCK_METHOD(ComponentStatus, removeLink, (const LinkID &link), (override));
    MOCK_METHOD(ActionTimeline, getTimeline, (Timestamp start, Timestamp end), (override));
    MOCK_METHOD(ComponentStatus, onTransportEvent, (const Event &event), (override));
    MOCK_METHOD(ComponentStatus, onUserInputReceived,
                (RaceHandle handle, bool answered, const std::string &response), (override));

public:
    LogExpect &logger;
    IUserModelSdk &sdk;
};

#endif