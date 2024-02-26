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

import android.Manifest;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInstaller;
import android.os.Bundle;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/** Broadcast intent receiver for installation status updates from the PackageInstaller */
public class InstallStatusReceiver extends BroadcastReceiver {

    private final Logger logger = LoggerFactory.getLogger(getClass());

    /**
     * If the app was successfully installed, attempt to grant external storage permissions to the
     * app (will only succeed if the daemon is a device admin) then start it.
     *
     * <p>If the app was not installed (because the daemon is not a device admin) start the pending
     * user action intent to ask the user to continue the install.
     *
     * @param context Intent context
     * @param intent Received intent
     */
    @Override
    public void onReceive(Context context, Intent intent) {
        try {
            if (Constants.APP_INSTALLED_ACTION.equals(intent.getAction())) {
                Bundle extras = intent.getExtras();

                int status = extras.getInt(PackageInstaller.EXTRA_STATUS);
                String message = extras.getString(PackageInstaller.EXTRA_STATUS_MESSAGE);
                logger.debug("Install status: " + status + ", " + message);

                switch (status) {
                    case PackageInstaller.STATUS_PENDING_USER_ACTION:
                        // Daemon is not the device owner, needs the user to confirm the install
                        logger.info("Requesting user to confirm RACE app install...");
                        Intent confirmIntent = (Intent) extras.get(Intent.EXTRA_INTENT);
                        confirmIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                        context.startActivity(confirmIntent);
                        break;

                    case PackageInstaller.STATUS_SUCCESS:
                        // App is installed, grant permissions
                        logger.info("Granting permissions to RACE app...");
                        try {
                            DevicePolicyManager devicePolicyManager =
                                    (DevicePolicyManager)
                                            context.getSystemService(Context.DEVICE_POLICY_SERVICE);
                            if (!devicePolicyManager.setPermissionGrantState(
                                    new ComponentName(context, AdminReceiver.class),
                                    Constants.RACE_APP_PACKAGE,
                                    Manifest.permission.WRITE_EXTERNAL_STORAGE,
                                    DevicePolicyManager.PERMISSION_GRANT_STATE_GRANTED)) {
                                logger.warn(
                                        "Unable to grant external storage permissions to RACE app");
                            }
                            if (!devicePolicyManager.setPermissionGrantState(
                                    new ComponentName(context, AdminReceiver.class),
                                    Constants.RACE_APP_PACKAGE,
                                    Manifest.permission.ACCESS_FINE_LOCATION,
                                    DevicePolicyManager.PERMISSION_GRANT_STATE_GRANTED)) {
                                logger.warn("Unable to grant location permissions to RACE app");
                            }
                        } catch (SecurityException err) {
                            logger.warn("Unable to grant permissions to RACE app", err);
                        }

                        Intent launchIntent =
                                context.getPackageManager()
                                        .getLaunchIntentForPackage(Constants.RACE_APP_PACKAGE);
                        context.startActivity(launchIntent);
                        break;

                    default:
                        logger.error("Install failed: " + status + ", " + message);
                }
            }
        } catch (Exception err) {
            logger.error("Error handling install status: ", err);
        }
    }
}
