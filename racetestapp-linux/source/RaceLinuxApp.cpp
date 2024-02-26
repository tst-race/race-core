
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

#include "RaceLinuxApp.h"

#include "racetestapp/raceTestAppHelpers.h"

RaceLinuxApp::RaceLinuxApp(IRaceTestAppOutput &appOutput, IRaceSdkApp &_raceSdk,
                           std::shared_ptr<opentracing::Tracer> tracer,
                           NodeDaemonPublisher &_nodeDaemonPublisher) :
    RaceApp(appOutput, _raceSdk, tracer), nodeDaemonPublisher(_nodeDaemonPublisher) {}

SdkResponse RaceLinuxApp::displayBootstrapInfoToUser(RaceHandle handle, const std::string &data,
                                                     RaceEnums::UserDisplayType /*displayType*/,
                                                     RaceEnums::BootstrapActionType actionType) {
    rtah::logDebug("RaceLinuxApp.displayBootstrapInfoToUser: called with data:  " + data);
    nodeDaemonPublisher.publishBootstrapAction(data, actionType);
    raceSdk.onUserAcknowledgementReceived(handle);
    return SDK_OK;
}
