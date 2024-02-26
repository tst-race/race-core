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

package com.twosix.race.service;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import ShimsJava.BootstrapActionType;

import com.twosix.race.core.Constants;

/** Class used to send actions to the Node Daemon via intents */
public class NodeDaemonActionPublisher {

    private static final String TAG = "NodeDaemonActionPublisher";
    private final Context context;

    /** @param context app context required to send intents */
    public NodeDaemonActionPublisher(Context context) {
        this.context = context;
    }

    /**
     * @param data information required to perform the bootstrap action
     * @param bootstrapActionType the type of action to perform
     */
    public synchronized void publishBootstrapAction(
            String data, BootstrapActionType bootstrapActionType) {
        try {

            Intent intent = new Intent();
            intent.setAction(Constants.FORWARD_BOOTSTRAP_INFO_ACTION);
            intent.putExtra("message", data);
            intent.putExtra("actionType", bootstrapActionType.toString());

            context.sendBroadcast(intent);
        } catch (Exception e) {
            Log.e(TAG, "Error publishing bootstrap action", e);
        }
    }
}
