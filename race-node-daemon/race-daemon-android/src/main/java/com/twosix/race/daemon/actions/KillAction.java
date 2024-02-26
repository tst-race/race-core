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

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;

import com.twosix.race.daemon.Constants;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class KillAction implements NodeAction {

    public static final String TYPE = "kill";

    private static final String KILL_ACTION = "com.twosix.race.KILL_ACTION";
    private static final String KILL_RECEIVER = "com.twosix.race.actions.KillActionReceiver";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final Context context;

    public KillAction(Context context) {
        this.context = context;
    }

    /**
     * Sends a broadcast intent to the RACE app to forcibly kill itself.
     *
     * @param jsonPayload Action payload
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        logger.info("Forwarding kill action to app");
        Intent intent = new Intent();
        intent.setAction(KILL_ACTION);
        intent.setComponent(new ComponentName(Constants.RACE_APP_PACKAGE, KILL_RECEIVER));
        context.sendBroadcast(intent);
    }
}
