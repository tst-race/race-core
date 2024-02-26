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

import com.twosix.race.daemon.RaceNodeStatusPublisher;
import com.twosix.race.daemon.sdk.IRaceNodeDaemonSdk;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/** Action to apply configuration changes to the daemon. */
public class SetDaemonConfigAction implements NodeAction {
    public static final String TYPE = "set-daemon-config";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final IRaceNodeDaemonSdk sdk;

    private final RaceNodeStatusPublisher nodeStatusPublisher;

    public SetDaemonConfigAction(
            IRaceNodeDaemonSdk sdk, RaceNodeStatusPublisher nodeStatusPublisher) {
        this.sdk = sdk;
        this.nodeStatusPublisher = nodeStatusPublisher;
    }

    /**
     * Set the Daemon Config for this node
     *
     * @param jsonPayload Action payload
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        try {
            logger.info("Setting Daemon Config");

            Long period = null;
            Long ttlFactor = null;

            if (jsonPayload.has("deployment-name")) {
                String deploymentName = jsonPayload.getString("deployment-name");
                if (!sdk.saveDaemonStateInfo("deployment-name", deploymentName)) {
                    logger.error("failed to set deployment name");
                }
            }

            if (jsonPayload.has("genesis")) {
                boolean genesis = jsonPayload.getBoolean("genesis");
                if (!sdk.saveDaemonStateInfo("genesis", genesis)) {
                    logger.error("failed to set genesis");
                }
            }

            if (jsonPayload.has("app")) {
                String app = jsonPayload.getString("app");
                if (!sdk.saveDaemonStateInfo("app", app)) {
                    logger.error("failed to set app");
                }
            }

            if (jsonPayload.has("period")) {
                period = jsonPayload.getLong("period");
                if (!sdk.saveDaemonStateInfo("period", period)) {
                    logger.error("failed to set period");
                }
            }

            if (jsonPayload.has("ttl-factor")) {
                ttlFactor = jsonPayload.getLong("ttl-factor");
                if (!sdk.saveDaemonStateInfo("ttl-factor", ttlFactor)) {
                    logger.error("failed to set ttl-factor");
                }
            }

            this.nodeStatusPublisher.start(period, ttlFactor);

        } catch (Exception e) {
            logger.error("Error setting Daemon Config: {}", e.getMessage());
        }
    }
}
