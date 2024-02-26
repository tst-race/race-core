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

package com.twosix.race.ui.setup;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.PermissionInfo;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Looper;
import android.provider.DocumentsContract;
import android.provider.Settings;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.result.ActivityResult;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.core.graphics.drawable.DrawableCompat;
import androidx.documentfile.provider.DocumentFile;
import androidx.fragment.app.DialogFragment;
import androidx.navigation.NavController;
import androidx.navigation.fragment.NavHostFragment;

import ShimsJava.ChannelStatus;
import ShimsJava.JChannelProperties;
import ShimsJava.PluginStatus;

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;
import com.twosix.race.R;
import com.twosix.race.core.AndroidFileSystemHelpers;
import com.twosix.race.core.Constants;
import com.twosix.race.service.ServiceListener;
import com.twosix.race.ui.RaceFragment;
import com.twosix.race.ui.channels.SelectChannelsFragment;

import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.file.Files;
import java.util.Map;

/* View fragment to perform first-time app setup */
public class SetupFragment extends RaceFragment {

    private static final String TAG = "SetupFragment";
    private static final String SHARED_PREF_FILE = "race_setup_shared_pref";
    private static final String SHARED_PREF_KEY_HAS_REQ_PERM = "has_requested_permission";

    private ServiceListener serviceListener =
            new ServiceListener() {
                @Override
                public void onRaceStatusChange(PluginStatus networkManagerStatus) {
                    if (networkManagerStatus.equals(PluginStatus.PLUGIN_READY)) {
                        getActivity()
                                .runOnUiThread(
                                        () -> {
                                            NavController navController =
                                                    NavHostFragment.findNavController(
                                                            SetupFragment.this);
                                            if (navController.getCurrentDestination().getId()
                                                    == R.id.setup_screen) {
                                                navController.navigate(
                                                        SetupFragmentDirections
                                                                .actionSetupToConversationList());
                                            }
                                        });
                    }
                }
            };

    private TextView permissionsText;
    private ImageView permissionsIcon;

    private TextView passphraseText;
    private ImageView passphraseIcon;

    private TextView assetsText;
    private ImageView assetsIcon;

    private TextView pluginsText;
    private ImageView pluginsIcon;

    private TextView configsText;
    private ImageView configsIcon;

    private TextView personaText;
    private ImageView personaIcon;

    private TextView channelsText;
    private ImageView channelsIcon;

    private ProgressBar progressBar;
    private TextView currentActionText;

    private boolean isHeadless = true;

    private final ActivityResultLauncher<String[]> requestPermissionLauncher =
            registerForActivityResult(
                    new ActivityResultContracts.RequestMultiplePermissions(),
                    this::onPermissionResult);

    private final ActivityResultLauncher<Intent> activityLauncher =
            registerForActivityResult(
                    new ActivityResultContracts.StartActivityForResult(), result -> {});

    private Uri selectedPluginsUri;
    private final ActivityResultLauncher<Intent> pluginsSelectLauncher =
            registerForActivityResult(
                    new ActivityResultContracts.StartActivityForResult(),
                    this::onPluginsSelectResult);

    private Uri selectedConfigsUri;
    private final ActivityResultLauncher<Intent> configsSelectLauncher =
            registerForActivityResult(
                    new ActivityResultContracts.StartActivityForResult(),
                    this::onConfigsSelectResult);

    private String[] selectedChannels;

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);

        raceService.addServiceListener(serviceListener);

        getActivity()
                .getSupportFragmentManager()
                .setFragmentResultListener(
                        SelectChannelsFragment.FRAGMENT_RESULT_KEY,
                        this,
                        this::onChannelSelectResult);
    }

    @Override
    public void onDetach() {
        super.onDetach();
        if (raceService != null) {
            raceService.removeServiceListener(serviceListener);
        }
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        Log.v(TAG, "onCreateView");

        getActivity().setTitle(R.string.setup_title);
        View view = inflater.inflate(R.layout.fragment_setup, container, false);

        permissionsText = view.findViewById(R.id.permissions_text);
        permissionsIcon = view.findViewById(R.id.permissions_icon);

        passphraseText = view.findViewById(R.id.passphrase_text);
        passphraseIcon = view.findViewById(R.id.passphrase_icon);

        assetsText = view.findViewById(R.id.assets_text);
        assetsIcon = view.findViewById(R.id.assets_icon);

        pluginsText = view.findViewById(R.id.plugins_text);
        pluginsIcon = view.findViewById(R.id.plugins_icon);

        configsText = view.findViewById(R.id.configs_text);
        configsIcon = view.findViewById(R.id.configs_icon);

        personaText = view.findViewById(R.id.persona_text);
        personaIcon = view.findViewById(R.id.persona_icon);

        channelsText = view.findViewById(R.id.channels_text);
        channelsIcon = view.findViewById(R.id.channels_icon);

        progressBar = view.findViewById(R.id.progress_bar);
        currentActionText = view.findViewById(R.id.current_action_text);

        isHeadless = checkIfIsHeadless();

        return view;
    }

    @Override
    public void onResume() {
        super.onResume();
        Log.v(TAG, "onResume");
        performSetupSteps();
    }

    private void performSetupSteps() {
        Log.d(TAG, "Running setup steps");

        if (raceService.arePermissionsSufficient()) {
            markComplete(permissionsText, permissionsIcon);
        } else {
            markInProgress(permissionsText, permissionsIcon);
            requestPermissions();
            return;
        }

        if (raceService.hasPassphrase()) {
            Log.d(TAG, "already has a passphrase");
            markComplete(passphraseText, passphraseIcon);
        } else {
            markInProgress(passphraseText, passphraseIcon);
            getPassphrase();
            return;
        }

        if (raceService.areAssetsExtracted()) {
            markComplete(assetsText, assetsIcon);
        } else {
            markInProgress(assetsText, assetsIcon);
            extractAssets();
            return;
        }

        if (raceService.hasPlugins()) {
            markComplete(pluginsText, pluginsIcon);
        } else {
            markInProgress(pluginsText, pluginsIcon);
            copyPlugins();
            return;
        }

        if (raceService.hasConfigs()) {
            markComplete(configsText, configsIcon);
        } else {
            markInProgress(configsText, configsIcon);
            copyConfigs();
            return;
        }

        if (raceService.hasPersona()) {
            markComplete(personaText, personaIcon);
        } else {
            markInProgress(personaText, personaIcon);
            getPersona();
            return;
        }

        // Have to try this before checking for enabled channels because we need the RACE SDK
        // to have been created in order to query it about channels.
        //
        // After channels get set, we'll come through here again and re-try (and ideally succeed).
        if (raceService.startRaceCommunications()) {
            markComplete(channelsText, channelsIcon);
            setCurrentActionText(R.string.setup_initializing);
            return;
        }

        if (raceService.hasEnabledChannels()) {
            markComplete(channelsText, channelsIcon);
        } else {
            markInProgress(channelsText, channelsIcon);
            setEnabledChannels();
        }
    }

    private static boolean checkIfIsHeadless() {
        try {
            File testAppConfigFile = new File(Constants.PATH_TO_TESTAPP_CONFIG);
            if (!testAppConfigFile.exists()) {
                Log.d(TAG, "No test app config file, defaulting to non-headless");
                return false;
            }

            String contents = new String(Files.readAllBytes(testAppConfigFile.toPath())).trim();

            JSONParser parser = new JSONParser();
            JSONObject testAppConfig = (JSONObject) parser.parse(contents);

            if (testAppConfig.containsKey("headless")) {
                boolean headless = (boolean) testAppConfig.get("headless");
                Log.d(TAG, "Will run headless? " + headless);
                return headless;
            }
        } catch (Exception err) {
            Log.w(TAG, "Error reading test app config file: " + err.toString());
        }

        Log.d(TAG, "Defaulting to headless");
        return true;
    }

    private static String getPassphraseFromUserResponses() {
        try {
            File userResponsesFile = new File(Constants.PATH_TO_USER_RESPONSES);
            if (!userResponsesFile.exists()) {
                throw new FileNotFoundException();
            }

            String contents = new String(Files.readAllBytes(userResponsesFile.toPath())).trim();

            JSONParser parser = new JSONParser();
            JSONObject userResponsesJson = (JSONObject) parser.parse(contents);
            JSONObject userResponsesSdkObject = (JSONObject) userResponsesJson.get("sdk");

            if (userResponsesSdkObject.containsKey("passphrase")) {
                String passphrase = (String) userResponsesSdkObject.get("passphrase");
                Log.d(TAG, "Passphrase found in user responses. Using that as default.");
                return passphrase;
            }
        } catch (Exception err) {
            Log.w(
                    TAG,
                    "Error reading user responses file "
                            + Constants.PATH_TO_USER_RESPONSES
                            + " : "
                            + err.toString());
        }

        Log.d(TAG, "No user responses file found. Requiring user to manually set passphrase.");
        return null;
    }

    /**********************************
     * Visual indicators
     *********************************/

    private void markInProgress(TextView textView, ImageView imageView) {
        Resources.Theme theme = getActivity().getTheme();
        int color = getResources().getColor(R.color.primaryText, theme);
        textView.setTextColor(color);

        Drawable pendingImage = getResources().getDrawable(R.drawable.ic_pending, theme);
        Drawable blackPendingImage = DrawableCompat.wrap(pendingImage);
        DrawableCompat.setTint(blackPendingImage, color);

        imageView.setImageDrawable(blackPendingImage);
    }

    private void setCurrentActionText(@StringRes int text) {
        if (Looper.getMainLooper().isCurrentThread()) {
            progressBar.setIndeterminate(true);
            currentActionText.setText(text);
        } else {
            getActivity().runOnUiThread(() -> setCurrentActionText(text));
        }
    }

    private void markComplete(TextView textView, ImageView imageView) {
        Resources.Theme theme = getActivity().getTheme();
        int color = getResources().getColor(R.color.primaryText, theme);
        textView.setTextColor(color);

        Drawable doneImage = getResources().getDrawable(R.drawable.ic_done, theme);
        Drawable greenDoneImage = DrawableCompat.wrap(doneImage);
        DrawableCompat.setTint(greenDoneImage, Color.GREEN);

        imageView.setImageDrawable(greenDoneImage);
        imageView.clearAnimation();

        progressBar.setIndeterminate(false);
        currentActionText.setText("");
    }

    private void markFailed(TextView textView, ImageView imageView, @StringRes Integer toast) {
        if (Looper.getMainLooper().isCurrentThread()) {
            Resources.Theme theme = getActivity().getTheme();
            int color = getResources().getColor(R.color.primaryText, theme);
            textView.setTextColor(color);

            Drawable errorImage = getResources().getDrawable(R.drawable.ic_error, theme);
            Drawable redErrorImage = DrawableCompat.wrap(errorImage);
            DrawableCompat.setTint(redErrorImage, Color.RED);

            imageView.setImageDrawable(redErrorImage);

            progressBar.setIndeterminate(false);
            currentActionText.setText("");

            if (toast != null) {
                Toast.makeText(getActivity(), toast, Toast.LENGTH_LONG).show();
            }
        } else {
            getActivity().runOnUiThread(() -> markFailed(textView, imageView, toast));
        }
    }

    /**********************************
     * Permissions
     *********************************/

    private void requestPermissions() {
        Log.d(TAG, "Requesting required permissions");

        SharedPreferences sharedPreferences =
                getActivity().getSharedPreferences(SHARED_PREF_FILE, Context.MODE_PRIVATE);

        // If we've already asked for permissions before and the user temporarily denied it,
        // shouldShowRequestPermissionRationale will return true. It will be false for both
        // never-asked and permanently-denied cases.
        for (String permission : raceService.getRequiredPermissions()) {
            if (shouldShowRequestPermissionRationale(permission)) {
                Log.d(TAG, "Showing permission rationale for " + permission);
                showPermissionRationale(permission);
                return;
            }
        }

        if (sharedPreferences.getBoolean(SHARED_PREF_KEY_HAS_REQ_PERM, false)) {
            // We've already asked for permissions before, and something was permanently denied
            // (otherwise we'd have hit a shouldShowRequestPermissionRationale case above).
            for (String permission : raceService.getRequiredPermissions()) {
                if (getActivity().checkSelfPermission(permission)
                        != PackageManager.PERMISSION_GRANTED) {
                    Log.d(
                            TAG,
                            "Permission for "
                                    + permission
                                    + " has already been requested and permanently denied by the"
                                    + " user");
                    showFinalPermissionRationale(permission);
                    return;
                }
            }
        }

        Log.d(TAG, "Launching permissions request");
        sharedPreferences.edit().putBoolean(SHARED_PREF_KEY_HAS_REQ_PERM, true).apply();
        requestPermissionLauncher.launch(raceService.getRequiredPermissions());
    }

    private void showPermissionRationale(String permission) {
        String permissionLabel = permission;
        try {
            PackageManager packageManager = getActivity().getPackageManager();
            PermissionInfo permissionInfo =
                    packageManager.getPermissionInfo(permission, PackageManager.GET_META_DATA);
            permissionLabel = getString(permissionInfo.labelRes);
        } catch (Exception err) {
        }

        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(getActivity());
        alertDialogBuilder
                .setTitle(R.string.setup_permissions_rationale_title)
                .setMessage(getString(R.string.setup_permissions_rationale_retry, permissionLabel))
                .setCancelable(false)
                .setPositiveButton(
                        R.string.setup_alert_retry,
                        (dialog, which) -> {
                            requestPermissionLauncher.launch(raceService.getRequiredPermissions());
                        })
                .setNegativeButton(
                        R.string.setup_alert_cancel,
                        (dialog, which) -> {
                            getActivity().finish();
                            dialog.cancel();
                        });
        AlertDialog alertDialog = alertDialogBuilder.create();
        alertDialog.show();
    }

    private void showFinalPermissionRationale(String permission) {
        String permissionLabel = permission;
        try {
            PackageManager packageManager = getActivity().getPackageManager();
            PermissionInfo permissionInfo =
                    packageManager.getPermissionInfo(permission, PackageManager.GET_META_DATA);
            permissionLabel = getString(permissionInfo.labelRes);
        } catch (Exception err) {
        }

        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(getActivity());
        alertDialogBuilder
                .setTitle(R.string.setup_permissions_rationale_title)
                .setMessage(
                        getString(R.string.setup_permissions_rationale_settings, permissionLabel))
                .setCancelable(false)
                .setPositiveButton(
                        R.string.setup_alert_settings,
                        (dialog, which) -> {
                            launchSettingsActivity();
                            dialog.cancel();
                        })
                .setNegativeButton(
                        R.string.setup_alert_cancel,
                        (dialog, which) -> {
                            getActivity().finish();
                            dialog.cancel();
                        });
        AlertDialog alertDialog = alertDialogBuilder.create();
        alertDialog.show();
    }

    private void onPermissionResult(Map<String, Boolean> permissions) {
        if (permissions.isEmpty()) {
            Log.d(TAG, "Permissions result map is empty");
        } else if (permissions.containsValue(false)) {
            Log.d(TAG, "At least one permission has been denied");
        } else {
            Log.d(TAG, "All permissions have been granted");
        }
    }

    private void launchSettingsActivity() {
        Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
        Uri uri = Uri.fromParts("package", getActivity().getPackageName(), null);
        intent.setData(uri);
        activityLauncher.launch(intent);
    }

    /**********************************
     * Assets
     *********************************/

    private void extractAssets() {
        new Thread(
                        () -> {
                            try {
                                File appDataDir = getActivity().getDataDir();
                                File pythonDir = new File(appDataDir, "python3.7");
                                pythonDir.mkdir();

                                setCurrentActionText(R.string.setup_asset_python_bindings);
                                // Copy Python bindings from assets
                                AndroidFileSystemHelpers.copyAssetDir(
                                        "race/python", getActivity().getApplicationContext());

                                setCurrentActionText(R.string.setup_asset_python_packages);

                                // .tar.gz files get the .gz extension stripped on install. They are
                                // still compressed though
                                if (System.getProperty("os.arch").equals("aarch64")) {
                                    AndroidFileSystemHelpers.extractTar(
                                            "python-packages-android-arm64-v8a.tar",
                                            pythonDir.getAbsolutePath() + "/",
                                            getActivity().getApplicationContext());
                                } else {
                                    AndroidFileSystemHelpers.extractTar(
                                            "python-packages-android-x86_64.tar",
                                            pythonDir.getAbsolutePath() + "/",
                                            getActivity().getApplicationContext());
                                }

                                getActivity()
                                        .runOnUiThread(
                                                () -> {
                                                    markComplete(assetsText, assetsIcon);
                                                    performSetupSteps();
                                                });
                            } catch (IOException err) {
                                Log.e(TAG, "Error extracting assets", err);
                                markFailed(
                                        assetsText,
                                        assetsIcon,
                                        R.string.setup_asset_extraction_failed);
                            }
                        })
                .start();
    }

    /**********************************
     * Plugins
     *********************************/

    private void copyPlugins() {
        File downloadArtifactsDir = new File(Constants.PATH_TO_DOWNLOADS + "/race/artifacts");
        if (downloadArtifactsDir.exists()) {
            copyPlugins(downloadArtifactsDir);
            // TODO remove artifacts dir out of download after copying
        } else if (isHeadless) {
            Log.e(TAG, "No plugins found and running non-interactively. Aborting.");
            getActivity().finish();
        } else if (selectedPluginsUri != null) {
            copyPlugins(selectedPluginsUri);
        } else {
            // Ask the user for the location of the plugins
            AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(getActivity());
            alertDialogBuilder
                    .setTitle(R.string.setup_plugins_prompt_title)
                    .setMessage(R.string.setup_plugins_prompt)
                    .setCancelable(false)
                    .setPositiveButton(
                            R.string.setup_alert_select,
                            (dialog, which) -> {
                                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
                                intent.putExtra(
                                        DocumentsContract.EXTRA_INITIAL_URI,
                                        Uri.fromFile(new File(Constants.PATH_TO_DOWNLOADS)));

                                pluginsSelectLauncher.launch(intent);
                            })
                    .setNegativeButton(
                            R.string.setup_alert_cancel,
                            (dialog, which) -> {
                                dialog.cancel();
                                getActivity().finish();
                            });
            AlertDialog alertDialog = alertDialogBuilder.create();
            alertDialog.show();
        }
    }

    private void onPluginsSelectResult(ActivityResult result) {
        Log.v(TAG, "onPluginsSelectResult");
        if (result.getResultCode() == Activity.RESULT_OK) {
            if (result.getData() != null) {
                selectedPluginsUri = result.getData().getData();
            }
        }
    }

    private void copyPlugins(Uri uri) {
        new Thread(
                        () -> {
                            try {
                                DocumentFile docFile = DocumentFile.fromTreeUri(getActivity(), uri);
                                if (docFile != null) {
                                    setCurrentActionText(R.string.setup_plugins_copy);
                                    File appDataDir = getActivity().getDataDir();
                                    File artifactsDir = new File(appDataDir, "race/artifacts");
                                    Log.d(
                                            TAG,
                                            "Copying plugin artifacts from "
                                                    + uri
                                                    + " to "
                                                    + artifactsDir);
                                    AndroidFileSystemHelpers.copyDir(
                                            getActivity().getContentResolver(),
                                            docFile,
                                            artifactsDir);

                                    getActivity()
                                            .runOnUiThread(
                                                    () -> {
                                                        markComplete(pluginsText, pluginsIcon);
                                                        performSetupSteps();
                                                    });
                                } else {
                                    markFailed(
                                            pluginsText,
                                            pluginsIcon,
                                            R.string.setup_plugins_failed);
                                }
                            } catch (IOException err) {
                                Log.e(TAG, "Error copying plugin artifacts", err);
                                markFailed(pluginsText, pluginsIcon, R.string.setup_plugins_failed);
                            }
                        })
                .start();
    }

    private void copyPlugins(File sourcePluginsDir) {
        new Thread(
                        () -> {
                            try {
                                setCurrentActionText(R.string.setup_plugins_copy);
                                File appDataDir = getActivity().getDataDir();
                                File artifactsDir = new File(appDataDir, "race/artifacts");
                                Log.d(
                                        TAG,
                                        "Copying plugin artifacts from "
                                                + sourcePluginsDir
                                                + " to "
                                                + artifactsDir);
                                AndroidFileSystemHelpers.copyDir(
                                        sourcePluginsDir.getAbsolutePath(),
                                        artifactsDir.getAbsolutePath());

                                getActivity()
                                        .runOnUiThread(
                                                () -> {
                                                    markComplete(pluginsText, pluginsIcon);
                                                    performSetupSteps();
                                                });
                            } catch (IOException err) {
                                Log.e(TAG, "Error copying plugin artifacts", err);
                                markFailed(pluginsText, pluginsIcon, R.string.setup_plugins_failed);
                            }
                        })
                .start();
    }

    /**********************************
     * Configs
     *********************************/

    private void copyConfigs() {
        File downloadConfigsTar = new File(Constants.PATH_TO_DOWNLOADS, "race/configs.tar.gz");
        if (downloadConfigsTar.exists()) {
            copyConfigs(downloadConfigsTar);
            // TODO remove configs tar out of download after copying
        } else if (isHeadless) {
            Log.e(TAG, "No configs found and running non-interactively. Aborting.");
            getActivity().finish();
        } else if (selectedConfigsUri != null) {
            copyConfigs(selectedConfigsUri);
        } else {
            // Ask the user for the location of the configs
            AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(getActivity());
            alertDialogBuilder
                    .setTitle(R.string.setup_configs_prompt_title)
                    .setMessage(R.string.setup_configs_prompt)
                    .setCancelable(false)
                    .setPositiveButton(
                            R.string.setup_alert_select,
                            (dialog, which) -> {
                                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                                intent.setType("application/gzip");
                                intent.putExtra(
                                        DocumentsContract.EXTRA_INITIAL_URI,
                                        Uri.fromFile(new File(Constants.PATH_TO_DOWNLOADS)));

                                configsSelectLauncher.launch(intent);
                            })
                    .setNegativeButton(
                            R.string.setup_alert_cancel,
                            (dialog, which) -> {
                                dialog.cancel();
                                getActivity().finish();
                            });
            AlertDialog alertDialog = alertDialogBuilder.create();
            alertDialog.show();
        }
    }

    private void onConfigsSelectResult(ActivityResult result) {
        Log.v(TAG, "onConfigsSelectResult");
        if (result.getResultCode() == Activity.RESULT_OK) {
            if (result.getData() != null) {
                selectedConfigsUri = result.getData().getData();
            }
        }
    }

    private void copyConfigs(Uri uri) {
        new Thread(
                        () -> {
                            try {
                                DocumentFile docFile =
                                        DocumentFile.fromSingleUri(getActivity(), uri);
                                if (docFile != null) {
                                    setCurrentActionText(R.string.setup_configs_copy);
                                    File raceDir = getActivity().getExternalFilesDir("race");
                                    File configsTar = new File(raceDir, "configs.tar.gz");
                                    Log.d(TAG, "Copying configs from " + uri + " to " + configsTar);
                                    AndroidFileSystemHelpers.copyDir(
                                            getActivity().getContentResolver(),
                                            docFile,
                                            configsTar);

                                    getActivity()
                                            .runOnUiThread(
                                                    () -> {
                                                        markComplete(configsText, configsIcon);
                                                        performSetupSteps();
                                                    });
                                } else {
                                    markFailed(
                                            configsText,
                                            configsIcon,
                                            R.string.setup_configs_failed);
                                }
                            } catch (IOException err) {
                                Log.e(TAG, "Error copying configs", err);
                                markFailed(configsText, configsIcon, R.string.setup_configs_failed);
                            }
                        })
                .start();
    }

    private void copyConfigs(File sourceConfigsTar) {
        new Thread(
                        () -> {
                            try {
                                setCurrentActionText(R.string.setup_configs_copy);
                                File raceDir = getActivity().getExternalFilesDir("race");
                                File configsTar = new File(raceDir, "configs.tar.gz");
                                Log.d(
                                        TAG,
                                        "Copying configs from "
                                                + sourceConfigsTar
                                                + " to "
                                                + configsTar);
                                AndroidFileSystemHelpers.copyDir(
                                        sourceConfigsTar.getAbsolutePath(),
                                        configsTar.getAbsolutePath());

                                getActivity()
                                        .runOnUiThread(
                                                () -> {
                                                    markComplete(configsText, configsIcon);
                                                    performSetupSteps();
                                                });
                            } catch (IOException err) {
                                Log.e(TAG, "Error copying configs", err);
                                markFailed(configsText, configsIcon, R.string.setup_configs_failed);
                            }
                        })
                .start();
    }

    /**********************************
     * Persona
     *********************************/

    private void getPersona() {
        if (isHeadless) {
            getPersonaFromDebugProp();
        } else {
            promptUserForPersona();
        }
    }

    private void getPersonaFromDebugProp() {
        new Thread(
                        () -> {
                            try {
                                Process process =
                                        Runtime.getRuntime()
                                                .exec("/system/bin/getprop debug.RACE_PERSONA");
                                InputStream stdin = process.getInputStream();
                                InputStreamReader isr = new InputStreamReader(stdin);
                                BufferedReader br = new BufferedReader(isr);
                                String persona = br.readLine();

                                raceService.setPersona(persona);
                                getActivity()
                                        .runOnUiThread(
                                                () -> {
                                                    markComplete(personaText, personaIcon);
                                                    performSetupSteps();
                                                });
                            } catch (IOException err) {
                                Log.e(TAG, "Error getting persona debug prop", err);
                                markFailed(personaText, personaIcon, R.string.setup_persona_failed);
                            }
                        })
                .start();
    }

    private void promptUserForPersona() {
        View view = LayoutInflater.from(getActivity()).inflate(R.layout.fragment_text_input, null);
        TextInputLayout textInputLayout = view.findViewById(R.id.text_input_layout);
        textInputLayout.setHint(R.string.setup_persona_hint);

        final TextInputEditText textInput = view.findViewById(R.id.text_input);

        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(getActivity());
        alertDialogBuilder
                .setTitle(R.string.setup_persona_prompt_title)
                .setCancelable(false)
                .setView(view)
                .setPositiveButton(
                        android.R.string.ok,
                        (dialog, which) -> {
                            String persona = textInput.getText().toString();
                            if (!persona.isEmpty()) {
                                raceService.setPersona(persona);
                            }
                            dialog.dismiss();
                            performSetupSteps();
                        })
                .setNegativeButton(
                        R.string.setup_alert_cancel,
                        (dialog, which) -> {
                            dialog.cancel();
                            getActivity().finish();
                        });
        alertDialogBuilder.show();
    }

    private void getPassphrase() {
        String passphrase = this.getPassphraseFromUserResponses();
        if (passphrase != null) {
            raceService.setPassphrase(passphrase);
            performSetupSteps();
        } else {
            promptUserForPassphrase();
        }
    }

    // TODO: this is very similar to `promptUserForPersona()`. Refactor to factor out common logic
    // for reusability.
    private void promptUserForPassphrase() {
        View view =
                LayoutInflater.from(getActivity()).inflate(R.layout.fragment_password_input, null);
        TextInputLayout textInputLayout = view.findViewById(R.id.password_input_layout);
        textInputLayout.setHint(R.string.setup_passphrase_hint);

        final TextInputEditText textInput = view.findViewById(R.id.password_input);

        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(getActivity());
        int titleId = 0;
        if (!raceService.isPassphraseSet()) {
            titleId = R.string.setup_new_passphrase_prompt_title;
        } else if (raceService.receivedInvalidPassphrase()) {
            titleId = R.string.setup_retry_passphrase_prompt_title;
        } else {
            titleId = R.string.setup_passphrase_prompt_title;
        }
        alertDialogBuilder
                .setTitle(titleId)
                .setCancelable(false)
                .setView(view)
                .setPositiveButton(
                        android.R.string.ok,
                        (dialog, which) -> {
                            String passphrase = textInput.getText().toString();
                            if (!passphrase.isEmpty()) {
                                raceService.setPassphrase(passphrase);
                            }
                            dialog.dismiss();
                            performSetupSteps();
                        })
                .setNegativeButton(
                        R.string.setup_alert_cancel,
                        (dialog, which) -> {
                            dialog.cancel();
                            getActivity().finish();
                        });
        alertDialogBuilder.show();
    }

    /**********************************
     * Enabled Channels
     *********************************/

    private void setEnabledChannels() {
        if (isHeadless) {
            raceService.useInitialEnabledChannels();
            markComplete(channelsText, channelsIcon);
            performSetupSteps();
        } else if (selectedChannels != null && selectedChannels.length > 0) {
            raceService.useEnabledChannels(selectedChannels);
            markComplete(channelsText, channelsIcon);
            performSetupSteps();
        } else {
            requestChannelSelection();
        }
    }

    private void requestChannelSelection() {
        // Use the disabled channels to filter out unsupported channels (if we're here, no channels
        // are enabled)
        String[] channelIds =
                raceService.getChannels(ChannelStatus.CHANNEL_DISABLED).stream()
                        .map(JChannelProperties::getChannelGid)
                        .toArray(String[]::new);
        DialogFragment dialogFragment = SelectChannelsFragment.newInstance(channelIds);
        dialogFragment.setCancelable(false);
        dialogFragment.show(getActivity().getSupportFragmentManager(), SelectChannelsFragment.TAG);
    }

    private void onChannelSelectResult(String key, Bundle result) {
        Log.v(TAG, "onChannelSelectResult");
        selectedChannels = result.getStringArray(SelectChannelsFragment.RESULT_CHANNELS_KEY);
        performSetupSteps();
    }
}
