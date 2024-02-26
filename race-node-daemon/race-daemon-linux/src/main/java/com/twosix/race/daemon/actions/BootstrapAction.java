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
import java.net.URL;
import java.time.Duration;

public class BootstrapAction implements NodeAction {
    public static final String TYPE = "bootstrap";
    private static final String BOOTSTRAP_FILE = "/tmp/bootstrap.zip";
    private static final String TMP_DIR = "/tmp/bootstrap";

    private static final int DEFAULT_TIMEOUT_SECS = 600;

    private static final Logger logger = LoggerFactory.getLogger(BootstrapAction.class);

    private final DaemonState state;

    private final NodeAction startAction;

    public BootstrapAction(DaemonState state, NodeAction startAction) {
        this.state = state;
        this.startAction = startAction;
    }

    /**
     * Bootstraps the current node by installing the RACE app from an introducer node.
     *
     * <ol>
     *   <li>Retrieve the bootstrap bundle from the introducer node
     *   <li>Extract the bootstrap bundle
     *   <li>Executes install script from the bootstrap bundle
     * </ol>
     *
     * @param jsonPayload Action payload containing the introducer and passphrase
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        logger.info("Bootstrapping application");
        try {
            String bootstrapBundleUrl = jsonPayload.getString("bootstrap-bundle-url");

            int timeoutSecs = DEFAULT_TIMEOUT_SECS;
            if (jsonPayload.has("timeout-secs")) {
                timeoutSecs = jsonPayload.getInt("timeout-secs");
            }
            Duration timeout = Duration.ofSeconds(timeoutSecs);

            URL url = new URL(bootstrapBundleUrl);
            File zipFile = new File(BOOTSTRAP_FILE);

            logger.debug("Bootstrap node-daemon looking up: {}", url);

            state.sdk.fetchRemoteFile(
                    url,
                    zipFile,
                    (success) -> {
                        try {
                            if (success) {
                                // untar file
                                File tmp = new File(TMP_DIR);
                                FileUtils.extractZipFile(zipFile, tmp);

                                // execute install script
                                Process process =
                                        new ProcessBuilder("bash", "race/install.sh")
                                                .directory(tmp)
                                                .inheritIO()
                                                .start();
                                int result = process.waitFor();
                                if (result == 0) {
                                    if (!zipFile.delete()) {
                                        logger.warn("Unable to delete " + zipFile);
                                    }
                                    if (!FileUtils.deleteFilesInDirectory(tmp)) {
                                        logger.warn("Unable to delete files in " + tmp);
                                    }
                                    if (!tmp.delete()) {
                                        logger.warn("Unable to delete " + tmp);
                                    }

                                    // payload does not matter for start action
                                    startAction.execute(new JSONObject());
                                } else {
                                    logger.error(
                                            "Install script failed with exit code: {}", result);
                                }
                            }
                        } catch (IOException e) {
                            logger.error(
                                    "Unable to bootstrap node into RACE network: "
                                            + e.getMessage());
                        } catch (InterruptedException e) {
                            logger.error("Interrupted during installation");
                        }
                    },
                    timeout);
        } catch (Exception e) {
            logger.error("Error bootstrapping RACE app: {}", e.getMessage());
        }
    }
}
