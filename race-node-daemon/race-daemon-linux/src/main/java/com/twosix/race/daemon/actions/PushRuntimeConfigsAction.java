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

/**
 * Action to get configs from a stopped deployment and push them to the file server for RiB to
 * download locally. This action is sent from rib when the user runs the `rib deployment <mode>
 * config pull-runtime` command.
 */
public class PushRuntimeConfigsAction implements NodeAction {
    public static final String TYPE = "push-runtime-configs";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final DaemonState state;

    public PushRuntimeConfigsAction(DaemonState state) {
        this.state = state;
    }

    @Override
    public void execute(JSONObject jsonPayload) {
        logger.info("Pushing runtime configs...");

        if (!jsonPayload.has("name")) {
            logger.error("Must provide a name when pushing runtime configs");
            return;
        }
        String configName = jsonPayload.getString("name");

        File configsDir = new File("/data");
        if (configsDir != null && configsDir.exists()) {
            // TODO: note that the encryption key location is hardcoded across the node daemaon and
            // racesdk/core. This is not ideal, but being that the node daemon is purely a testing
            // application I can live with it for now. An alternative option would be to define it
            // in an environment variable in raceclient and racetestapp, then read the location
            // here. Not sure if it's really worth the effort though. -GP
            state.sdk.pushRuntimeConfigs(configsDir, configName, "/etc/race/file_key");
        } else {
            logger.error("Runtime configs directory " + configsDir.toString() + " does not exist");
            return;
        }

        logger.info("Pushing runtime configs complete");
    }
}
