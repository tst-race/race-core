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

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;

import com.twosix.race.daemon.Constants;
import com.twosix.race.daemon.sdk.IRaceNodeDaemonSdk;

import org.json.JSONException;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;

public class RotateLogsAction implements NodeAction {

    public static final String TYPE = "rotate-logs";

    private static final String DELETE_MESSAGES_ACTION = "com.twosix.race.DELETE_MESSAGES_ACTION";
    private static final String DELETE_MESSAGES_RECEIVER =
            "com.twosix.race.actions.DeleteMessagesActionReceiver";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final Context context;
    private final IRaceNodeDaemonSdk sdk;

    public RotateLogsAction(Context context, IRaceNodeDaemonSdk sdk) {
        this.context = context;
        this.sdk = sdk;
    }

    /**
     * Backs up and/or deletes logs from the RACE app.
     *
     * <p>If the action payload contains a backup ID, the RACE app's logs are uploaded to the RiB
     * file server. If the action payload contains the delete indicator, then the logs are also
     * deleted and a broadcast intent is sent to the RACE app to delete its internal databases.
     *
     * @param jsonPayload Action payload
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        try {
            logger.info("Forwarding rotate logs action to app");

            String backupId = "";
            if (jsonPayload.has("backup-id")) {
                backupId = jsonPayload.getString("backup-id");
            }

            boolean delete = true;
            if (jsonPayload.has("delete")) {
                delete = jsonPayload.getBoolean("delete");
            }

            try {
                Context appContext = context.createPackageContext(Constants.RACE_APP_PACKAGE, 0);
                File logsDir = appContext.getExternalFilesDir("logs");
                if (logsDir != null && logsDir.exists()) {
                    sdk.rotateLogs(logsDir, delete, backupId);
                } else {
                    logger.warn("Logs directory does not exist");
                }
            } catch (NameNotFoundException e) {
                logger.warn(
                        "Unable to create package context for RACE app, it may not be installed");
            }

            // We need to tell the app to clear out its internal messages database
            if (delete) {
                logger.info("Sending delete-messages action to app");
                Intent intent = new Intent();
                intent.setAction(DELETE_MESSAGES_ACTION);
                intent.setComponent(
                        new ComponentName(Constants.RACE_APP_PACKAGE, DELETE_MESSAGES_RECEIVER));
                context.sendBroadcast(intent);
            }
        } catch (JSONException e) {
            logger.error("Unable to parse rotate logs payload: " + e.getMessage());
        }
    }
}
