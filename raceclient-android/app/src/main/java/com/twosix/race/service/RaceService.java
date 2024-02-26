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

import android.Manifest;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.drawable.Icon;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.drawable.IconCompat;

import ShimsJava.BootstrapActionType;
import ShimsJava.ChannelStatus;
import ShimsJava.JChannelProperties;
import ShimsJava.JClrMsg;
import ShimsJava.PluginStatus;
import ShimsJava.RaceHandle;
import ShimsJava.RaceSdkApp;
import ShimsJava.StorageEncryptionInvalidPassphraseException;
import ShimsJava.UserDisplayType;

import com.twosix.race.R;
import com.twosix.race.room.Conversation;
import com.twosix.race.room.ConversationDatabase;
import com.twosix.race.room.Message;
import com.twosix.race.service.actions.AppActionReceiver;
import com.twosix.race.ui.MainActivity;

import java.io.File;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.TimeZone;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

/** Foreground service to run RACE SDK */
public class RaceService extends Service implements IRaceService, IServiceListener {

    private static final String TAG = "RaceService";
    private static final String SERVICE_CHANNEL_ID = "com.twosix.race.SERVICE_CHANNEL";
    private static final String MESSAGE_CHANNEL_ID = "com.twosix.race.MESSAGE_CHANNEL";
    private static final int MESSAGE_NOTIF_ID = 1;
    private static final String ALERT_CHANNEL_ID = "com.twosix.race.ALERT_CHANNEL";
    private static final int ALERT_INPUT_NOTIF_ID = 2;
    private static final int ALERT_DISPLAY_NOTIF_ID = 3;
    private static final int BOOTSTRAP_NOTIF_ID = 4;
    private static final String BOOTSTRAP_NOTIF_TAG = "bootstrap";
    private static final String SHARED_PREF_FILE = "race_service_shared_pref";
    private static final String SHARED_PREF_KEY_PERSONA = "persona";
    private static final String SHARED_PREF_KEY_PASSPHRASE_SET = "passphrase_set";
    public static final String ACTION_CANCEL_BOOTSTRAP = "com.twosix.race.ACTION_CANCEL_BOOTSTRAP";
    private static final String[] REQUIRED_PERMISSIONS = {
        Manifest.permission.READ_EXTERNAL_STORAGE,
        Manifest.permission.WRITE_EXTERNAL_STORAGE,
        // Peraton Permissions
        Manifest.permission.ACCESS_WIFI_STATE,
        Manifest.permission.ACCESS_FINE_LOCATION,
        Manifest.permission.ACCESS_COARSE_LOCATION,
        Manifest.permission.ACCESS_NETWORK_STATE,
        Manifest.permission.CHANGE_NETWORK_STATE,
        Manifest.permission.CHANGE_WIFI_STATE
    };

    private final IBinder binder = new RaceServiceBinder();

    private final CopyOnWriteArrayList<IServiceListener> listeners = new CopyOnWriteArrayList<>();

    private final Executor executor = Executors.newSingleThreadExecutor();
    private ConversationDatabase db;

    private RaceSdkApp raceSdk;

    private AndroidRaceApp raceApp;

    private RaceAppStatusPublisher statusPublisher;

    private AppActionReceiver actionReceiver;

    private final StopServiceReceiver stopReceiver = new StopServiceReceiver(this);

    private NotificationManagerCompat notificationManager;
    private final AtomicReference<String> suspendMessageNotificationsPersona =
            new AtomicReference<>();

    private final AtomicInteger nextRequestCode = new AtomicInteger(0);

    private RaceHandle currentBootstrapHandle = new RaceHandle(0L);

    private String passphrase;
    private boolean didReceiveInvalidPassphrase = false;

    public class RaceServiceBinder extends Binder {
        public IRaceService getService() {
            return RaceService.this;
        }
    }

    private final BroadcastReceiver cancelBootstrapListener =
            new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    if (intent.getAction().equals(ACTION_CANCEL_BOOTSTRAP)) {
                        try {
                            raceSdk.cancelBootstrap(currentBootstrapHandle);
                            currentBootstrapHandle = new RaceHandle(0L);
                        } catch (Exception ex) {
                            Log.v(TAG, ex.toString());
                        }
                    } else {
                        Log.v(TAG, "unanticipated action " + intent.getAction());
                    }
                }
            };

    /**********************************
     * Service lifecycle methods
     *********************************/

    @Override
    public IBinder onBind(Intent intent) {
        Log.v(TAG, "Binding RACE service");
        return binder;
    }

    /** Creates all notification channels required by the RACE service */
    @Override
    public void onCreate() {
        Log.d(TAG, "Creating RACE service");
        super.onCreate();
        notificationManager = NotificationManagerCompat.from(this);

        createServiceNotificationChannel();
        createMessageNotificationChannel();
        createAlertNotificationChannel();

        db = ConversationDatabase.getInstance(this);

        statusPublisher = new RaceAppStatusPublisher(getApplicationContext(), getPersona());
        actionReceiver = AppActionReceiver.initReceiver(getApplicationContext());

        stopReceiver.register(getApplicationContext());

        // register to recv bootstrap cancelled intent
        IntentFilter filter = new IntentFilter();
        filter.addAction(ACTION_CANCEL_BOOTSTRAP);
        // TODO: use Context.RECEIVER_NOT_EXPORTED once using API 33 or newer
        getApplicationContext().registerReceiver(cancelBootstrapListener, filter);
    }

    /** Creates the service foreground notification channel */
    private void createServiceNotificationChannel() {
        NotificationChannel channel =
                new NotificationChannel(
                        SERVICE_CHANNEL_ID, "RACE Service", NotificationManager.IMPORTANCE_MIN);
        notificationManager.createNotificationChannel(channel);
    }

    /** Creates the message receipt notification channel */
    private void createMessageNotificationChannel() {
        NotificationChannel channel =
                new NotificationChannel(
                        MESSAGE_CHANNEL_ID, "RACE Messages", NotificationManager.IMPORTANCE_HIGH);
        notificationManager.createNotificationChannel(channel);
    }

    /** Creates the alert (plugin input, display messages, etc.) notification channel */
    private void createAlertNotificationChannel() {
        NotificationChannel channel =
                new NotificationChannel(
                        ALERT_CHANNEL_ID, "RACE Alerts", NotificationManager.IMPORTANCE_HIGH);
        notificationManager.createNotificationChannel(channel);
    }

    /**
     * Starts the RACE foreground service.
     *
     * @param intent Not used
     * @param flags Not used
     * @param startId Not used
     * @return START_STICKY so that if the service dies it is restarted
     */
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "Starting RACE service");

        Intent launchIntent = new Intent(this, MainActivity.class);
        PendingIntent pendingLaunchIntent =
                PendingIntent.getActivity(this, 0, launchIntent, PendingIntent.FLAG_IMMUTABLE);

        Intent stopIntent = new Intent();
        stopIntent.setAction(StopServiceReceiver.ACTION);
        PendingIntent pendingStopIntent =
                PendingIntent.getBroadcast(this, 0, stopIntent, PendingIntent.FLAG_IMMUTABLE);

        NotificationCompat.Action stopAction =
                new NotificationCompat.Action.Builder(
                                IconCompat.createFromIcon(
                                        getApplicationContext(),
                                        Icon.createWithResource(this, R.drawable.ic_close)),
                                getString(R.string.service_stop),
                                pendingStopIntent)
                        .build();

        Notification notification =
                new NotificationCompat.Builder(this, SERVICE_CHANNEL_ID)
                        .setContentTitle(getString(R.string.service_race_is_running))
                        .setSmallIcon(R.mipmap.ic_launcher)
                        .setOngoing(
                                true) // prevent users from swiping away the notification & killing
                        // the service
                        .setContentIntent(pendingLaunchIntent)
                        .addAction(stopAction)
                        .build();
        startForeground(1, notification);

        statusPublisher.start();

        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.v(TAG, "onDestroy");

        getApplicationContext().unregisterReceiver(cancelBootstrapListener);

        statusPublisher.stop();
        stopReceiver.unregister(getApplicationContext());
    }

    /**********************************
     * IRaceService methods
     *********************************/

    @Override
    public void stopService() {
        Log.d(TAG, "Stopping RACE service");
        listeners.forEach(listener -> listener.onStopApp());

        if (raceApp != null) {
            Log.d(TAG, "Shutting down RACE app");
            raceApp.shutdown();
            raceApp = null;
        }
        if (raceSdk != null) {
            Log.d(TAG, "Shutting down RACE SDK");
            raceSdk.shutdown();
            raceSdk = null;
        }

        stopSelf();
        Log.d(TAG, "RACE service stopped");
    }

    @Override
    public void addServiceListener(IServiceListener listener) {
        Log.v(TAG, "Adding service listener");
        listeners.add(listener);
    }

    @Override
    public void removeServiceListener(IServiceListener listener) {
        Log.v(TAG, "Removing service listener");
        listeners.remove(listener);
    }

    @Override
    public boolean arePermissionsSufficient() {
        Context context = getApplicationContext();
        for (String permission : REQUIRED_PERMISSIONS) {
            if (ContextCompat.checkSelfPermission(context, permission)
                    != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    @Override
    public String[] getRequiredPermissions() {
        return REQUIRED_PERMISSIONS;
    }

    @Override
    public boolean areAssetsExtracted() {
        String[] expectedAssetDirs = {"python3.7", "race/python"};
        File appDataDir = getDataDir();
        for (String expectedDir : expectedAssetDirs) {
            File assetDir = new File(appDataDir, expectedDir);
            if (!assetDir.isDirectory()) {
                return false;
            }
        }
        return true;
    }

    @Override
    public boolean hasPlugins() {
        // At a minimum, only the artifact-manager plugins are required.
        File appDataDir = getDataDir();
        File ampPluginsDir = new File(appDataDir, "race/artifacts/artifact-manager");
        String[] list = ampPluginsDir.list();
        boolean result = ampPluginsDir.exists() && list != null && list.length != 0;
        return result;
    }

    @Override
    public boolean hasConfigs() {
        // Look for extracted configs first
        File externalRaceDir = getApplicationContext().getExternalFilesDir("race");
        File externalConfigsDir = new File(externalRaceDir, "data/configs");
        String[] list = externalConfigsDir.list();
        if (externalConfigsDir.exists() && list != null && list.length != 0) {
            return true;
        }
        // Then look for configs tar
        File externalConfigsTar = new File(externalRaceDir, "configs.tar.gz");
        return externalConfigsTar.exists();
    }

    @Override
    public boolean hasPersona() {
        return getPersona() != null;
    }

    @Override
    public String getPersona() {
        return getSharedPreferences(SHARED_PREF_FILE, Context.MODE_PRIVATE)
                .getString(SHARED_PREF_KEY_PERSONA, null);
    }

    @Override
    public void setPersona(String persona) {
        SharedPreferences sharedPreferences =
                getSharedPreferences(SHARED_PREF_FILE, Context.MODE_PRIVATE);
        sharedPreferences.edit().putString(SHARED_PREF_KEY_PERSONA, persona).apply();
        statusPublisher.setPersona(persona);
    }

    @Override
    public boolean hasPassphrase() {
        return passphrase != null;
    }

    @Override
    public void setPassphrase(String passphrase) {
        this.passphrase = passphrase;
    }

    @Override
    public boolean hasEnabledChannels() {
        if (raceSdk == null) {
            return false;
        }

        for (JChannelProperties channelProps : raceSdk.getAllChannelProperties()) {
            // Enabled channels can have status ENABLED, AVAILABLE, STARTING, FAILED, or UNAVAILABLE
            if (!channelProps.channelStatus.equals(ChannelStatus.CHANNEL_DISABLED)
                    && !channelProps.channelStatus.equals(ChannelStatus.CHANNEL_UNSUPPORTED)) {
                return true;
            }
        }

        return false;
    }

    @Override
    public void useInitialEnabledChannels() {
        if (raceSdk == null) {
            Log.e(
                    TAG,
                    "Attempt to use initial enabled channels but SDK has not been initialized yet");
            return;
        }

        Log.d(TAG, "Using initial enabled channels from config");
        raceSdk.setEnabledChannels(raceSdk.getInitialEnabledChannels());
    }

    @Override
    public void useEnabledChannels(String[] channels) {
        if (raceSdk == null) {
            Log.e(TAG, "Attempt to set enabled channels but SDK has not been initialized yet");
            return;
        }

        Log.d(TAG, "Setting enabled channels to " + Arrays.toString(channels));
        raceSdk.setEnabledChannels(channels);
    }

    @Override
    public List<JChannelProperties> getChannels(ChannelStatus status) {
        ArrayList<JChannelProperties> channels = new ArrayList<>();

        if (raceSdk == null) {
            Log.e(TAG, "Attempt to get channels but SDK has not been initialized yet");
            return channels;
        }

        for (JChannelProperties channelProps : raceSdk.getAllChannelProperties()) {
            if (status == null || channelProps.channelStatus.equals(status)) {
                channels.add(channelProps);
            }
        }

        return channels;
    }

    @Override
    public void setChannelEnabled(String channelId, boolean enabled) {
        if (raceSdk == null) {
            Log.e(TAG, "Attempt to set channel enabled but SDK has not been initialized yet");
            return;
        }

        Log.d(TAG, "Setting channel " + channelId + " enabled=" + enabled);
        if (enabled) {
            raceSdk.enableChannel(channelId);
        } else {
            raceSdk.disableChannel(channelId);
        }
    }

    @Override
    public boolean startRaceCommunications() {
        if (raceApp != null && raceApp.isInitialized()) {
            return true;
        }

        // Verify setup prerequisites have been completed
        if (!arePermissionsSufficient()
                || !hasPassphrase()
                || !areAssetsExtracted()
                || !hasPlugins()
                || !hasConfigs()
                || !hasPersona()) {
            Log.w(TAG, "Unable to start RACE comms due to insufficient setup prerequisites");
            return false;
        }

        if (raceSdk == null) {
            try {
                raceSdk = RaceSdkAppFactory.createInstance(this, getPersona(), this.passphrase);
                if (raceSdk == null) {
                    Log.e(TAG, "Unable to create RACE SDK");
                    return false;
                }
            } catch (StorageEncryptionInvalidPassphraseException err) {
                Log.e(TAG, "Invalid passphrase entered when creating RACE SDK. Try again...");
                this.passphrase = null;
                this.didReceiveInvalidPassphrase = true;
                return false;
            } catch (Exception err) {
                Log.e(TAG, "Exception thrown while creating RACE SDK: " + err.getMessage());
                throw err;
            }
            this.didReceiveInvalidPassphrase = false;
        }
        SharedPreferences sharedPreferences =
                getSharedPreferences(SHARED_PREF_FILE, Context.MODE_PRIVATE);
        sharedPreferences.edit().putBoolean(SHARED_PREF_KEY_PASSPHRASE_SET, true).apply();

        if (raceApp == null) {
            raceApp = AndroidRaceApp.createInstance(getApplicationContext(), raceSdk, this);
            if (raceApp == null) {
                Log.e(TAG, "Unable to create RACE app");
                return false;
            }
        }

        if (hasEnabledChannels() && !raceApp.isInitialized()) {
            raceApp.initialize();
        }

        actionReceiver.setRaceApp(raceApp);

        boolean initialized = raceApp.isInitialized();
        statusPublisher.setHasValidConfigs(initialized);
        return initialized;
    }

    @Override
    public void sendMessage(String message, String recipient) {
        if (raceApp == null || !raceApp.isInitialized()) {
            Log.e(TAG, "Attempt to send message but RACE app has not been initialized yet");
            return;
        }

        raceApp.sendMessage(message, recipient);
    }

    @Override
    public void suspendMessageNotificationsFor(String recipient) {
        suspendMessageNotificationsPersona.set(recipient);
        notificationManager.cancel(recipient, MESSAGE_NOTIF_ID);
    }

    @Override
    public void processUserInputResponse(RaceHandle handle, boolean answered, String response) {
        if (raceApp == null) {
            Log.e(
                    TAG,
                    "Attempt to respond to user input but RACE app has not been initialized yet");
            return;
        }

        raceApp.processUserInputResponse(handle, answered, response);
        notificationManager.cancel(String.valueOf(handle.getValue()), ALERT_INPUT_NOTIF_ID);
    }

    @Override
    public void acknowledgeAlert(RaceHandle handle) {
        if (raceSdk == null) {
            Log.e(TAG, "Attempt to acknowledge alert but SDK has not been initialized yet");
            return;
        }

        if (handle.getValue() != 0l) {
            raceSdk.onUserAcknowledgementReceived(handle);
        }
        notificationManager.cancel(String.valueOf(handle.getValue()), ALERT_DISPLAY_NOTIF_ID);
    }

    @Override
    public void prepareToBootstrap(
            String platform,
            String arch,
            String nodeType,
            String passphrase,
            String bootstrapChannelId) {
        if (raceSdk == null) {
            Log.e(TAG, "Attempt to prepare to bootstrap but SDK has not been initialized yet");
            return;
        }
        currentBootstrapHandle =
                raceSdk.prepareToBootstrap(
                        platform, arch, nodeType, passphrase, bootstrapChannelId);
    }

    /**********************************
     * IServiceListener proxy methods
     *********************************/

    @Override
    public void onStopApp() {}

    @Override
    public void onRaceStatusChange(PluginStatus networkManagerStatus) {
        listeners.forEach(listener -> listener.onRaceStatusChange(networkManagerStatus));
        statusPublisher.setNMStatus(networkManagerStatus);
    }

    @Override
    public void onMessageAdded(JClrMsg message) {
        executor.execute(() -> addMessageToDb(message));
        listeners.forEach(listener -> listener.onMessageAdded(message));
        showMessageNotification(message);
    }

    @Override
    public void onUserInputRequested(RaceHandle handle, String prompt) {
        listeners.forEach(listener -> listener.onUserInputRequested(handle, prompt));
        if (listeners.isEmpty()) {
            showUserInputNotification(handle, prompt);
        }
    }

    @Override
    public void onDisplayAlert(RaceHandle handle, String message, UserDisplayType displayType) {
        listeners.forEach(listener -> listener.onDisplayAlert(handle, message, displayType));
        if (listeners.isEmpty() || displayType.equals(UserDisplayType.UD_NOTIFICATION)) {
            showDisplayAlertNotification(handle, message, displayType);
        }
    }

    @Override
    public void onDisplayBootstrapInfo(
            RaceHandle handle,
            String message,
            UserDisplayType displayType,
            BootstrapActionType actionType) {
        listeners.forEach(
                listener ->
                        listener.onDisplayBootstrapInfo(handle, message, displayType, actionType));

        if (actionType.equals(BootstrapActionType.BS_COMPLETE)
                || actionType.equals(BootstrapActionType.BS_FAILED)) {
            // Clear out previous bootstrapping related notifications
            notificationManager.cancel(BOOTSTRAP_NOTIF_TAG, BOOTSTRAP_NOTIF_ID);

            if (actionType.equals(BootstrapActionType.BS_FAILED) && listeners.isEmpty()) {
                // use big text Notification to communicate target retry instructions if there are
                // no listeners
                // if MainActivity is listening the user is presented a dialog that the user must
                // dismiss
                showDisplayAlertNotification(
                        handle,
                        message,
                        displayType,
                        BOOTSTRAP_NOTIF_TAG,
                        BOOTSTRAP_NOTIF_ID,
                        false,
                        null,
                        getString(R.string.bootstrap_failed_instruction));
            } else {
                // Show final bootstrap message as a normal alert
                showDisplayAlertNotification(handle, message, displayType);
            }
        } else if (listeners.isEmpty() || displayType.equals(UserDisplayType.UD_NOTIFICATION)) {
            boolean showProgressBar =
                    actionType.equals(BootstrapActionType.BS_PREPARING_BOOTSTRAP)
                            || actionType.equals(BootstrapActionType.BS_PREPARING_CONFIGS)
                            || actionType.equals(BootstrapActionType.BS_ACQUIRING_ARTIFACT)
                            || actionType.equals(BootstrapActionType.BS_CREATING_BUNDLE)
                            || actionType.equals(BootstrapActionType.BS_PREPARING_TRANSFER);

            Intent cancelIntent = new Intent(ACTION_CANCEL_BOOTSTRAP);
            PendingIntent pCancelIntent =
                    PendingIntent.getBroadcast(
                            this,
                            nextRequestCode.incrementAndGet(),
                            cancelIntent,
                            PendingIntent.FLAG_IMMUTABLE);
            String name = getString(android.R.string.cancel);
            NotificationCompat.Action action =
                    new NotificationCompat.Action(R.drawable.ic_done, name, pCancelIntent);

            showDisplayAlertNotification(
                    handle,
                    message,
                    displayType,
                    BOOTSTRAP_NOTIF_TAG,
                    BOOTSTRAP_NOTIF_ID,
                    showProgressBar,
                    action,
                    null);
        } else if (!displayType.equals(UserDisplayType.UD_NOTIFICATION)) {
            // If we're hitting this, it means the alert wasn't a notification type (e.g., QR code)
            // AND it got displayed by the activity (otherwise listeners would have been empty and
            // the prior conditional block would have been hit), so we want to clear out any prior
            // notification
            notificationManager.cancel(BOOTSTRAP_NOTIF_TAG, BOOTSTRAP_NOTIF_ID);
        }
    }

    @Override
    public boolean isPassphraseSet() {
        SharedPreferences sharedPreferences =
                getSharedPreferences(SHARED_PREF_FILE, Context.MODE_PRIVATE);
        return sharedPreferences.getBoolean(SHARED_PREF_KEY_PASSPHRASE_SET, false);
    }

    @Override
    public boolean receivedInvalidPassphrase() {
        return this.didReceiveInvalidPassphrase;
    }

    /**********************************
     * Message/notification methods
     *********************************/

    private void addMessageToDb(JClrMsg clrMsg) {
        // convId is other client involved in the conversation because we're assuming no
        // group messages
        String myPersona = getPersona();

        Date date = new Date();
        DateFormat formatter = new SimpleDateFormat("MM-dd-yyyy HH:mm:ss");
        formatter.setTimeZone(TimeZone.getTimeZone("EST"));
        String dateFormatted = formatter.format(date);

        Message message = new Message();
        // ClrMsg times are in microseconds, Date requires milliseconds
        message.createTime = formatter.format(new Date(clrMsg.createTime / 1000));
        message.plainMsg = clrMsg.plainMsg;

        if (myPersona.equals(clrMsg.fromPersona)) {
            // This is a send message
            message.convId = clrMsg.toPersona;
            message.fromPersona = clrMsg.fromPersona;
            message.timeReceived = null;

            // Add Conversation if it doesn't exist
            // (shouldn't be an issue if it already exists)
            if (db.conversationDao().getConversation(clrMsg.toPersona) == null) {
                Conversation conversation = new Conversation(clrMsg.toPersona);
                conversation.participants.add(clrMsg.toPersona);
                db.conversationDao().insertConversation(conversation);
            }
        } else {
            // This is a received message
            message.convId = clrMsg.fromPersona;
            message.fromPersona = clrMsg.fromPersona;
            message.timeReceived = dateFormatted;

            // Add Conversation if it doesn't exist
            // (shouldn't be an issue if it already exists)
            if (db.conversationDao().getConversation(clrMsg.fromPersona) == null) {
                Conversation conversation = new Conversation(clrMsg.fromPersona);
                conversation.participants.add(clrMsg.fromPersona);
                db.conversationDao().insertConversation(conversation);
            }
        }

        db.conversationDao().insertMessage(message);
    }

    private void showMessageNotification(JClrMsg message) {
        if (message.fromPersona.equals(getPersona())) {
            // This is a send message, no notification
            return;
        }

        if (message.fromPersona.equals(suspendMessageNotificationsPersona.get())) {
            // Explicitly told to suspend notifications from this sender
            return;
        }

        // This PendingIntent uses fixed request code but persona-specific intent identifier
        Intent intent = new Intent(this, MainActivity.class);
        intent.setAction(MainActivity.ACTION_OPEN_CONVERSATION);
        intent.setIdentifier(message.fromPersona);
        intent.putExtra(MainActivity.ARG_CONVERSATION_ID, message.fromPersona);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        PendingIntent pendingIntent =
                PendingIntent.getActivity(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);

        Notification notification =
                new NotificationCompat.Builder(this, MESSAGE_CHANNEL_ID)
                        .setSmallIcon(R.mipmap.ic_launcher)
                        // TODO use customized nickname rather than persona
                        .setContentTitle(
                                getString(R.string.service_unread_messages, message.fromPersona))
                        .setContentIntent(pendingIntent)
                        .build();
        notificationManager.notify(message.fromPersona, MESSAGE_NOTIF_ID, notification);
    }

    private void showUserInputNotification(RaceHandle handle, String prompt) {
        Log.v(TAG, "showUserInputNotification");
        // This PendingIntent uses unique request codes
        Intent intent = new Intent(this, MainActivity.class);
        intent.setAction(MainActivity.ACTION_RESPOND_TO_INPUT);
        intent.putExtra(MainActivity.ARG_INPUT_HANDLE, handle.getValue());
        intent.putExtra(MainActivity.ARG_INPUT_PROMPT, prompt);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        PendingIntent pendingIntent =
                PendingIntent.getActivity(
                        this,
                        nextRequestCode.incrementAndGet(),
                        intent,
                        PendingIntent.FLAG_UPDATE_CURRENT);

        Notification notification =
                new NotificationCompat.Builder(this, ALERT_CHANNEL_ID)
                        .setSmallIcon(R.mipmap.ic_launcher)
                        .setContentTitle(getString(R.string.service_input_required))
                        .setContentIntent(pendingIntent)
                        .build();
        notificationManager.notify(
                String.valueOf(handle.getValue()), ALERT_INPUT_NOTIF_ID, notification);
    }

    private void showDisplayAlertNotification(
            RaceHandle handle, String message, UserDisplayType displayType) {
        showDisplayAlertNotification(
                handle,
                message,
                displayType,
                String.valueOf(handle.getValue()),
                ALERT_DISPLAY_NOTIF_ID,
                false,
                null,
                null);
    }

    private void showDisplayAlertNotification(
            RaceHandle handle,
            String message,
            UserDisplayType displayType,
            String tag,
            int notifId,
            boolean progress,
            NotificationCompat.Action cancelAction,
            String extraText) {
        String contentText = message;
        PendingIntent contentIntent = null;

        switch (displayType) {
            case UD_TOAST:
            case UD_NOTIFICATION:
                // Keep default content text
                // Auto-acknowledge the alert
                raceSdk.onUserAcknowledgementReceived(handle);
                break;

            default:
                // For all other types, clicking on the notification will launch the activity and
                // display the appropriate alert (dialog, QR code, etc.)
                if (message.length() > 30) {
                    contentText = message.substring(0, 25) + "...";
                }

                // This PendingIntent uses unique request codes
                Intent intent = new Intent(this, MainActivity.class);
                intent.setAction(MainActivity.ACTION_DISPLAY_ALERT);
                intent.putExtra(MainActivity.ARG_ALERT_HANDLE, handle.getValue());
                intent.putExtra(MainActivity.ARG_ALERT_MESSAGE, message);
                intent.putExtra(MainActivity.ARG_ALERT_TYPE, displayType.getValue());
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
                contentIntent =
                        PendingIntent.getActivity(
                                this,
                                nextRequestCode.incrementAndGet(),
                                intent,
                                PendingIntent.FLAG_IMMUTABLE);
        }

        NotificationCompat.Builder notificationBuilder =
                new NotificationCompat.Builder(this, ALERT_CHANNEL_ID);
        if (extraText != null && extraText.isEmpty() == false) {
            String fullText = message + "\n" + extraText;
            notificationBuilder.setStyle(new NotificationCompat.BigTextStyle().bigText(fullText));
        } else {
            notificationBuilder.setContentText(contentText);
        }
        notificationBuilder
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentTitle(getString(R.string.app_name))
                .setContentIntent(contentIntent)
                .setAutoCancel(true);

        if (progress) {
            notificationBuilder.setProgress(100, 0, true);
        }
        if (cancelAction != null) {
            notificationBuilder.addAction(cancelAction);
        }

        Notification notification = notificationBuilder.build();
        notificationManager.notify(tag, notifId, notification);
    }
}
