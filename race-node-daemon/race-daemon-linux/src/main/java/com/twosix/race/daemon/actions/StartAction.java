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

package com.twosix.race.daemon.actions;

import com.twosix.race.daemon.Constants;
import com.twosix.race.daemon.DaemonState;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class StartAction implements NodeAction {
    public static final String TYPE = "start";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final DaemonState state;

    public StartAction(DaemonState state) {
        this.state = state;
    }

    /**
     * Starts the RACE app.
     *
     * @param jsonPayload Action payload
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        try {
            if (!state.sdk.isAppAlive()) {
                logger.info("Starting RACE app");
                String executable =
                        Constants.PATH_TO_TA3_ARTIFACTS
                                + (String) state.sdk.getDaemonStateInfo("app");
                state.raceapp = new ProcessBuilder(executable).inheritIO().start();
                logger.info("Started RACE app");
            } else {
                logger.info("Received request to start, but RACE app already started");
            }
        } catch (Exception e) {
            logger.error("Error starting RACE app: {}", e.getMessage());
        }
    }
}
