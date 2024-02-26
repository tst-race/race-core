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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;

public class StopServiceReceiver extends BroadcastReceiver {

    private static final String TAG = "StopServiceReceiver";

    public static final String ACTION = "com.twosix.race.STOP_SERVICE";

    private final IRaceService raceService;

    public StopServiceReceiver(IRaceService raceService) {
        this.raceService = raceService;
    }

    public void register(Context context) {
        IntentFilter filter = new IntentFilter();
        filter.addAction(ACTION);
        context.getApplicationContext().registerReceiver(this, filter);
    }

    public void unregister(Context context) {
        context.getApplicationContext().unregisterReceiver(this);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.v(TAG, "Received stop broadcast");
        raceService.stopService();
    }
}
