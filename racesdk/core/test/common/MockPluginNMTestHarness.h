
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

#ifndef __MOCK_PLUGIN_NETWORK_MANAGER_TEST_HARNESS_H_
#define __MOCK_PLUGIN_NETWORK_MANAGER_TEST_HARNESS_H_

#include "MockRacePluginNM.h"
#include "PluginNMTestHarness.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockPluginNMTestHarness : public PluginNMTestHarness, public MockRacePluginNM {
public:
    MockPluginNMTestHarness(IRaceSdkNM *sdk) : PluginNMTestHarness(sdk) {}

    MOCK_METHOD(PluginResponse, processNMBypassMsg,
                (RaceHandle handle, const std::string &route, const ClrMsg &msg), (override));
    MOCK_METHOD(PluginResponse, openRecvConnection,
                (RaceHandle handle, const std::string &persona, const std::string &route),
                (override));
    MOCK_METHOD(PluginResponse, rpcDeactivateChannel, (const std::string &channelGid), (override));
    MOCK_METHOD(PluginResponse, rpcDestroyLink, (const std::string &linkId), (override));
    MOCK_METHOD(PluginResponse, rpcCloseConnection, (const std::string &connectionId), (override));
};

#endif