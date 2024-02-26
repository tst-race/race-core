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

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;

/** Action to set the timezone on the machine. Currently triggered by RiB during a `set time`. */
public class SetTimezoneAction implements NodeAction {
    public static final String TYPE = "set-timezone";

    private static final int DEFAULT_TIMEOUT_SECS = 120;

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final DaemonState state;

    public SetTimezoneAction(DaemonState state) {
        this.state = state;
    }

    /**
     * Sets the timezone on the machine
     *
     * @param jsonPayload Action payload
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        logger.info("Changing the system's timezone");

        try {
            String specifiedTimezone = jsonPayload.getString("timezone");

            // call timedatectl to set the timezone
            String[] processToCall = {"timedatectl", "set-timezone", specifiedTimezone};
            ProcessBuilder pb = new ProcessBuilder(processToCall).inheritIO();
            pb.start();

            // call timedatectl to use returned stream to log new system time
            Process process = Runtime.getRuntime().exec("/usr/bin/timedatectl");
            InputStream stdin = process.getInputStream();
            InputStreamReader isr = new InputStreamReader(stdin);
            BufferedReader br = new BufferedReader(isr);

            logger.info("New system time is: ");

            String temp = null;
            // print timedatectl return stream
            while ((temp = br.readLine()) != null) {
                logger.info(temp);
            }
        } catch (Exception e) {
            logger.error("Error setting timezone", e);
        }

        logger.info("Changing system's timezone complete");
    }
}
