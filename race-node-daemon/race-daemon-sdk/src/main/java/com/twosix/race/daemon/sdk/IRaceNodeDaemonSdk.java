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

import java.io.File;
import java.net.URL;
import java.time.Duration;
import java.util.function.Consumer;

/** Daemon SDK interface. */
public interface IRaceNodeDaemonSdk {
    /**
     * Gets the value for BASE_APP_IS_ALIVE_KEY from redis for internal use by the daemon i.e.
     * checking if the app is alive before attempting to start
     *
     * @return boolean Returns true if the app is running, false otherwise
     */
    public boolean isAppAlive();

    /**
     * Reports application status to RiB.
     *
     * @param jsonStatus App status JSON string
     * @param expirationSeconds Time before status report is invalidated if not updated
     */
    public void updateAppStatus(String jsonStatus, long expirationSeconds);

    /**
     * Reports node status to RiB.
     *
     * @param jsonStatus Node status JSON string
     * @param expirationSeconds Time before status report is invalidated if not updated
     */
    public void updateNodeStatus(String jsonStatus, long expirationSeconds);

    /**
     * Sends bootstrap information to the bootstrap target.
     *
     * @param target Target of the bootstrap
     * @param message Information message
     * @param actionType Action to perform using the information
     */
    public void sendBootstrapInfo(String target, String message, String actionType);

    /**
     * Registers a listener to receive orchestration action notifications.
     *
     * @param listener Action listener
     */
    public void registerActionListener(IRaceNodeActionListener listener);

    /** Checks if the Action Listener is Registered */
    public boolean isActionListenerRegistered();

    /**
     * Rotates the application logs.
     *
     * <p>If a backup ID is provided, the logs will be uploaded off of the node. If deletion is
     * enabled, all files in the logs directory will be deleted.
     *
     * @param logsDir Location of the application's logs directory
     * @param delete Enable deletion of log files
     * @param backupId Identifier to associate with backed up log files, if empty no backup is made
     */
    public void rotateLogs(File logsDir, boolean delete, String backupId);

    /**
     * Pushed the current runtime configs from the deployment to the file server.
     *
     * @param configsDir Location of the application's runtime configs directory.
     * @param configName Name to give the saved runtime configs.
     * @param pathToKey Path to the key used for encryption, e.g. "/etc/race/file_key" on Linux.
     */
    public void pushRuntimeConfigs(File configsDir, String configName, String pathToKey);

    /**
     * Fetches a file from the specified URL, retrying until the specified timeout is reached.
     *
     * @param url Remote file URL
     * @param localFile Path to place the file
     * @param callback Callback to be invoked with the success result of the operation
     * @param timeout Amount of time to retry before failing
     */
    public void fetchRemoteFile(
            URL url, File localFile, Consumer<Boolean> callback, Duration timeout);

    /**
     * Fetches a file from the central file server as provided by RiB.
     *
     * @param filename Name of the file
     * @param localFile Path to place the file
     * @param callback Callback to be invoked with the success result of the operation
     * @param timeout Amount of time to retry before failing
     */
    public void fetchFromFileServer(
            String filename, File localFile, Consumer<Boolean> callback, Duration timeout);

    /**
     * Gets the name of the deployment from which the RACE configs were obtained.
     *
     * @return Name of the associated deployment, or an empty string if not able to be determined
     */
    public String getDeploymentName();

    /**
     * Checks if DNS lookups for RiB services are succeeding on this node.
     *
     * @return True if DNS lookups are successful
     */
    public boolean isDnsSuccessful();

    /** Checks if the node is a Genesis node */
    public boolean getIsGenesis();

    /** Saves Daemon State info to a json file on disk */
    public boolean saveDaemonStateInfo(String key, Object value);

    /** Gets Daemon State info from a json file on disk */
    public Object getDaemonStateInfo(String key);
}
