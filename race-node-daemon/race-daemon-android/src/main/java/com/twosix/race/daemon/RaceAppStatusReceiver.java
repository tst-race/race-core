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

import com.twosix.race.daemon.sdk.IRaceNodeDaemonSdk;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/** Broadcast intent receiver for status published by the RACE app */
public class RaceAppStatusReceiver extends BroadcastReceiver {

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final IRaceNodeDaemonSdk sdk;

    RaceAppStatusReceiver(IRaceNodeDaemonSdk sdk) {
        this.sdk = sdk;
    }

    /**
     * Publishes the app status update through the daemon SDK.
     *
     * @param context Intent context
     * @param intent Received intent
     */
    @Override
    public void onReceive(Context context, Intent intent) {
        String jsonStatus = intent.getStringExtra("status");
        if (jsonStatus == null || jsonStatus.isEmpty()) {
            logger.error("No status in received intent, unable to forward app status");
            return;
        }

        long ttl = intent.getLongExtra("ttl", 0L);
        if (ttl == 0L) {
            logger.error("No TTL in received intent, unable to forward app status");
            return;
        }

        sdk.updateAppStatus(jsonStatus, ttl);
    }
}
