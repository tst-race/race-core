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

package ShimsJava;

/** Interface to the RACE SDK for comms plugins. */
public class JRaceSdkComms {
    private static String logLabel = "JRaceSdkComms";

    private final long sdkPointer;

    // Load the C++ .so
    static {
        try {
            // "lib" is automatically prepended
            // and ".so" is automatically appended
            System.loadLibrary("RaceJavaShims");
        } catch (Error e) {
            RaceLog.logError(logLabel, "exception loading lib: " + e.getMessage(), "");
        }
    }

    public JRaceSdkComms(long sdkPointer, String pluginName) {
        this.sdkPointer = sdkPointer;
        _jni_initialize(sdkPointer, pluginName);
    }

    private native void _jni_initialize(long sdkPointer, String pluginName);

    /**
     * Retrieve the timeout constant value to indicate that an SDK operation should block.
     *
     * @return Blocking timeout value
     */
    public static native int getBlockingTimeout();

    /**
     * Query the system for entropy.
     *
     * <p>Entropy will be gathered from system calls as well as external sources (e.g., user
     * scribbling on screen or reading from a device's microphone).
     *
     * @param numBytes Number of bytes of entropy to be requested
     * @return Bytes of entropy
     */
    public native byte[] getEntropy(int numBytes);

    /**
     * Get the active persona for the RACE system.
     *
     * @return The active persona
     */
    public native String getActivePersona();

    /**
     * Request properties of a channel with a given GID.
     *
     * <p>This data will be queried from the internal cache held by core.
     *
     * @param channelGid The GID of the channel
     * @return ChannelProperties The properties of the requested channel
     */
    public native JChannelProperties getChannelProperties(String channelGid);

    /**
     * Request properties of all channels
     *
     * <p>This data will be queried from the internal cache held by core.
     *
     * @return ChannelProperties The properties of all the channels
     */
    public native JChannelProperties[] getAllChannelProperties();

    /**
     * Create the directory of directoryPath, including any directories in the path that do not yet
     * exist
     *
     * @param directoryPath the path of the directory to create.
     * @return SdkResponse indicator of success or failure of the create
     */
    public native SdkResponse makeDir(String directoryPath);

    /**
     * Recurively remove the directory of directoryPath
     *
     * @param directoryPath the path of the directory to remove.
     * @return SdkResponse indicator of success or failure of the removal
     */
    public native SdkResponse removeDir(String directoryPath);

    /**
     * List the contents (directories and files) of the directory path
     *
     * @param directoryPath the path of the directory to list.
     * @return string array list of directories and files
     */
    public native String[] listDir(String directoryPath);

    /**
     * Read the contents of a file in this plugin's storage.
     *
     * @param filename The string name of the file to be read.
     * @return byte array of the file contents
     */
    public native byte[] readFile(String filename);

    /**
     * Append the contents of data to filename in this plugin's storage.
     *
     * @param filename The string name of the file to be appended to (or written).
     * @param data The string of data to append to the file.
     * @return SdkResponse indicator of success or failure of the append.
     */
    public native SdkResponse appendFile(String filename, byte[] data);

    /**
     * @brief Write the contents of data to filename in this plugin's storage (overwriting if file
     *     exists)
     * @param filename The string name of the file to be written.
     * @param data The string of data to write to the file.
     * @return SdkResponse indicator of success or failure of the write.
     */
    public native SdkResponse writeFile(String filename, byte[] data);

    /**
     * Request plugin-specific input from the user with the specified prompt message. The response
     * may be cached in persistent storage, in which case the user will not be re-prompted if a
     * cached response exists for the given prompt.
     *
     * <p>The response will be provided in the userInputReceived callback with the handle matching
     * the handle returned in this SdkResponse object.
     *
     * @param key Prompt identifier for the user input request
     * @param prompt Message to be presented to the user
     * @param cache If true, the response will be cached in persistent storage
     * @return SdkResponse indicator of success or failure of the request
     */
    public native SdkResponse requestPluginUserInput(String key, String prompt, boolean cache);

    /**
     * Request application-wide input from the user associated with the given key. The key
     * identifies a common user input prompt and must be a key supported by the RACE SDK. The
     * response is cached in persistent storage, so the user will not be re-prompted if a cached
     * response exists for the given key.
     *
     * <p>The response will be provided in the userInputReceived callback with the handle matching
     * the handle returned in this SdkResponse object.
     *
     * @param key Prompt identifier for the application-wide user input request
     * @return SdkResponse indicator of success or failure of the request
     */
    public native SdkResponse requestCommonUserInput(String key);

    /**
     * Notify network manager via the SDK that the status of a package has changed.
     *
     * @param handle The RaceHandle identifying the sendEncryptedPackage call it corresponds to
     * @param status The new PackageStatus
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse onPackageStatusChanged(
            RaceHandle handle, PackageStatus status, int timeout);

    /**
     * Notify network manager via the SDK that the status of a connection has changed.
     *
     * @param handle The RaceHandle identifying the network manager call it corresponds to
     * @param connId The connection ID for the connection that has changed
     * @param status The new ConnectionStatus
     * @param properties Link properties of the connection
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse onConnectionStatusChanged(
            RaceHandle handle,
            String connId,
            ConnectionStatus status,
            JLinkProperties properties,
            int timeout);

    /**
     * Notify network manager via the SDK that the status of a link has changed.
     *
     * @param handle The RaceHandle identifying the network manager call it corresponds to
     * @param linkId The name for the link that has changed
     * @param status The new LinkStatus
     * @param properties LinkProperties of the link
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse onLinkStatusChanged(
            RaceHandle handle,
            String linkId,
            LinkStatus status,
            JLinkProperties properties,
            int timeout);

    /**
     * Notify network manager via the SDK that the status of a channel has changed.
     *
     * @param handle The RaceHandle identifying the network manager call it corresponds to
     * @param channelGid The name for the channel that has changed
     * @param status The new ChannelStatus
     * @param properties ChannelProperties of the channel
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse onChannelStatusChanged(
            RaceHandle handle,
            String channelGid,
            ChannelStatus status,
            JChannelProperties properties,
            int timeout);

    /**
     * Notify the SDK and network manager of a change in LinkProperties.
     *
     * @param linkId The LinkID identifying the link with updated properties
     * @param linkProperties The new LinkProperties
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse updateLinkProperties(
            String linkId, JLinkProperties linkProperties, int timeout);

    /**
     * Request the SDK to create a new ConnectionID for the plugin
     *
     * @param linkId The LinkID for the link the connection will be on
     * @return ConnectionID the ID for the connection provided by the SDK
     */
    public native String generateConnectionId(String linkId);

    /**
     * Request the SDK to create a new LinkID for the plugin
     *
     * @param channelGid The name of the channel this link instantiates
     * @return LinkID the ID for the link provided by the SDK
     */
    public native String generateLinkId(String channelGid);

    /**
     * Notify network manager via the SDK of a new EncPkg that was received
     *
     * @param pkg The EncPkg that was received
     * @param connectionIds collection of ConnectionIDs the package was received on (since comms
     *     channels can re-use connections with multiple connection IDs)
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse receiveEncPkg(JEncPkg pkg, String[] connectionIds, int timeout);

    /**
     * @brief Displays information to the User
     *     <p>The task posted to the work queue will display information to the user input prompt,
     *     wait an optional amount of time, then notify the SDK of the user acknowledgment.
     * @param data data to display
     * @param displayType type of user display to display data in
     * @return SdkResponse object that contains whether the post was successful
     */
    public native SdkResponse displayInfoToUser(String data, UserDisplayType displayType);

    /**
     * @brief Displays information to the User
     *     <p>The task posted to the work queue will display information to the user input prompt,
     *     wait an optional amount of time, then notify the SDK of the user acknowledgment.
     * @param data data to display
     * @param displayType type of user display to display data in
     * @param actionType type of action the Daemon must take
     * @return SdkResponse object that contains whether the post was successful
     */
    public native SdkResponse displayBootstrapInfoToUser(
            String data, UserDisplayType displayType, BootstrapActionType actionType);

    /**
     * @brief unblock a connection queue previously blocked by returning PLUGIN_TEMP_ERROR from send
     *     package
     * @param connId connection to unblock
     * @return SdkResponse object that contains whether the unblock was successful
     */
    public native SdkResponse unblockQueue(String connId);
}
