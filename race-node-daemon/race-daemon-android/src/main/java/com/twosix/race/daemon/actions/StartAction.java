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

import android.content.Context;
import android.content.Intent;

import com.twosix.race.daemon.Constants;
import com.twosix.race.daemon.sdk.IRaceNodeDaemonSdk;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class StartAction implements NodeAction {

    public static final String TYPE = "start";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final Context context;

    private final IRaceNodeDaemonSdk sdk;

    public StartAction(Context context, IRaceNodeDaemonSdk sdk) {
        this.context = context;
        this.sdk = sdk;
    }

    /**
     * Starts the RACE app.
     *
     * <ol>
     *   <li>Retrieve configs from the RiB file server
     *   <li>Move configs to shared external storage
     *   <li>Send a launch intent to start the app
     * </ol>
     *
     * @param jsonPayload Action payload
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        try {
            // Check if app is already running before trying to start again
            if (sdk.isAppAlive()) {
                logger.info("Received request to start, but RACE app already started");
                return;
            }

            // Start the app
            try {
                logger.info("Starting RACE app");
                Intent intent =
                        context.getPackageManager()
                                .getLaunchIntentForPackage(Constants.RACE_APP_PACKAGE);
                context.startActivity(intent);
                logger.info("Started RACE app");
            } catch (Exception e) {
                logger.error("Failed to send launch intent", e);
            }
        } catch (Exception e) {
            logger.error("Error starting RACE app: {}", e.getMessage());
        }
    }
}
