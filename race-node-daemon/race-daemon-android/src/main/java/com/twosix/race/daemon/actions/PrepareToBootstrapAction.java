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

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

import com.twosix.race.daemon.Constants;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class PrepareToBootstrapAction implements NodeAction {

    public static final String TYPE = "prepare-to-bootstrap";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final Context context;

    public PrepareToBootstrapAction(Context context) {
        this.context = context;
    }

    /**
     * Saves the target of the current bootstrapping so we can forward bootstrap info to that node
     * later when prompted by the RACE app.
     *
     * @param jsonPayload Action payload
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        try {
            // Remove the target from the payload since the RACE app shouldn't need to know it but
            // it was in the original payload because the daemon needs it in order to know to where
            // to send bootstrap info
            String target = (String) jsonPayload.remove("target");
            if (target == null || target.isEmpty()) {
                logger.error("No target specified to bootstrap");
                return;
            }

            // Save the current target into shared preferences (can only be one at a time, and
            // that's acceptable for now)
            SharedPreferences preferences =
                    context.getSharedPreferences(Constants.PREFERENCES, Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = preferences.edit();
            editor.putString(Constants.CURRENT_BOOTSTRAP_TARGET, target);
            editor.apply();

            JSONObject action = new JSONObject();
            action.put("type", TYPE);
            action.put("payload", jsonPayload);
            String jsonAction = action.toString();

            logger.info("Forwarding action to app: {}", jsonAction);
            Intent intent = new Intent();
            intent.setAction(Constants.RACE_APP_ACTION);
            intent.putExtra("action", jsonAction);
            context.sendBroadcast(intent);
        } catch (Exception e) {
            logger.error("Unable to forward prepare-to-bootstrap action", e);
        }
    }
}
