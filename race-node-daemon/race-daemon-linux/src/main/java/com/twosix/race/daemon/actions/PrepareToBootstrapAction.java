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

public class PrepareToBootstrapAction implements NodeAction {
    public static final String TYPE = "prepare-to-bootstrap";

    private static final Logger logger = LoggerFactory.getLogger(PrepareToBootstrapAction.class);

    private final DaemonState state;

    public PrepareToBootstrapAction(DaemonState state) {
        this.state = state;
    }

    @Override
    public void execute(JSONObject jsonPayload) {
        logger.info("Bootstrapping application");
        try {
            // Remove the target from the payload since the RACE app shouldn't need to know it but
            // it was in the original payload because the daemon needs it in order to know to where
            // to send bootstrap info
            String target = (String) jsonPayload.remove("target");
            if (target == null || target.isEmpty()) {
                logger.error("No target specified to bootstrap");
                return;
            }
            this.state.currentBootstrapTarget = target;

            // Recreate original json action to forward on to the sdk
            JSONObject originalJsonAction = new JSONObject();
            originalJsonAction.put("payload", jsonPayload);
            originalJsonAction.put("type", PrepareToBootstrapAction.TYPE);
            this.state.communicator.sendToApp(originalJsonAction.toString());

        } catch (Exception e) {
        }
    }
}
