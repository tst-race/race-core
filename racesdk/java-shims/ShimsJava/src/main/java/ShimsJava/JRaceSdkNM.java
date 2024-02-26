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

// For API documentation please see the equivalent C++ header:
// racesdk/common/include/IRaceSdkNM.h

package ShimsJava;

import java.util.HashMap;

/** Interface to the RACE SDK for network manager plugins. */
public class JRaceSdkNM {
    private static String logLabel = "JRaceSdkNM";

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

    public JRaceSdkNM(long sdkPointer) {
        _jni_initialize(sdkPointer);
    }

    private native void _jni_initialize(long sdkPointer);

    /**
     * Retrieve the timeout constant value to indicate that an SDK operation should block.
     *
     * @return Blocking timeout value
     */
    public static native int getBlockingTimeout();

    /**
     * Retrieve the timeout constant value to indicate that a connection should never drop packed
     * due to timeout
     *
     * @return Unlimted timeout value
     */
    public static native int getUnlimitedTimeout();

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
     * Pass an EncPkg to a comms channel via the SDK to send out.
     *
     * @param ePkg The EncPkg to send
     * @param connectionId The ConnectionID for the connection to use to send it out - will place
     *     the send call on a queue for that particular connection
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse sendEncryptedPackage(
            JEncPkg ePkg, String connectionId, long batchId, int timeout);

    /**
     * Pass a ClrMsg to the client or server RACE app (likely for presentation to the user).
     *
     * @param msg The ClrMsg to present
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse presentCleartextMessage(JClrMsg msg);

    /**
     * Open a connection of a given type on the given link.
     *
     * <p>Additional link-specific options can be specified in the linkHints string as JSON. The
     * ConnID of the new connection will be provided in the onConnectionStatusChanged call with the
     * handle matching the handle returned in this SdkResponse object.
     *
     * @param linkType The link type: send, receive, or bi-directional
     * @param linkId The ID of the link to open the connection on
     * @param linkHints Additional optional configuration information provided by network manager as
     *     a stringified JSON Object. May be ignored/honored by the comms plugin.
     * @param priority The priority of this call, higher is more important
     * @param sendTimeout If a package sent on this link takes songer than this many seconds,
     *     generate a package failed callback.
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse openConnection(
            LinkType linkType,
            String linkId,
            String linkHints,
            int priority,
            int sendTimeout,
            int timeout);

    /**
     * Close a connection with a given ID.
     *
     * @param connectionId The ID of the connection to close.
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse closeConnection(String connectionId, int timeout);

    /**
     * Get all of the links that connect to a set of personas.
     *
     * @param recipientPersonas The personas that the links can connect to. All personas must be
     *     reachable.
     * @param linkType The type of links to get: send, receive, or bi-directional
     * @return Array of link IDs that can connect to the given persona
     */
    public native String[] getLinksForPersonas(String[] recipientPersonas, LinkType linkType);

    /**
     * Get the links for a given channel.
     *
     * @param channelGid The ID of the channel to get links for
     * @return std::vector<LinkID> A vector of the links for the given channel
     */
    public native String[] getLinksForChannel(String channelGid);

    /**
     * Request properties of a link with a given type and ID.
     *
     * <p>This data will be queried from the internal cache held by core.
     *
     * @param linkId The ID of the link
     * @return LinkProperties The properties of the requested link
     */
    public native JLinkProperties getLinkProperties(String linkId);

    /**
     * Get all of personas associated with this link
     *
     * @param linkId The ID of the link
     * @return Array of link IDs that can connect to the given persona
     */
    public native String[] getPersonasForLink(String linkId);

    /**
     * Change the list of personas associated with a link
     *
     * @param linkId The ID of the link
     * @param personas The list of personas to associate with the link
     * @return SdkResponse The state of the SDK in response to this call
     */
    public native SdkResponse setPersonasForLink(String linkId, String[] personas);

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
     * The list of channels supported and their properties
     *
     * <p>This data will be queried from the internal cache held by core.
     *
     * @return Map of ChannelGID to ChannelProperties
     */
    public native HashMap<String, JChannelProperties> getSupportedChannels();

    /**
     * Deactivate a new channel
     *
     * @param channelId The global ID of the channel to deactivate
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse deactivateChannel(String channelId, int timeout);

    /**
     * Activate a new channel
     *
     * @param channelId The global ID of the channel to activate
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse activateChannel(String channelId, String role, int timeout);

    /**
     * Destroy a new link
     *
     * @param linkId The LinkID of the link to destroy
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse destroyLink(String linkId, int timeout);

    /**
     * Create a new link of the specified channel and associate with a set of personas
     *
     * @param channelGid The global ID of the channel to create the link
     * @param personas The list of personas to associate with the link
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse createLink(String channelGid, String[] personas, int timeout);

    /**
     * @brief create link from address specified by genensis configs
     * @param channelGid The name of the channel to create a new link for
     * @param linkAddress The LinkAddress used to create this link
     * @param personas The list of personas to associate this link with
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     *     RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     *     arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse createLinkFromAddress(
            String channelGid, String linkAddress, String[] personas, int timeout);

    /**
     * Load a link address of the specified channel and associate with a set of personas
     *
     * @param channelGid The global ID of the channel to create the link
     * @param linkAddress The address to load for instantiating the link
     * @param personas The list of personas to associate with the link
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse loadLinkAddress(
            String channelGid, String linkAddress, String[] personas, int timeout);

    /**
     * Load a list of link addresses of the specified channel and associate with a set of personas
     *
     * <p>Only some channels will support this API, most only support loadLinkAddress()
     *
     * @param channelGid The global ID of the channel to create the link
     * @param linkAddresses The addresses to load for instantiating the link
     * @param personas The list of personas to associate with the link
     * @param timeout Timeout in milliseconds to block, 0 indicates a non-blocking call, negative
     *     values are invalid arguments
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse loadLinkAddresses(
            String channelGid, String[] linkAddresses, String[] personas, int timeout);

    /**
     * @brief Bootstrap a node with the specified configs
     * @param handle The handle passed to network manager in prepareToBootstrap
     * @param commsChannels the list of comms plugins to install in the bootstrapp
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse bootstrapDevice(RaceHandle handle, String[] commsChannels);

    /**
     * @brief Inform the sdk that a bootstrap failed
     * @param handle The handle passed to network manager in prepareToBootstrap
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse bootstrapFailed(RaceHandle handle);

    /**
     * @brief Send a bootstrap package containing the persona and package used for enrollment of
     *     this node over the specified connection
     * @param connectionId The ConnectionId of the connect to send the package over
     * @param persona The persona of this node
     * @param pkg The pkg used by the bootstrapping node to enroll this node
     * @return SdkResponse the status of the SDK in response to this call.
     */
    public native SdkResponse sendBootstrapPkg(
            String connectionId, String persona, byte[] pkg, int timeout);

    /**
     * @brief Notify the Race App of status change (e.g. when it is ready to send client messages)
     * @param pluginStatus The status of the plugin
     * @return SdkResponse the status of the SDK in response to this call
     */
    public native SdkResponse onPluginStatusChanged(PluginStatus pluginStatus);

    /**
     * @brief Callback for update the SDK on clear message status.
     * @param handle The handle passed into processClrMsg when the network manager plugin initally
     *     received the relevant clear message.
     * @param status The new status of the message.
     * @return SdkResponse the status of the SDK in response to this call.
     */
    public native SdkResponse onMessageStatusChanged(RaceHandle handle, MessageStatus status);

    /**
     * @brief Displays information to the User
     *     <p>The task posted to the work queue will display information to the user input prompt,
     *     wait an optional amount of time, then notify the SDK of the user acknowledgment.
     * @param data data to display
     * @param displayType type of user display to display data in
     * @return SdkResponse object that contains whether the post was successful
     */
    public native SdkResponse displayInfoToUser(String data, UserDisplayType displayType);
}
