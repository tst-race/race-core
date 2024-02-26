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

package com.twosix.race.daemon;

import android.content.Context;
import android.content.Intent;

import com.twosix.race.daemon.actions.BootstrapAction;
import com.twosix.race.daemon.actions.ClearArtifactsAction;
import com.twosix.race.daemon.actions.ClearConfigsAndEtcAction;
import com.twosix.race.daemon.actions.KillAction;
import com.twosix.race.daemon.actions.NodeAction;
import com.twosix.race.daemon.actions.PrepareToBootstrapAction;
import com.twosix.race.daemon.actions.PullConfigsAction;
import com.twosix.race.daemon.actions.PushRuntimeConfigsAction;
import com.twosix.race.daemon.actions.RotateLogsAction;
import com.twosix.race.daemon.actions.SetDaemonConfigAction;
import com.twosix.race.daemon.actions.StartAction;
import com.twosix.race.daemon.sdk.IRaceNodeActionListener;
import com.twosix.race.daemon.sdk.IRaceNodeDaemonSdk;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;

/** Daemon SDK action listener that routes received actions to the appropriate handlers. */
class NodeActionListener implements IRaceNodeActionListener {

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final Context context;

    private final HashMap<String, NodeAction> actions = new HashMap<>();

    NodeActionListener(
            String persona,
            IRaceNodeDaemonSdk sdk,
            RaceNodeStatusPublisher nodeStatusPublisher,
            Context context) {
        this.context = context;

        actions.put(BootstrapAction.TYPE, new BootstrapAction(context, sdk, persona));
        actions.put(KillAction.TYPE, new KillAction(context));
        actions.put(PrepareToBootstrapAction.TYPE, new PrepareToBootstrapAction(context));
        actions.put(PullConfigsAction.TYPE, new PullConfigsAction(sdk, persona));
        actions.put(PushRuntimeConfigsAction.TYPE, new PushRuntimeConfigsAction(sdk));
        actions.put(RotateLogsAction.TYPE, new RotateLogsAction(context, sdk));
        actions.put(StartAction.TYPE, new StartAction(context, sdk));
        actions.put(
                SetDaemonConfigAction.TYPE, new SetDaemonConfigAction(sdk, nodeStatusPublisher));
        actions.put(ClearConfigsAndEtcAction.TYPE, new ClearConfigsAndEtcAction(context, sdk));
        actions.put(ClearArtifactsAction.TYPE, new ClearArtifactsAction(context, sdk));
    }

    /**
     * Parses the action JSON and routes the action to the appropriate handlers.
     *
     * <p>If the action is not recognized as an action that the daemon must execute directly, the
     * action is forwarded to the RACE app via a broadcast intent.
     *
     * @param jsonAction Action JSON string
     */
    @Override
    public void execute(String jsonAction) {
        try {
            JSONObject action = new JSONObject(jsonAction);
            String type = action.getString("type");
            JSONObject payload;
            if (action.has("payload")) {
                payload = action.getJSONObject("payload");
            } else {
                payload = new JSONObject();
            }

            NodeAction handler = actions.get(type);
            if (handler != null) {
                handler.execute(payload);
            } else {
                // Forward the action to the RACE client app
                logger.info("Forwarding action to app: {}", jsonAction);
                Intent intent = new Intent();
                intent.setAction(Constants.RACE_APP_ACTION);
                intent.putExtra("action", jsonAction);
                context.sendBroadcast(intent);
            }

        } catch (Exception e) {
            logger.error("Error executing action", e);
        }
    }
}
