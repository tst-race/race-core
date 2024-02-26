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

package com.twosix.race.daemon.sdk;

import org.json.JSONObject;
import org.json.JSONTokener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import redis.clients.jedis.Jedis;
import redis.clients.jedis.JedisPool;
import redis.clients.jedis.JedisPoolConfig;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.net.UnknownHostException;
import java.nio.channels.Channels;
import java.nio.channels.ReadableByteChannel;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Duration;
import java.time.Instant;
import java.util.Objects;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;

public class RaceNodeDaemonSdk implements IRaceNodeDaemonSdk {

    // We create a pool of core size 3 because...
    // 1 thread for the actions subscription (blocking)
    // 1 thread for periodic status updates
    // 1 thread for action processing (e.g., bootstrapping)
    private static final int CORE_THREAD_POOL_SIZE = 3;

    private static final String BASE_ACTIONS_CHANNEL = "race.node.actions:";
    private static final String BASE_NODE_STATUS_KEY = "race.node.status:";
    private static final String BASE_NODE_IS_ALIVE_KEY = "race.node.is.alive:";
    private static final String BASE_APP_STATUS_KEY = "race.app.status:";
    private static final String BASE_APP_IS_ALIVE_KEY = "race.app.is.alive:";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final RaceNodeDaemonConfig config;
    private final String persona;
    private final ScheduledExecutorService executorService;
    private final JedisPool jedisPool;

    private ActionListenerWrapper actionListenerWrapper = null;

    /**
     * Creates an instance of the daemon SDK.
     *
     * @param config Daemon SDK configuration
     */
    public RaceNodeDaemonSdk(RaceNodeDaemonConfig config) {
        this.config = config;
        persona = config.getPersona();
        logger.debug("RACE node daemon SDK initializing for persona: {}", persona);

        executorService = Executors.newScheduledThreadPool(CORE_THREAD_POOL_SIZE);

        JedisPoolConfig jedisPoolConfig = new JedisPoolConfig();
        // Need to disable JMX for Android, and we have no need for it on Linux anyway
        jedisPoolConfig.setJmxEnabled(false);

        jedisPool = new JedisPool(jedisPoolConfig, config.getRedisHost(), config.getRedisPort());
        logger.debug(
                "Redis client configured for {}:{}", config.getRedisHost(), config.getRedisPort());
    }

    @Override
    public boolean isAppAlive() {
        String isAliveKey = BASE_APP_IS_ALIVE_KEY + persona;
        String isAliveValue;

        try (Jedis jedis = jedisPool.getResource()) {
            isAliveValue = jedis.get(isAliveKey);
        }
        return (Objects.equals(isAliveValue, "true"));
    }

    @Override
    public void updateAppStatus(String jsonStatus, long expirationSeconds) {
        executorService.submit(
                () -> {
                    String isAliveKey = BASE_APP_IS_ALIVE_KEY + persona;
                    String statusKey = BASE_APP_STATUS_KEY + persona;
                    logger.trace("Updating app status, key: {}, status: {}", statusKey, jsonStatus);
                    try (Jedis jedis = jedisPool.getResource()) {
                        jedis.set(statusKey, jsonStatus);
                        jedis.setex(isAliveKey, expirationSeconds, "true");
                    }
                });
    }

    @Override
    public void updateNodeStatus(String jsonStatus, long expirationSeconds) {
        executorService.submit(
                () -> {
                    String isAliveKey = BASE_NODE_IS_ALIVE_KEY + persona;
                    String statusKey = BASE_NODE_STATUS_KEY + persona;
                    logger.trace(
                            "Updating node status, key: {}, status: {}", statusKey, jsonStatus);
                    try (Jedis jedis = jedisPool.getResource()) {
                        jedis.set(statusKey, jsonStatus);
                        jedis.setex(isAliveKey, expirationSeconds, "true");
                    }
                });
    }

    @Override
    public void sendBootstrapInfo(String target, String message, String actionType) {
        executorService.submit(
                () -> {
                    try {
                        JSONObject action = new JSONObject();
                        JSONObject payload = new JSONObject();

                        if (actionType.equals("BS_NETWORK_CONNECT")) {
                            logger.error("Not implemented (contact TwoSix)");
                            // TODO implement this
                            return;
                        } else if (actionType.equals("BS_DOWNLOAD_BUNDLE")) {
                            action.put("type", "bootstrap");
                            payload.put("bootstrap-bundle-url", message);
                            payload.put("deployment-name", getDeploymentName());
                        } else {
                            logger.error("Unrecognized bootstrap info action type: " + actionType);
                            return;
                        }

                        action.put("payload", payload);
                        String jsonAction = action.toString();

                        String channel = BASE_ACTIONS_CHANNEL + target;
                        logger.info("Publishing bootstrap action to {}: {}", channel, jsonAction);

                        try (Jedis jedis = jedisPool.getResource()) {
                            jedis.publish(channel, jsonAction);
                        }
                    } catch (Exception e) {
                        logger.error(
                                "Unable to send bootstrap info to target: "
                                        + e.getClass().getName()
                                        + ": "
                                        + e.getMessage());
                    }
                });
    }

    @Override
    public void registerActionListener(IRaceNodeActionListener listener) {
        if (actionListenerWrapper == null) {
            actionListenerWrapper = new ActionListenerWrapper(listener);
            Runnable[] r = new Runnable[1];
            r[0] =
                    () -> {
                        String channel = BASE_ACTIONS_CHANNEL + persona;
                        logger.debug("Subscribing to channel {}", channel);
                        try (Jedis jedis = jedisPool.getResource()) {
                            jedis.subscribe(actionListenerWrapper, channel);
                            logger.debug("Subscribed to channel {}", channel);
                        } catch (Exception e) {
                            logger.error(
                                    "Failed to subscribe to channel {}, rescheduling", channel);
                            executorService.schedule(r[0], 1, TimeUnit.SECONDS);
                        }
                    };
            executorService.submit(r[0]);
        }
    }

    @Override
    public boolean isActionListenerRegistered() {
        if (actionListenerWrapper == null) {
            return false;
        }
        return actionListenerWrapper.getSubscribedStatus();
    }

    @Override
    public void rotateLogs(File logsDir, boolean delete, String backupId) {
        executorService.submit(
                () -> {
                    try {
                        if (backupId != null && !backupId.isEmpty()) {
                            logger.info("Backing up files in " + logsDir.getAbsolutePath());

                            Path logsTarFile =
                                    Paths.get(
                                            FileUtils.getTempDirectory(),
                                            "logs-" + backupId + "-" + persona + ".tar.gz");
                            logger.debug("Creating temporary archive " + logsTarFile);
                            FileUtils.createArchiveOfDirectory(logsDir.toPath(), logsTarFile);

                            // Try 3 times
                            int attempt = 0;
                            while (attempt < 3) {
                                try {
                                    logger.debug("Uploading {}", logsTarFile);
                                    if (FileServerUtils.upload(logsTarFile.toFile(), config)) {
                                        break;
                                    }
                                } catch (IOException e) {
                                    logger.warn("Unable to upload log files", e);
                                }

                                try {
                                    attempt += 1;
                                    if (attempt >= 3) {
                                        logger.error(
                                                "Failed to upload log files, too many failed"
                                                        + " attempts");
                                        return;
                                    }
                                    Thread.sleep(1000);
                                } catch (InterruptedException e) {
                                    logger.error("Interrupted, log backup failed");
                                    return;
                                }
                            }

                            logsTarFile.toFile().delete();
                        }
                        if (delete) {
                            logger.info("Deleting files in " + logsDir.getAbsolutePath());
                            FileUtils.deleteFilesInDirectory(logsDir);
                        }
                    } catch (Exception e) {
                        logger.error(
                                "Unable to rotate logs: "
                                        + e.getClass().getName()
                                        + ": "
                                        + e.getMessage());
                    }
                });
    }

    @Override
    public void pushRuntimeConfigs(File configsDir, String configName, String pathToKey) {
        executorService.submit(
                () -> {
                    try {
                        logger.info("Pushing runtime configs in " + configsDir.getAbsolutePath());

                        // Note that this file name MUST match what is defined in race-in-the-box.
                        // Definition can be found in the push_runtime_configs method of the
                        // RibDeployment class.
                        Path configsTar =
                                Paths.get(
                                        FileUtils.getTempDirectory(),
                                        "runtime-configs-"
                                                + configName
                                                + "-"
                                                + persona
                                                + ".tar.gz");
                        logger.debug("Creating temporary archive " + configsTar);

                        // Decrypt the log files to a temporary directory.
                        // Create a temporary directory.
                        Path tempDecryptDir = Paths.get(FileUtils.getTempDirectory(), configName);
                        FileEncryptionUtils.decryptDirectory(
                                configsDir.toPath(), tempDecryptDir, pathToKey, true);

                        FileUtils.createArchiveOfDirectory(tempDecryptDir, configsTar);

                        int attempt = 0;
                        final int MAX_ATTEMPTS = 3;
                        while (attempt < MAX_ATTEMPTS) {
                            try {
                                logger.debug("Uploading {}", configsTar);
                                if (FileServerUtils.upload(configsTar.toFile(), this.config)) {
                                    break;
                                }
                            } catch (IOException e) {
                                logger.warn("Unable to upload runtime configs", e);
                            }

                            try {
                                attempt += 1;
                                if (attempt >= 3) {
                                    logger.error(
                                            "Failed to upload runtime configs, too many failed"
                                                    + " attempts");
                                    return;
                                }
                                Thread.sleep(1000);
                            } catch (InterruptedException e) {
                                logger.error("Interrupted, pushing runtime configs failed");
                                return;
                            }
                        }

                        configsTar.toFile().delete();

                    } catch (Exception e) {
                        logger.error(
                                "Unable to push runtime configs: "
                                        + e.getClass().getName()
                                        + ": "
                                        + e.getMessage());
                    }
                });
    }

    @Override
    public void fetchRemoteFile(
            URL url, File localFile, Consumer<Boolean> callback, Duration timeout) {
        final Instant startTime = Instant.now();
        final Runnable[] r = new Runnable[1];
        r[0] =
                () -> {
                    try {
                        URLConnection urlConnection = url.openConnection();
                        try (ReadableByteChannel urlChannel =
                                        Channels.newChannel(urlConnection.getInputStream());
                                FileOutputStream localFileStream =
                                        new FileOutputStream(localFile)) {
                            long expectedSize = urlConnection.getContentLengthLong();
                            logger.debug(
                                    "Fetching file of size " + expectedSize + " bytes from " + url);
                            long downloadedSize = 0l;
                            while (downloadedSize < expectedSize) {
                                downloadedSize +=
                                        localFileStream
                                                .getChannel()
                                                .transferFrom(
                                                        urlChannel, downloadedSize, Long.MAX_VALUE);
                                logger.trace("Downloaded " + downloadedSize + " bytes");
                            }
                        }
                        try {
                            callback.accept(true);
                        } catch (Exception e) {
                            logger.error("Error invoking callback", e);
                        }
                    } catch (Exception e) {
                        Instant currentTime = Instant.now();
                        if (Duration.between(startTime, currentTime).compareTo(timeout) >= 0) {
                            logger.error("Failed to download {}", url);
                            callback.accept(false);
                        } else {
                            logger.warn(
                                    "Unable to download {}: {}, {}",
                                    url,
                                    e.getClass().getName(),
                                    e.getMessage());
                            executorService.schedule(r[0], 1, TimeUnit.SECONDS);
                        }
                    }
                };
        executorService.submit(r[0]);
    }

    @Override
    public void fetchFromFileServer(
            String filename, File localFile, Consumer<Boolean> callback, Duration timeout) {
        try {
            URL url =
                    new URL(
                            "http://"
                                    + config.getFileServerHost()
                                    + ":"
                                    + config.getFileServerPort()
                                    + "/"
                                    + filename);
            fetchRemoteFile(url, localFile, callback, timeout);
        } catch (MalformedURLException e) {
            logger.error("Error creating file server URL: {}", e.getMessage());
            callback.accept(false);
        }
    }

    @Override
    public String getDeploymentName() {
        return (String) getDaemonStateInfo("deployment");
    }

    @Override
    public boolean isDnsSuccessful() {
        try {
            InetAddress.getByName(config.getFileServerHost());
            return true;
        } catch (UnknownHostException err) {
            return false;
        }
    }

    @Override
    public boolean getIsGenesis() {
        return config.getIsGenesis();
    }

    @Override
    public boolean saveDaemonStateInfo(String key, Object value) {
        try {
            File stateFile = new File(config.getDaemonStateInfoJsonPath());
            JSONObject existingState = new JSONObject();

            if (stateFile.exists()) {
                String stateFileContents =
                        new String(Files.readAllBytes(stateFile.toPath())).trim();
                JSONTokener tokener = new JSONTokener(stateFileContents);
                existingState = new JSONObject(tokener);
            }

            existingState.put(key, value);
            String existingStateString = existingState.toString();

            Files.write(stateFile.toPath(), existingStateString.getBytes());

        } catch (Exception e) {
            return false;
        }
        return true;
    }

    @Override
    public Object getDaemonStateInfo(String key) {
        File stateFile = new File(config.getDaemonStateInfoJsonPath());
        if (!stateFile.exists()) {
            logger.error("daemon state info file not found");
            return "";
        }
        try {
            String stateFileContents = new String(Files.readAllBytes(stateFile.toPath())).trim();
            JSONTokener tokener = new JSONTokener(stateFileContents);
            JSONObject existingState = new JSONObject(tokener);
            return existingState.get(key);
        } catch (Exception e) {
            logger.error("Error getting daemon state info: {}", e.getMessage());
            return "";
        }
    }
}
