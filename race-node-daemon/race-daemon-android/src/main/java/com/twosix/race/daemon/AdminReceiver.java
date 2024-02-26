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

import androidx.annotation.NonNull;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Device administration component (no action is taken here, but it is required to be implemented in
 * order for the daemon to be registered as a device/profile admin)
 */
public class AdminReceiver extends android.app.admin.DeviceAdminReceiver {

    private final Logger logger = LoggerFactory.getLogger(getClass());

    @Override
    public void onEnabled(@NonNull Context context, @NonNull Intent intent) {
        logger.info("Device admin enabled");
    }

    @Override
    public void onDisabled(@NonNull Context context, @NonNull Intent intent) {
        logger.info("Device admin disabled");
    }
}
