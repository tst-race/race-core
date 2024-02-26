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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

import com.twosix.race.daemon.sdk.IRaceNodeDaemonSdk;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/** Broadcast intent receiver for broadcast info published by the RACE app */
public class BootstrapInfoReceiver extends BroadcastReceiver {

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final IRaceNodeDaemonSdk sdk;

    BootstrapInfoReceiver(IRaceNodeDaemonSdk sdk) {
        this.sdk = sdk;
    }

    /**
     * Forwards the bootstrap info to the target node through the daemon SDK.
     *
     * @param context Intent context
     * @param intent Received intent
     */
    @Override
    public void onReceive(Context context, Intent intent) {
        String message = intent.getStringExtra("message");
        if (message == null || message.isEmpty()) {
            logger.error("No message in received intent, unable to forward bootstrap info");
            return;
        }

        String actionType = intent.getStringExtra("actionType");
        if (actionType == null || actionType.isEmpty()) {
            logger.error("No action type in received intent, unable to forward bootstrap info");
            return;
        }

        SharedPreferences preferences =
                context.getSharedPreferences(Constants.PREFERENCES, Context.MODE_PRIVATE);
        String currentTarget = preferences.getString(Constants.CURRENT_BOOTSTRAP_TARGET, "");

        if (currentTarget.isEmpty()) {
            logger.error("No current bootstrap target, unable to forward bootstrap info to target");
            return;
        }

        if (actionType.equals("BS_COMPLETE")) {
            // Clear out the current bootstrap target
            SharedPreferences.Editor editor = preferences.edit();
            editor.remove(Constants.CURRENT_BOOTSTRAP_TARGET);
            editor.apply();
        } else {
            sdk.sendBootstrapInfo(currentTarget, message, actionType);
        }
    }
}
