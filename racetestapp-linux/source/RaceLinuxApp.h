
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

#ifndef _RACE_LINUX_APP_H_
#define _RACE_LINUX_APP_H_

#include "racetestapp/RaceApp.h"
#include "output/NodeDaemonPublisher.h"

class RaceLinuxApp : public RaceApp {
public:

    /**
     * @brief Constructor.
     *
     * @param appOutput The application output used for logging received messages.
     * @param _raceSdk The sdk reference
     * @param tracer The opentracing tracer used for logging received messages.
     * @param _nodeDaemonPublisher Publisher used to communicate to the RACE Node Daemon
     */
    explicit RaceLinuxApp(IRaceTestAppOutput &appOutput, IRaceSdkApp &_raceSdk,
                     std::shared_ptr<opentracing::Tracer> tracer, NodeDaemonPublisher &_nodeDaemonPublisher);


    /**
     * @brief Displays information to the User and forward information to target node for
     * automated testing
     *
     * @param handle The RaceHandle to use for the onUserInputReceived callback
     * @param data data to display
     * @param displayType type of user display to display data in
     * @param actionType type of action the Daemon must take
     * @return SdkResponse
     */
    SdkResponse displayBootstrapInfoToUser(RaceHandle handle, const std::string &data,
                                           RaceEnums::UserDisplayType displayType,
                                           RaceEnums::BootstrapActionType actionType) override;

private:
    NodeDaemonPublisher &nodeDaemonPublisher;

};

#endif
