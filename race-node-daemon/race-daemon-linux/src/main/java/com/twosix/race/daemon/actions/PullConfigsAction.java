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
import java.io.IOException;
import java.time.Duration;

/**
 * Action to pull configs from the file server. Currently triggered by RiB during a `deployment up`.
 */
public class PullConfigsAction implements NodeAction {
    public static final String TYPE = "pull-configs";

    private static final int DEFAULT_TIMEOUT_SECS = 120;

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final DaemonState state;

    public PullConfigsAction(DaemonState state) {
        this.state = state;
    }

    /**
     * Pulls config files from the file server.
     *
     * @param jsonPayload Action payload
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        logger.info("Pulling configs and etc");

        try {
            Duration timeout = Duration.ofSeconds(DEFAULT_TIMEOUT_SECS);
            String deploymentName = jsonPayload.getString("deployment-name");

            // Save deployment name
            if (!this.state.sdk.saveDaemonStateInfo("deployment", deploymentName)) {
                logger.error("failed to set deployment name");
                return;
            }

            // Save Genesis Status
            if (!this.state.sdk.saveDaemonStateInfo(
                    "isGenesis", "" + this.state.sdk.getIsGenesis())) {
                logger.error("failed to set isGenesis");
                return;
            }
            // TODO: Rather than check genesis, add another field in the payload to specify
            // whether or not this node should pull configs based on OrchestrationPrerequisites
            if (!this.state.sdk.getIsGenesis()) {
                logger.info("RACE is not installed, not pulling configs");
            } else {
                logger.info("Pulling configs");
                File configsTarFile = new File(FileUtils.getTempDirectory(), "configs.tar.gz");
                String configsFileToFetch =
                        deploymentName + "_" + this.state.persona + "_configs.tar.gz";
                this.state.sdk.fetchFromFileServer(
                        configsFileToFetch,
                        configsTarFile,
                        (success) -> {
                            if (success) {
                                logger.debug("Downloaded configs to " + configsTarFile.toString());

                                // Save Configs Tar Name
                                if (!state.sdk.saveDaemonStateInfo(
                                        "configsTar", configsFileToFetch)) {
                                    logger.error("failed to set Configs Tar name");
                                    return;
                                }
                            } else {
                                logger.error("Unable to pull RACE configs");
                            }
                        },
                        timeout);
            }

            logger.info("Pulling etc");
            File etcTarFile = new File(FileUtils.getTempDirectory(), "etc.tar.gz");
            String etcFileToFetch = deploymentName + "_" + this.state.persona + "_etc.tar.gz";
            this.state.sdk.fetchFromFileServer(
                    etcFileToFetch,
                    etcTarFile,
                    (success) -> {
                        if (success) {
                            // Extract the etc archive into the config directory.
                            File etcDir = new File("/etc/race");
                            logger.debug(
                                    "extracting "
                                            + etcTarFile.toString()
                                            + " to "
                                            + etcDir.toString()
                                            + " ...");
                            try {
                                FileUtils.extractArchive(etcTarFile, etcDir, true);
                            } catch (IOException e) {
                                logger.error("Failed to extract configs", e);
                                return;
                            }
                            logger.debug("extracted " + etcTarFile + " to " + etcDir);

                            if (!etcTarFile.delete()) {
                                logger.warn("Unable to delete " + etcTarFile);
                            }

                            // Save Etc Tar Name
                            if (!state.sdk.saveDaemonStateInfo("etcTar", etcFileToFetch)) {
                                logger.error("failed to set Etc Tar name");
                                return;
                            }
                        } else {
                            logger.error("Unable to pull RACE etc");
                        }
                    },
                    timeout);
        } catch (Exception e) {
            logger.error("Error pulling configs and etc: {}", e.getMessage());
        }

        logger.info("Pulling configs and etc complete");
    }
}
