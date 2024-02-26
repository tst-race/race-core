
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

#ifndef __TEST_HARNESS_WRAPPER_H__
#define __TEST_HARNESS_WRAPPER_H__

#include "NMWrapper.h"

class PluginNMTestHarness;
class RaceSdk;

class TestHarnessWrapper : public NMWrapper {
protected:
    std::shared_ptr<PluginNMTestHarness> mTestHarness;

public:
    explicit TestHarnessWrapper(RaceSdk &sdk);

    virtual bool isTestHarness() const override {
        return true;
    }

    /* processNMBypassMsg: call processNMBypassMsg on the wrapped test harness plugin
     *
     * processNMBypassMsg will be called on the plugin thread. processNMBypassMsg may return
     * before the processNMBypassMsg method of the wrapped plugin is complete.
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    std::tuple<bool, double> processNMBypassMsg(RaceHandle handle, const ClrMsg &msg,
                                                const std::string &route, int32_t timeout);

    /* openRecvConnection: call openRecvConnection on the wrapped test harness plugin
     *
     * openRecvConnection will be called on the plugin thread. openRecvConnection may return
     * before the openRecvConnection method of the wrapped plugin is complete.
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    std::tuple<bool, double> openRecvConnection(RaceHandle handle, const std::string &persona,
                                                const std::string &route, int32_t timeout);

    /**
     * @brief Call rpcDeactivateChannel on the wrapped test harness plugin
     *
     * rpcDeactivateChannel will be called on the plugin thread. rpcDeactivateChannel may return
     * before the rpcDeactivateChannel method of the wrapped plugin is complete.
     *
     * @param channelGid ID of the channel to deactivate
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    std::tuple<bool, double> rpcDeactivateChannel(const std::string &channelGid, int32_t timeout);

    /**
     * @brief Call rpcDestroyLink on the wrapped test harness plugin
     *
     * rpcDestroyLink will be called on the plugin thread. rpcDestroyLink may return
     * before the rpcDestroyLink method of the wrapped plugin is complete.
     *
     * @param linkId ID of the link to destroy
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    std::tuple<bool, double> rpcDestroyLink(const std::string &linkId, int32_t timeout);

    /**
     * @brief Call rpcCloseConnection on the wrapped test harness plugin
     *
     * rpcCloseConnection will be called on the plugin thread. rpcCloseConnection may return
     * before the rpcCloseConnection method of the wrapped plugin is complete.
     *
     * @param connectionId ID of the connection to close
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    std::tuple<bool, double> rpcCloseConnection(const std::string &connectionId, int32_t timeout);
};

#endif