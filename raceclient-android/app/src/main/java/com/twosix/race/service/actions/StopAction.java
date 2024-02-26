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

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.twosix.race.service.StopServiceReceiver;

import org.json.JSONObject;

public class StopAction implements AppAction {

    public static final String TYPE = "stop";

    private static final String TAG = "StopAction";

    @Override
    public void execute(Context context, JSONObject jsonPayload) {
        try {
            Log.i(TAG, "Stopping RACE app");

            Intent stopIntent = new Intent();
            stopIntent.setAction(StopServiceReceiver.ACTION);
            context.sendBroadcast(stopIntent);

            //            Intent homeIntent = new Intent(Intent.ACTION_MAIN);
            //            homeIntent.addCategory(Intent.CATEGORY_HOME);
            //            homeIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP |
            // Intent.FLAG_ACTIVITY_NEW_TASK);
            //            context.startActivity(homeIntent);
        } catch (Exception e) {
            Log.e(TAG, "Error stopping RACE app: " + e.getMessage());
        }
    }
}
