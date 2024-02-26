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

package com.twosix.race.ui;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.navigation.NavController;
import androidx.navigation.fragment.NavHostFragment;

import ShimsJava.BootstrapActionType;
import ShimsJava.JClrMsg;
import ShimsJava.PluginStatus;
import ShimsJava.RaceHandle;
import ShimsJava.UserDisplayType;

import com.twosix.race.NavGraphDirections;
import com.twosix.race.R;
import com.twosix.race.service.IRaceService;
import com.twosix.race.service.IRaceServiceProvider;
import com.twosix.race.service.IServiceListener;
import com.twosix.race.service.RaceService;
import com.twosix.race.ui.app.SplashFragmentDirections;
import com.twosix.race.ui.dialogs.InputDialog;
import com.twosix.race.ui.dialogs.MessageDialog;
import com.twosix.race.ui.dialogs.QRCodeDialog;

/** RACE app UI activity */
public class MainActivity extends AppCompatActivity
        implements IRaceServiceProvider, IServiceListener {

    public static final String ACTION_OPEN_CONVERSATION = "openConversation";
    public static final String ACTION_RESPOND_TO_INPUT = "respondToInput";
    public static final String ACTION_DISPLAY_ALERT = "displayAlert";
    public static final String ARG_CONVERSATION_ID = "conversationId";

    public static final String ARG_INPUT_HANDLE = "inputHandle";
    public static final String ARG_INPUT_PROMPT = "inputPrompt";

    public static final String ARG_ALERT_HANDLE = "alertHandle";
    public static final String ARG_ALERT_MESSAGE = "alertMessage";
    public static final String ARG_ALERT_TYPE = "alertType";

    private static final String TAG = "MainActivity";

    private static final String SHARED_PREF_FILE = "race_activity_pref";
    private static final String SHARED_PREF_KEY_FIRST_RUN = "first_run_of_app";

    private NavController navController;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.v(TAG, "onCreate");
        setTheme(R.style.AppTheme);
        setContentView(R.layout.activity_main);

        NavHostFragment navHostFragment =
                (NavHostFragment)
                        getSupportFragmentManager().findFragmentById(R.id.nav_host_fragment);
        navController = navHostFragment.getNavController();

        startRaceService();
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        Log.v(TAG, "onNewIntent");
        // This happens when the app is already running and the user selects a message notification
        if (intent.hasExtra(ARG_CONVERSATION_ID)) {
            navController.navigate(
                    NavGraphDirections.actionGlobalMessageList(
                            intent.getStringExtra(ARG_CONVERSATION_ID)));
        }

        // This happens when the app is in the background and the user selects an input notification
        requestUserInputIfIntent(intent);

        // This happens when the app is in the background and the user selects an alert notification
        displayAlertIfIntent(intent);
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.v(TAG, "onStart");
        bindRaceService();
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.v(TAG, "onStop");
        unbindRaceService();
    }

    private IRaceService raceService;

    private ServiceConnection serviceConnection =
            new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName name, IBinder service) {
                    raceService = ((RaceService.RaceServiceBinder) service).getService();
                    raceService.addServiceListener(MainActivity.this);

                    // Only if we're currently on the splash screen destination, handle initial
                    // navigation
                    if (navController.getCurrentDestination().getId() == R.id.splash_screen) {
                        if (!raceService.startRaceCommunications()) {
                            navController.navigate(SplashFragmentDirections.actionSplashToSetup());
                        } else {
                            // If not launched from history, check if it was launched with a
                            // conversation ID
                            String destConvId = null;
                            if ((getIntent().getFlags()
                                            & Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY)
                                    == 0) {
                                destConvId = getIntent().getStringExtra(ARG_CONVERSATION_ID);
                            }
                            navController.navigate(
                                    SplashFragmentDirections.actionSplashToConversationList()
                                            .setConversationId(destConvId));
                        }

                        // If not launched from history, check if it was launched with input/alert
                        // args
                        if ((getIntent().getFlags() & Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY)
                                == 0) {
                            requestUserInputIfIntent(getIntent());
                            displayAlertIfIntent(getIntent());
                        }
                    }
                }

                @Override
                public void onServiceDisconnected(ComponentName name) {
                    raceService = null;
                }
            };

    private void startRaceService() {
        Intent intent = new Intent(this, RaceService.class);
        startForegroundService(intent);
    }

    private void bindRaceService() {
        Intent intent = new Intent(this, RaceService.class);
        bindService(intent, serviceConnection, Context.BIND_INCLUDE_CAPABILITIES);
    }

    private void unbindRaceService() {
        if (raceService != null) {
            raceService.removeServiceListener(this);
        }
        unbindService(serviceConnection);
    }

    private void requestUserInputIfIntent(Intent intent) {
        if (intent.hasExtra(ARG_INPUT_HANDLE) && intent.hasExtra(ARG_INPUT_PROMPT)) {
            onUserInputRequested(
                    new RaceHandle(intent.getLongExtra(ARG_INPUT_HANDLE, 0)),
                    intent.getStringExtra(ARG_INPUT_PROMPT));
        }
    }

    private void displayAlertIfIntent(Intent intent) {
        if (intent.hasExtra(ARG_ALERT_HANDLE)
                && intent.hasExtra(ARG_ALERT_MESSAGE)
                && intent.hasExtra(ARG_ALERT_TYPE)) {
            onDisplayAlert(
                    new RaceHandle(intent.getLongExtra(ARG_ALERT_HANDLE, 0)),
                    intent.getStringExtra(ARG_ALERT_MESSAGE),
                    UserDisplayType.valueOf(intent.getIntExtra(ARG_ALERT_TYPE, 0)));
        }
    }

    @Override
    public IRaceService getRaceService() {
        return raceService;
    }

    @Override
    public void onStopApp() {
        Log.v(TAG, "onStopApp");
        runOnUiThread(() -> finish());
    }

    @Override
    public void onRaceStatusChange(PluginStatus networkManagerStatus) {
        if (networkManagerStatus.equals(PluginStatus.PLUGIN_READY)) {
            Log.i(TAG, "RACE is fully ready");

            SharedPreferences sharedPreferences =
                    getSharedPreferences(SHARED_PREF_FILE, Context.MODE_PRIVATE);
            if (sharedPreferences.getBoolean(SHARED_PREF_KEY_FIRST_RUN, true)) {
                sharedPreferences.edit().putBoolean(SHARED_PREF_KEY_FIRST_RUN, false).apply();
                onDisplayAlert(
                        new RaceHandle(0),
                        getString(R.string.activity_welcome),
                        UserDisplayType.UD_DIALOG);
            }
        }
    }

    @Override
    public void onMessageAdded(JClrMsg message) {
        // Nothing to do for now (maybe handle read vs unread?)
    }

    @Override
    public void onUserInputRequested(RaceHandle handle, String prompt) {
        runOnUiThread(
                () -> {
                    InputDialog.newInstance(handle, prompt)
                            .show(getSupportFragmentManager(), prompt);
                });
    }

    @Override
    public void onDisplayAlert(RaceHandle handle, String message, UserDisplayType displayType) {
        runOnUiThread(
                () -> {
                    switch (displayType) {
                        case UD_TOAST:
                            Toast.makeText(MainActivity.this, message, Toast.LENGTH_LONG).show();
                            raceService.acknowledgeAlert(handle);
                            break;

                        case UD_NOTIFICATION:
                            // This type will have already been handled by the foreground service
                            break;

                        case UD_DIALOG:
                            MessageDialog.newInstance(handle, message)
                                    .show(getSupportFragmentManager(), message);
                            break;

                        case UD_QR_CODE:
                            QRCodeDialog.newInstance(handle, message)
                                    .show(getSupportFragmentManager(), message);
                            break;

                        default:
                            Log.w(TAG, "Unrecognized alert display type: " + displayType);
                    }
                });
    }

    @Override
    public void onDisplayBootstrapInfo(
            RaceHandle handle,
            String message,
            UserDisplayType displayType,
            BootstrapActionType actionType) {
        // ensure user sees target fail instructions
        if (actionType.equals(BootstrapActionType.BS_FAILED)) {
            String failedInstructions = getString(R.string.bootstrap_failed_instruction);
            onDisplayAlert(handle, failedInstructions, UserDisplayType.UD_DIALOG);
        } else {
            onDisplayAlert(handle, message, displayType);
        }
    }
}
