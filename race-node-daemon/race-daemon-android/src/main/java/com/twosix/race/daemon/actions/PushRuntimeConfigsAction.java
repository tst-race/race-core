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

import com.twosix.race.daemon.sdk.IRaceNodeDaemonSdk;

import org.json.JSONException;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;

/**
 * Action to get configs from a stopped deployment and push them to the file server for RiB to
 * download locally. This action is sent from rib when the user runs the `rib deployment <mode>
 * config pull-runtime` command.
 */
public class PushRuntimeConfigsAction implements NodeAction {
    public static final String TYPE = "push-runtime-configs";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final IRaceNodeDaemonSdk sdk;

    public PushRuntimeConfigsAction(IRaceNodeDaemonSdk sdk) {
        this.sdk = sdk;
    }

    @Override
    public void execute(JSONObject jsonPayload) {
        logger.info("Pushing runtime configs...");

        if (!jsonPayload.has("name")) {
            logger.error("Must provide a name when pushing runtime configs");
            return;
        }

        String configName;
        try {
            configName = jsonPayload.getString("name");
        } catch (JSONException e) {
            logger.error(
                    "PushRuntimeConfigsAction: name must be provided in json payload: "
                            + e.getMessage());
            return;
        }

        File configsDir =
                new File("/storage/self/primary/Android/data/com.twosix.race/files/race/data/");
        if (configsDir != null && configsDir.exists()) {
            // TODO: note that the encryption key location is hardcoded across the node daemaon and
            // racesdk/core. This is not ideal, but being that the node daemon is purely a testing
            // application I can live with it for now. An alternative option would be to define it
            // in an environment variable in raceclient and racetestapp, then read the location
            // here. Not sure if it's really worth the effort though. -GP
            this.sdk.pushRuntimeConfigs(
                    configsDir, configName, "/storage/self/primary/Download/race/etc/file_key");
        } else {
            logger.error("Runtime configs directory " + configsDir.toString() + " does not exist");
            return;
        }

        logger.info("Pushing runtime configs complete");
    }
}
