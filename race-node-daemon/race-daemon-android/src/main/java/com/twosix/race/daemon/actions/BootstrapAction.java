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

package com.twosix.race.daemon.actions;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;

import com.twosix.race.daemon.Constants;
import com.twosix.race.daemon.sdk.FileUtils;
import com.twosix.race.daemon.sdk.IRaceNodeDaemonSdk;

import org.json.JSONException;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Duration;

public class BootstrapAction implements NodeAction {

    public static final String TYPE = "bootstrap";

    private static final int DEFAULT_TIMEOUT_SECS = 600;

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final Context context;

    private final IRaceNodeDaemonSdk sdk;

    private final String persona;

    public BootstrapAction(Context context, IRaceNodeDaemonSdk sdk, String persona) {
        this.context = context;
        this.sdk = sdk;
        this.persona = persona;
    }

    /**
     * Bootstraps the current node by installing the RACE app from an introducer node.
     *
     * <ol>
     *   <li>Retrieve configs from the RiB file server in order to get the user responses file
     *   <li>Retrieve the bootstrap bundle from the introducer node
     *   <li>Extract the bootstrap bundle and move configs and plugins into shared external storage
     *   <li>Send an intent to prompt the user to install the RACE app
     * </ol>
     *
     * @param jsonPayload Action payload containing the introducer and passphrase
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        try {
            logger.info("Installing RACE from introducer node");

            String bootstrapBundleUrl = jsonPayload.getString("bootstrap-bundle-url");
            String deploymentName = jsonPayload.getString("deployment-name");

            // Save deployment name
            if (!sdk.saveDaemonStateInfo("deployment", deploymentName)) {
                logger.error("failed to set deployment name");
                return;
            }

            int timeoutSecs = DEFAULT_TIMEOUT_SECS;
            if (jsonPayload.has("timeout-secs")) {
                timeoutSecs = jsonPayload.getInt("timeout-secs");
            }
            Duration timeout = Duration.ofSeconds(timeoutSecs);

            Path bootstrapZipFile = Paths.get(FileUtils.getTempDirectory(), "bootstrap.zip");
            File configsTarFile = new File(FileUtils.getTempDirectory(), "configs.tar.gz");
            File rootExtDir = new File("/storage/self/primary/Download/race");

            if (!rootExtDir.exists()) {
                if (!rootExtDir.mkdirs()) {
                    logger.error("Unable to create " + rootExtDir.getAbsolutePath());
                    return;
                }
            }

            // Get the user responses from the central file server
            sdk.fetchFromFileServer(
                    deploymentName + "_" + this.persona + "_configs.tar.gz",
                    configsTarFile,
                    (success) -> {
                        try {
                            if (success) {
                                File configsDir =
                                        new File(FileUtils.getTempDirectory(), "race-configs");
                                FileUtils.extractArchive(configsTarFile, configsDir, true);

                                Path etcDir = configsDir.toPath().resolve("etc");
                                if (etcDir.toFile().exists()) {
                                    File extEtcDir = new File(rootExtDir, "etc");
                                    FileUtils.copyDirectory(etcDir, extEtcDir.toPath(), true);
                                }

                                if (!configsTarFile.delete()) {
                                    logger.warn("Unable to delete " + configsTarFile);
                                }
                                if (!FileUtils.deleteFilesInDirectory(configsDir)) {
                                    logger.warn("Unable to delete files in " + configsDir);
                                }
                                if (!configsDir.delete()) {
                                    logger.warn("Unable to delete " + configsDir);
                                }
                            }
                        } catch (IOException e) {
                            logger.error("Unable to obtain user responses: " + e.getMessage());
                        }
                    },
                    timeout);

            URL url = new URL(bootstrapBundleUrl);

            logger.debug("Bootstrap node-daemon looking up: {}", url);

            // Get the bootstrap bundle from the specified location
            sdk.fetchRemoteFile(
                    url,
                    bootstrapZipFile.toFile(),
                    (success) -> {
                        if (success) {
                            // Extract the bootstrap bundle archive to external shared storage
                            try {
                                FileUtils.extractZipFile(bootstrapZipFile.toFile(), rootExtDir);
                            } catch (IOException e) {
                                logger.error("Failed to extract bootstrap bundle", e);
                                return;
                            }

                            // Install app
                            try {
                                Path appApk = rootExtDir.toPath().resolve("race/race.apk");
                                logger.info("Installing " + appApk);

                                PackageManager packageManager = context.getPackageManager();
                                PackageInstaller packageInstaller =
                                        packageManager.getPackageInstaller();
                                PackageInstaller.SessionParams params =
                                        new PackageInstaller.SessionParams(
                                                PackageInstaller.SessionParams.MODE_FULL_INSTALL);
                                params.setAppPackageName(Constants.RACE_APP_PACKAGE);

                                int sessionId = packageInstaller.createSession(params);
                                PackageInstaller.Session session =
                                        packageInstaller.openSession(sessionId);
                                try (OutputStream out = session.openWrite("package", 0, -1)) {
                                    Files.copy(appApk, out);
                                }

                                Intent callbackIntent = new Intent(Constants.APP_INSTALLED_ACTION);
                                session.commit(
                                        PendingIntent.getBroadcast(
                                                        context, sessionId, callbackIntent, 0)
                                                .getIntentSender());
                            } catch (Exception e) {
                                logger.error("Failed to install", e);
                            }

                            if (!bootstrapZipFile.toFile().delete()) {
                                logger.warn("Unable to delete " + bootstrapZipFile);
                            }
                        }
                    },
                    timeout);

        } catch (JSONException e) {
            logger.error("Invalid bootstrap action received: " + e.getMessage());
        } catch (MalformedURLException e) {
            logger.error("Error creating bootstrap bundle URL: " + e.getMessage());
        }
    }
}
