
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

#ifndef __MOCK_RACE_CLIENT_H_
#define __MOCK_RACE_CLIENT_H_

#include "IRaceApp.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockRaceApp : public IRaceApp {
public:
    explicit MockRaceApp(IRaceSdkApp *sdk = nullptr) {
        using ::testing::_;
        if (sdk != nullptr) {
            ON_CALL(*this, requestUserInput(_, _, _, _, _))
                .WillByDefault([sdk](RaceHandle handle, const std::string & /*pluginId*/,
                                     const std::string & /*key*/, const std::string & /*prompt*/,
                                     bool /*cache*/) {
                    std::thread([handle, sdk]() {
                        sdk->onUserInputReceived(handle, false, "");
                    }).detach();
                    return SdkResponse{SDK_OK};
                });
        }
    }
    MOCK_METHOD(void, handleReceivedMessage, (ClrMsg msg), (override));
    MOCK_METHOD(void, onMessageStatusChanged, (RaceHandle handle, MessageStatus status),
                (override));
    MOCK_METHOD(SdkResponse, requestUserInput,
                (RaceHandle handle, const std::string &pluginId, const std::string &key,
                 const std::string &prompt, bool cache),
                (override));
    MOCK_METHOD(void, onSdkStatusChanged, (const nlohmann::json &sdkStatus), (override));
    MOCK_METHOD(nlohmann::json, getSdkStatus, (), (override));
    MOCK_METHOD(SdkResponse, displayInfoToUser,
                (RaceHandle handle, const std::string &data,
                 RaceEnums::UserDisplayType displayType),
                (override));
    MOCK_METHOD(SdkResponse, displayBootstrapInfoToUser,
                (RaceHandle handle, const std::string &data, RaceEnums::UserDisplayType displayType,
                 RaceEnums::BootstrapActionType actionType),
                (override));
};

#endif
