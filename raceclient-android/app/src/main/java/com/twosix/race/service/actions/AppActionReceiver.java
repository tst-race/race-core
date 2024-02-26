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

package com.twosix.race.service.actions;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;

import com.twosix.race.service.AndroidRaceApp;

import org.json.JSONObject;

import java.util.HashMap;

public class AppActionReceiver extends BroadcastReceiver {

    private static final String TAG = "AppActionReceiver";
    private static final String RACE_APP_ACTION = "com.twosix.race.APP_ACTION";

    private static AppActionReceiver instance;

    private final HashMap<String, AppAction> actions = new HashMap<>();

    private AndroidRaceApp raceApp;

    private AppActionReceiver() {
        super();
        actions.put(StopAction.TYPE, new StopAction());
    }

    /**
     * Registers the app action broadcast receiver.
     *
     * <p>This is done dynamically (i.e., not in the manifest) to ensure that these intents are only
     * received while the app's foreground service is running.
     *
     * @param context Application context
     */
    public static AppActionReceiver initReceiver(Context context) {
        if (instance != null) {
            Log.e(TAG, "Action receiver has already been initialized");
            return instance;
        }

        instance = new AppActionReceiver();

        IntentFilter filter = new IntentFilter();
        filter.addAction(RACE_APP_ACTION);
        context.getApplicationContext().registerReceiver(instance, filter);

        return instance;
    }

    public void setRaceApp(AndroidRaceApp raceApp) {
        this.raceApp = raceApp;
    }

    /**
     * Executes the action contained in the received intent, or forwards the action to the RACE SDK.
     *
     * @param context Intent context
     * @param intent Received intent, sent by the RACE node daemon
     */
    @Override
    public void onReceive(Context context, Intent intent) {
        try {
            String jsonAction = intent.getStringExtra("action");
            JSONObject action = new JSONObject(jsonAction);
            String type = action.getString("type");
            JSONObject payload;
            if (action.has("payload")) {
                payload = action.getJSONObject("payload");
            } else {
                payload = new JSONObject();
            }

            AppAction handler = actions.get(type);
            if (handler != null) {
                handler.execute(context, payload);
            } else if (raceApp != null) {
                // Forward the action to the RACE SDK
                Log.i(TAG, "Forwarding action to RACE SDK: " + jsonAction);
                raceApp.processRaceTestAppCommand(jsonAction);
            } else {
                Log.w(TAG, "RACE SDK is not yet created, cannot forward action: " + jsonAction);
            }

        } catch (Exception e) {
            Log.e(TAG, "Error executing action", e);
        }
    }
}
