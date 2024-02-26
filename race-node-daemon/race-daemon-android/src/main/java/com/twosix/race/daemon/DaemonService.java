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

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.IBinder;

import com.twosix.race.daemon.sdk.IRaceNodeDaemonSdk;
import com.twosix.race.daemon.sdk.RaceNodeDaemonConfig;
import com.twosix.race.daemon.sdk.RaceNodeDaemonSdk;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Foreground service to keep the daemon application alive while other apps are in the foreground.
 */
public class DaemonService extends Service {

    private static final String CHANNEL_ID = "com.twosix.race.daemon.CHANNEL";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private IRaceNodeDaemonSdk sdk;
    private RaceNodeStatusPublisher nodeStatusPublisher;

    public DaemonService() {}

    /** Creates the notification channel to be used for the foreground notification. */
    @Override
    public void onCreate() {
        logger.debug("Creating Daemon service");
        super.onCreate();

        NotificationChannel channel =
                new NotificationChannel(
                        CHANNEL_ID, "RACE Daemon", NotificationManager.IMPORTANCE_MIN);
        NotificationManager notificationManager = getSystemService(NotificationManager.class);
        notificationManager.createNotificationChannel(channel);
    }

    /**
     * Starts the daemon foreground service.
     *
     * <ol>
     *   <li>Starts the foreground notification
     *   <li>Initializes the daemon SDK
     *   <li>Registers the RACE app status broadcast intent receiver
     *   <li>Starts publishing periodic node status updates
     * </ol>
     *
     * @param intent Intent that started the service
     * @param flags Not used
     * @param startId Not used
     * @return START_NOT_STICKY so that if the daemon dies it is not restarted
     */
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        logger.debug("Starting Daemon service");

        String persona = intent.getStringExtra("persona");
        if (persona == null || persona.isEmpty()) {
            logger.error("Persona is empty, unable to start daemon service");
            return START_NOT_STICKY;
        }

        Notification notification =
                new Notification.Builder(this, CHANNEL_ID)
                        .setContentTitle("RACE Daemon " + persona)
                        .setSmallIcon(R.mipmap.ic_launcher)
                        .build();
        startForeground(1, notification);

        RaceNodeDaemonConfig config = new RaceNodeDaemonConfig();
        config.setDaemonStateInfoJsonPath("/storage/self/primary/Download/daemon-state-info.json");
        config.setPersona(persona);
        config.setIsGenesis(isRaceAppInstalled(this));

        sdk = new RaceNodeDaemonSdk(config);

        RaceAppStatusReceiver appStatusReceiver = new RaceAppStatusReceiver(sdk);
        IntentFilter appStatusIntentFilter = new IntentFilter(Constants.UPDATE_APP_STATUS_ACTION);
        registerReceiver(appStatusReceiver, appStatusIntentFilter);

        InstallStatusReceiver installStatusReceiver = new InstallStatusReceiver();
        IntentFilter installStatusIntentFilter = new IntentFilter(Constants.APP_INSTALLED_ACTION);
        registerReceiver(installStatusReceiver, installStatusIntentFilter);

        BootstrapInfoReceiver bootstrapInfoReceiver = new BootstrapInfoReceiver(sdk);
        IntentFilter bootstrapInfoIntentFilter =
                new IntentFilter(Constants.FORWARD_BOOTSTRAP_INFO_ACTION);
        registerReceiver(bootstrapInfoReceiver, bootstrapInfoIntentFilter);

        nodeStatusPublisher = new RaceNodeStatusPublisher(persona, sdk, this);

        sdk.registerActionListener(new NodeActionListener(persona, sdk, nodeStatusPublisher, this));

        nodeStatusPublisher.start(null, null);

        return START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    /**
     * Checks if the RACE app is currently installed on the device.
     *
     * <p>Installation is determined by checking if the package is found in the Android package
     * manager.
     *
     * @return true if the RACE app is installed
     */
    private boolean isRaceAppInstalled(Context context) {
        try {
            PackageManager packageManager = context.getPackageManager();
            packageManager.getPackageInfo(Constants.RACE_APP_PACKAGE, 0);
            return true;
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        } catch (Exception e) {
            logger.warn("Error checking RACE app installation", e);
            return false;
        }
    }
}
