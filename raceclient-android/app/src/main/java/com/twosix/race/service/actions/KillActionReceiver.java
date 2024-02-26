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
import android.util.Log;

public class KillActionReceiver extends BroadcastReceiver {

    private static final String TAG = "KillActionReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        int pid = android.os.Process.myPid();
        Log.i(TAG, "Killing PID " + pid);
        // This works for the app because we don't specify for any other Android app components to
        // run in a separate process, but may not necessarily kill any further child processes that
        // may have been started by any plugins
        android.os.Process.killProcess(pid);
    }
}
