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

package com.twosix.race.service;

import ShimsJava.BootstrapActionType;
import ShimsJava.JClrMsg;
import ShimsJava.PluginStatus;
import ShimsJava.RaceHandle;
import ShimsJava.UserDisplayType;

/** Implementation of IServiceListener with no-op methods */
public class ServiceListener implements IServiceListener {

    @Override
    public void onStopApp() {}

    @Override
    public void onRaceStatusChange(PluginStatus networkManagerStatus) {}

    @Override
    public void onMessageAdded(JClrMsg message) {}

    @Override
    public void onUserInputRequested(RaceHandle handle, String prompt) {}

    @Override
    public void onDisplayAlert(RaceHandle handle, String message, UserDisplayType displayType) {}

    @Override
    public void onDisplayBootstrapInfo(
            RaceHandle handle,
            String message,
            UserDisplayType displayType,
            BootstrapActionType actionType) {}
}
