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
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import com.twosix.race.daemon.util.DeviceProperties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/** Main activity to start the daemon service. */
public class MainActivity extends AppCompatActivity {

    /** General app permissions required in order for the daemon to function */
    private static final String[] REQUIRED_PERMISSIONS =
            new String[] {
                Manifest.permission.READ_EXTERNAL_STORAGE,
                Manifest.permission.WRITE_EXTERNAL_STORAGE
            };
    /**
     * Request code to be used if the user is redirected over to app settings after denying any
     * requested general permissions.
     */
    private static final int APP_SETTINGS_REQUEST = 1;
    /**
     * Request code to be used if the user is redirected over to app settings to grant the overlay
     * permission. Overlay/SYSTEM_ALERT_WINDOW permission is what allows the daemon to launch an
     * activity from the foreground service.
     */
    private static final int OVERLAY_PERMISSION_REQUEST = 2;

    private final Logger logger = LoggerFactory.getLogger(getClass());

    /**
     * Determines the persona for the current node and starts the daemon foreground service.
     *
     * <p>If a persona is included in the launch intent, use that. Otherwise use the persona set via
     * device property. If no persona is specified or set as a device property, then prompt the user
     * for it.
     *
     * @param savedInstanceState Not used
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (hasAllRequiredPermissions()) {
            checkForOverlayPermission();
        } else {
            logger.info("Requesting required permissions from user");
            ActivityCompat.requestPermissions(MainActivity.this, REQUIRED_PERMISSIONS, 1);
        }
    }

    /**
     * Handle results of permissions request.
     *
     * @param requestCode Not used
     * @param permissions Not used
     * @param grantResults Not used
     */
    @Override
    public void onRequestPermissionsResult(
            int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        checkPermissions();
    }

    /**
     * Check if the daemon has all required general app permissions.
     *
     * <p>If the daemon has all required permissions, continue to check for the overlay permission
     * (it is handled differently than the other permissions).
     *
     * <p>If the user has denied permissions to the daemon but has not selected the "do not show
     * again" option (i.e., shouldShowRequestPermissionRationale returns true) re-prompt the user to
     * grant permissions. This requires re-starting the activity.
     *
     * <p>Otherwise the user has selected the "do not show again" option on the permissions request
     * (i.e., shouldShowRequestPermissionRationale returns false), so we have to send the user over
     * to the app settings to manually grant them.
     */
    private void checkPermissions() {
        if (hasAllRequiredPermissions()) {
            checkForOverlayPermission();
        } else if (shouldShowRequestPermissionRationale(REQUIRED_PERMISSIONS[0])) {
            logger.warn("User has denied permissions, re-prompt them");
            AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);
            alertDialogBuilder
                    .setTitle("Second Chance")
                    .setMessage("Click RETRY to grant required permissions for the daemon")
                    .setCancelable(false)
                    .setPositiveButton(
                            "RETRY",
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    Intent intent =
                                            new Intent(MainActivity.this, MainActivity.class);
                                    intent.addFlags(
                                            Intent.FLAG_ACTIVITY_CLEAR_TOP
                                                    | Intent.FLAG_ACTIVITY_NEW_TASK);
                                    startActivity(intent);
                                }
                            })
                    .setNegativeButton(
                            "EXIT",
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    finish();
                                    dialog.cancel();
                                }
                            });
            AlertDialog alertDialog = alertDialogBuilder.create();
            alertDialog.show();
        } else {
            logger.error("User has denied permissions, send them over to settings");
            AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);
            alertDialogBuilder
                    .setTitle("Change Permissions in Settings")
                    .setMessage(
                            "Click SETTINGS to manually grant required permissions to the daemon")
                    .setCancelable(false)
                    .setPositiveButton(
                            "SETTINGS",
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    Intent intent =
                                            new Intent(
                                                    Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                                    Uri uri = Uri.fromParts("package", getPackageName(), null);
                                    intent.setData(uri);
                                    startActivityForResult(intent, APP_SETTINGS_REQUEST);
                                }
                            });
            AlertDialog alertDialog = alertDialogBuilder.create();
            alertDialog.show();
        }
    }

    /**
     * Check if the daemon has all required app permissions.
     *
     * @return True if daemon has all required permissions
     */
    private boolean hasAllRequiredPermissions() {
        for (String permission : REQUIRED_PERMISSIONS) {
            if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    /**
     * Check if the daemon has been granted overlay permissions. Overlay/SYSTEM_ALERT_WINDOW
     * permission allows the daemon to launch activities from the foreground service without any
     * direct user interaction.
     *
     * <p>If the daemon has overlay permissions, continue on to check for the RACE persona.
     *
     * <p>Otherwise prompt the user to grant overlay permissions via app settings.
     */
    private void checkForOverlayPermission() {
        if (!Settings.canDrawOverlays(this)) {
            logger.info("Requesting overlay permissions from the user");
            AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);
            alertDialogBuilder
                    .setTitle("Require Overlay Permissions")
                    .setMessage(
                            "Click SETTINGS to manually grant required overlay permission to the"
                                    + " daemon")
                    .setCancelable(false)
                    .setPositiveButton(
                            "SETTINGS",
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    Intent intent =
                                            new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION);
                                    Uri uri = Uri.fromParts("package", getPackageName(), null);
                                    intent.setData(uri);
                                    startActivityForResult(intent, OVERLAY_PERMISSION_REQUEST);
                                }
                            });
            AlertDialog alertDialog = alertDialogBuilder.create();
            alertDialog.show();
        } else {
            checkForPersona();
        }
    }

    /**
     * If we're returning from the general permissions settings, re-check if we have all required
     * permissions or continue checking for overlay permissions.
     *
     * <p>If we're returning from the overlay permissions settings, re-check if we have the overlay
     * permissions.
     *
     * @param requestCode Activity request code
     * @param resultCode Not used
     * @param data Not used
     */
    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        logger.debug("onActivityResult, " + requestCode + ", " + resultCode);
        if (requestCode == APP_SETTINGS_REQUEST) {
            if (hasAllRequiredPermissions()) {
                checkForOverlayPermission();
            } else {
                checkPermissions();
            }
        } else if (requestCode == OVERLAY_PERMISSION_REQUEST) {
            checkForOverlayPermission();
        }
    }

    /**
     * Determines the persona for the current node and starts the daemon foreground service.
     *
     * <p>If a persona is included in the launch intent, use that. Otherwise use the persona set via
     * device property. If no persona is specified or set as a device property, then prompt the user
     * for it.
     */
    private void checkForPersona() {
        String persona = getIntent().getStringExtra("persona");
        if (persona == null || persona.isEmpty()) {
            persona = DeviceProperties.getProp(DeviceProperties.PERSONA);
        }

        if (persona.isEmpty()) {
            promptUserForPersona();
        } else {
            startDaemonService(persona);
        }
    }

    /**
     * Displays an alert dialog to the user to allow input of the persona.
     *
     * <p>If the user enters a persona, the daemon foreground service is started. If the user does
     * not enter a persona, the activity finishes and the app closes.
     */
    private void promptUserForPersona() {
        AlertDialog.Builder alertBuilder = new AlertDialog.Builder(this);
        alertBuilder.setTitle("Set Persona");

        View view =
                LayoutInflater.from(this)
                        .inflate(
                                R.layout.persona_prompt_dialog,
                                (ViewGroup) getWindow().getDecorView().getRootView(),
                                false);
        final EditText input = (EditText) view.findViewById(R.id.personaInput);
        alertBuilder.setView(view);

        alertBuilder.setPositiveButton(
                android.R.string.ok,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                        startDaemonService(input.getText().toString());
                    }
                });

        alertBuilder.setNegativeButton(
                android.R.string.cancel,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.cancel();
                        finish();
                    }
                });

        alertBuilder.show();
    }

    /**
     * Starts the daemon foreground service for the given persona.
     *
     * @param persona RACE node persona
     */
    private void startDaemonService(String persona) {
        logger.info("Starting daemon service for persona: {}", persona);

        Intent intent = new Intent(this, DaemonService.class);
        intent.putExtra("persona", persona);
        startForegroundService(intent);

        finish();
    }
}
