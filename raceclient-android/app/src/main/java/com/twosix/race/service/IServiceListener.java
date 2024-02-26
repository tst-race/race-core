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

public interface IServiceListener {

    /** Invoked when the RACE app is stopped. */
    public void onStopApp();

    /**
     * Invoked when the RACE status changes.
     *
     * @param networkManagerStatus network manager plugin status
     */
    public void onRaceStatusChange(PluginStatus networkManagerStatus);

    /**
     * Invoked when a message is sent or received.
     *
     * @param message Clear message
     */
    public void onMessageAdded(JClrMsg message);

    /**
     * Invoked when user input is requested by a RACE plugin.
     *
     * @param handle RACE handle to be used for response callback
     * @param prompt Input prompt message
     */
    public void onUserInputRequested(RaceHandle handle, String prompt);

    /**
     * Invoked when user display is requested by a RACE plugin.
     *
     * @param handle RACE handle to be used for acknowledgement callback
     * @param message Display message
     * @param displayType Display type
     */
    public void onDisplayAlert(RaceHandle handle, String message, UserDisplayType displayType);

    /**
     * Invoked when bootstrap info display is requested by a RACE plugin.
     *
     * @param handle RACE handle to be used for acknowledgement callback
     * @param message Display message
     * @param displayType Display type
     * @param actionType Bootstrap action/step
     */
    public void onDisplayBootstrapInfo(
            RaceHandle handle,
            String message,
            UserDisplayType displayType,
            BootstrapActionType actionType);
}
