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
import android.content.pm.PackageManager;

import com.twosix.race.daemon.sdk.IRaceNodeDaemonSdk;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.time.Instant;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.TimeZone;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class RaceNodeStatusPublisher {

    private static final long DEFAULT_PUBLISH_PERIOD_SEC = 3;
    private static final long DEFAULT_TTL_FACTOR = 3;

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final DateTimeFormatter dateTimeFormatter =
            DateTimeFormatter.ISO_LOCAL_DATE_TIME.withZone(
                    ZoneId.of(TimeZone.getDefault().getID()));

    private final String persona;
    private final IRaceNodeDaemonSdk sdk;
    private final PackageManager packageManager;
    private final ScheduledExecutorService executorService;

    private long publishPeriodSec = DEFAULT_PUBLISH_PERIOD_SEC;
    private long ttlFactor = DEFAULT_TTL_FACTOR;
    private Future<?> future;

    RaceNodeStatusPublisher(String persona, IRaceNodeDaemonSdk sdk, Context context) {
        this.persona = persona;
        this.sdk = sdk;

        packageManager = context.getPackageManager();

        executorService = Executors.newScheduledThreadPool(1);
    }

    /**
     * Starts the publishing of node status at a fixed rate.
     *
     * @param publishPeriodSec New publishing rate in seconds, or null to keep the current period
     * @param ttlFactor New TTL factor to use for status keep-alive, or null to keep the current
     *     factor
     */
    public void start(Long publishPeriodSec, Long ttlFactor) {
        if (publishPeriodSec != null) {
            this.publishPeriodSec = publishPeriodSec;
        }
        if (ttlFactor != null) {
            this.ttlFactor = ttlFactor;
        }
        if (future != null) {
            future.cancel(false);
        }
        future =
                executorService.scheduleAtFixedRate(
                        this::publish, 0, this.publishPeriodSec, TimeUnit.SECONDS);
    }

    /** Publishes the current node status through the daemon SDK. */
    public void publish() {
        if (sdk.isActionListenerRegistered()) {
            try {
                JSONObject nodeStatus = new JSONObject();
                nodeStatus.put("timestamp", dateTimeFormatter.format(Instant.now()));
                nodeStatus.put("persona", persona);
                nodeStatus.put("installed", isRaceAppInstalled());
                nodeStatus.put("configsPresent", areConfigsPresent());
                nodeStatus.put("configsExtracted", areConfigsExtracted());
                nodeStatus.put("userResponsesExists", doesUserResponesesFileExist());
                nodeStatus.put("jaegerConfigExists", doesJaegerConfigFileExist());
                nodeStatus.put("deployment", sdk.getDeploymentName());
                nodeStatus.put("dnsSuccessful", sdk.isDnsSuccessful());

                nodeStatus.put("nodePlatform", "android");
                nodeStatus.put("nodeArchitecture", getNodeArchitecture());

                sdk.updateNodeStatus(nodeStatus.toString(), this.publishPeriodSec * this.ttlFactor);
            } catch (Exception e) {
                logger.error("Error updating node status", e);
            }
        }
    }

    /**
     * Checks if the RACE app is currently installed on the device.
     *
     * <p>Installation is determined by checking if the package is found in the Android package
     * manager.
     *
     * @return true if the RACE app is installed
     */
    private boolean isRaceAppInstalled() {
        try {
            packageManager.getPackageInfo(Constants.RACE_APP_PACKAGE, 0);
            return true;
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        } catch (Exception e) {
            logger.warn("Error checking RACE app installation", e);
            return false;
        }
    }

    private boolean doesJaegerConfigFileExist() {
        return new File("/storage/self/primary/Download/race/etc/jaeger-config.yml").isFile();
    }

    private boolean doesUserResponesesFileExist() {
        return new File("/storage/self/primary/Download/race/etc/user-responses.json").isFile();
    }

    /**
     * Checks if the RACE configs exist where the deamon expects them on the device.
     *
     * @return true if the configs exist in the downloads directory
     */
    private boolean areConfigsPresent() {
        // TODO: don't hardcode this file path. Store as static member in some central location?
        // PullConfigsAction class also uses this.
        // NOTE: this name is/should be set by RiB, so may need to be dynamic. Need to think on how
        // to do that.
        return new File("/storage/self/primary/Download/race/configs.tar.gz").isFile();
    }

    /**
     * Checks if the RACE configs have been extracted.
     *
     * @return true if the configs have been extracted
     */
    private boolean areConfigsExtracted() {
        return new File(
                        "/storage/self/primary/Android/data/com.twosix.race/files/race/data/configs/sdk")
                .exists();
    }

    /**
     * Checks node architecture
     *
     * @return node architecture
     */
    private String getNodeArchitecture() {
        return System.getProperty("os.arch");
    }
}
