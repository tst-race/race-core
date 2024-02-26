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

import com.twosix.race.daemon.DaemonState;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;

public class LogsAction implements NodeAction {
    public static final String TYPE = "rotate-logs";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final DaemonState state;

    public LogsAction(DaemonState state) {
        this.state = state;
    }

    /**
     * Backs up and/or deletes logs from the RACE app.
     *
     * <p>If the action payload contains a backup ID, the RACE app's logs are uploaded to the RiB
     * file server. If the action payload contains the delete indicator, then the logs are also
     * deleted.
     *
     * @param jsonPayload Action payload
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        logger.info("Rotating logs");

        String backupId = "";
        if (jsonPayload.has("backup-id")) {
            backupId = jsonPayload.getString("backup-id");
        }

        boolean delete = true;
        if (jsonPayload.has("delete")) {
            delete = jsonPayload.getBoolean("delete");
        }

        File logsDir = new File("/log");
        if (logsDir != null && logsDir.exists()) {
            state.sdk.rotateLogs(logsDir, delete, backupId);
        } else {
            logger.warn("Logs directory does not exist");
        }
        logger.info("Rotating logs complete");
    }
}
