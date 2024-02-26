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
import com.twosix.race.daemon.sdk.FileUtils;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;

public class ClearArtifactsAction implements NodeAction {
    public static final String TYPE = "clear-artifacts";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final DaemonState state;

    public ClearArtifactsAction(DaemonState state) {
        this.state = state;
    }

    /**
     * Clear RACE Artifacts from the node.
     *
     * @param jsonPayload Action payload
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        try {
            logger.info("Clear Artifacts from the node");

            // uninstall app if bootstrap node
            File raceDir = new File("/usr/local/lib/race/core/race");
            if (raceDir.isDirectory()) {
                FileUtils.deleteFilesInDirectory(raceDir);
            }
            String[] pluginTypes = {"network-manager", "comms", "artifact-manager"};
            for (String pluginType : pluginTypes) {
                File pluginDir = new File("/usr/local/lib/race/" + pluginType);
                if (pluginDir.isDirectory()) {
                    FileUtils.deleteFilesInDirectory(pluginDir);
                }
            }
            logger.info("Cleared Artifacts from the node");
        } catch (Exception e) {
            logger.error("Error clearing artifacts from node: {}", e.getMessage());
        }
    }
}
