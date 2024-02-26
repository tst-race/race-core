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

package com.twosix.race.service;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import ShimsJava.PluginStatus;

import com.twosix.race.core.Constants;

import org.json.simple.JSONObject;
import org.json.simple.parser.*;

import java.io.File;
import java.nio.file.Files;
import java.time.Instant;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.TimeZone;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

public class RaceAppStatusPublisher {

    private static final String TAG = "RaceAppStatusPublisher";

    // defaults for if file is not found
    private static final long PUBLISH_PERIOD_SEC = 3;
    private static final long TTL_FACTOR = 3;

    private long period = PUBLISH_PERIOD_SEC;
    private long time_to_live = TTL_FACTOR;

    private final DateTimeFormatter dateTimeFormatter =
            DateTimeFormatter.ISO_LOCAL_DATE_TIME.withZone(
                    ZoneId.of(TimeZone.getDefault().getID()));

    private final Context context;
    private final ScheduledExecutorService executorService;
    private ScheduledFuture<?> future;

    // Cached status values
    private String persona;
    private boolean hasValidConfigs = false;
    private PluginStatus networkManagerStatus = PluginStatus.PLUGIN_NOT_READY;

    public RaceAppStatusPublisher(Context context, String persona) {
        this.context = context;
        this.persona = persona;
        if (this.persona == null) {
            this.persona = "";
        }

        try {
            // try opening file
            File testAppConfigFile = new File(Constants.PATH_TO_TESTAPP_CONFIG);
            if (!testAppConfigFile.exists()) {
                // file can't be found, defaults will be used
                throw new Exception("File testapp-config.json does not exist");
            }

            String testAppConfigFileContents =
                    new String(Files.readAllBytes(testAppConfigFile.toPath())).trim();

            // parse file into JSON Object
            JSONParser jsParse = new JSONParser();
            JSONObject testAppConfigJSON = (JSONObject) jsParse.parse(testAppConfigFileContents);

            // use JSON Object to populate values into ttl and period
            this.period = (long) testAppConfigJSON.get("period");
            this.time_to_live = (long) testAppConfigJSON.get("ttl-factor");
        } catch (Exception e) {
            // reset to defaults here
            period = PUBLISH_PERIOD_SEC;
            time_to_live = TTL_FACTOR;

            // log that defaults are being used
            Log.e(TAG, "Error opening testapp-config.json, using defaults", e);
        }

        executorService = Executors.newScheduledThreadPool(1);
    }

    public synchronized void start() {
        if (future != null) {
            Log.w(TAG, "Publisher has already been started");
            return;
        }

        Log.i(TAG, "Starting app status publisher");
        future = executorService.scheduleAtFixedRate(this::publish, 0, period, TimeUnit.SECONDS);
    }

    public synchronized void stop() {
        if (future == null) {
            Log.w(TAG, "Publisher is already stopped");
            return;
        }

        Log.i(TAG, "Stopping app status publisher");
        future.cancel(false);
        future = null;

        // Run one last status publish with the stopped status
        executorService.execute(this::publish);
    }

    public synchronized void setPersona(String persona) {
        this.persona = persona;
    }

    public synchronized void setHasValidConfigs(boolean hasValidConfigs) {
        this.hasValidConfigs = hasValidConfigs;
    }

    public synchronized void setNMStatus(PluginStatus status) {
        networkManagerStatus = status;
    }

    public synchronized void publish() {
        try {
            JSONObject appStatus = new JSONObject();
            appStatus.put("timestamp", dateTimeFormatter.format(Instant.now()));
            appStatus.put("persona", persona);

            JSONObject raceStatus = new JSONObject();
            raceStatus.put("validConfigs", hasValidConfigs);
            raceStatus.put("network-manager-status", networkManagerStatus.toString());
            appStatus.put("RaceStatus", raceStatus);

            Intent intent = new Intent();
            intent.setAction(Constants.UPDATE_APP_STATUS_ACTION);
            intent.putExtra("status", appStatus.toString());

            // If the scheduled task isn't running, use a 1-sec TTL to quickly report the app as
            // stopped
            if (future == null) {
                intent.putExtra("ttl", 1L);
            } else {
                intent.putExtra("ttl", period * time_to_live);
            }
            context.sendBroadcast(intent);
        } catch (Exception e) {
            Log.e(TAG, "Error updating app status", e);
        }
    }
}
