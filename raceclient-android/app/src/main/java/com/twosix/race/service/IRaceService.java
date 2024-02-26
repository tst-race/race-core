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

import ShimsJava.ChannelStatus;
import ShimsJava.JChannelProperties;
import ShimsJava.RaceHandle;

import java.util.List;

/** RACE service interface for UI */
public interface IRaceService {

    /** Stops the RACE service. */
    public void stopService();

    /**
     * Register the callback listener to receive real-time notifications from the RACE service.
     *
     * @param listener Service listener
     */
    public void addServiceListener(IServiceListener listener);

    /**
     * Unregister the callback listener from receiving real-time notifications from the RACE
     * service.
     *
     * @param listener Service listener
     */
    public void removeServiceListener(IServiceListener listener);

    /**
     * Checks if all required permissions have been granted.
     *
     * @return True if all required permissions have been granted
     */
    public boolean arePermissionsSufficient();

    /**
     * Get the list of required permissions.
     *
     * @return List of required permissions
     */
    public String[] getRequiredPermissions();

    /**
     * Checks if all asset resources have been extracted.
     *
     * @return True if all asset resources have been extracted
     */
    public boolean areAssetsExtracted();

    /**
     * Checks if plugins have been copied to the internal plugin location.
     *
     * @return True if plugins are present
     */
    public boolean hasPlugins();

    /**
     * Checks if configs have been copied to the internal configs location.
     *
     * @return True if configs are present
     */
    public boolean hasConfigs();

    /**
     * Checks if the user persona is defined.
     *
     * @return True if user persona is defined.
     */
    public boolean hasPersona();

    /**
     * Gets the user persona.
     *
     * @return User persona
     */
    public String getPersona();

    /**
     * Sets the user persona.
     *
     * @param persona User persona
     */
    public void setPersona(String persona);

    /**
     * Checks if the user has entered a passphrase for the encryption key.
     *
     * @return True if user has provided a passphrase.
     */
    public boolean hasPassphrase();

    /**
     * Sets the user's passphrase used for encryption.
     *
     * @param passphrase User passphrase.
     */
    public void setPassphrase(String passphrase);

    /**
     * Checks if initially-enabled channels are defined.
     *
     * @return True if initially-enabled channels are defined.
     */
    public boolean hasEnabledChannels();

    /**
     * Use initial set of enabled channels as specified in RACE config. This is only used for
     * headless testing.
     */
    public void useInitialEnabledChannels();

    /**
     * Use specified set of channels to initialize RACE.
     *
     * @param channels List of channel IDs to be enabled
     */
    public void useEnabledChannels(String[] channels);

    /**
     * Get list of channel properties, optionally filtered by status.
     *
     * @param status Optional status to filter channels, else all channels are returned if null
     * @return List of channel properties
     */
    public List<JChannelProperties> getChannels(ChannelStatus status);

    /**
     * Enables or disables the specified channel.
     *
     * @param channelId Channel ID
     * @param enabled True if enabled, false if disabled
     */
    public void setChannelEnabled(String channelId, boolean enabled);

    /**
     * Start RACE communications, if not already started.
     *
     * @return True if RACE communications have been started
     */
    public boolean startRaceCommunications();

    /**
     * Send the given message to the specified destination persona.
     *
     * @param message Plaintext message contents
     * @param recipient Destination persona
     */
    public void sendMessage(String message, String recipient);

    /**
     * Suspend notifications for received messages from the specified recipient. This is used while
     * the message list for that recipient is currently on screen.
     *
     * @param recipient Recipient persona or null to resume all notifications
     */
    public void suspendMessageNotificationsFor(String recipient);

    /**
     * Process the response to previously requested user input.
     *
     * @param handle RACE handle from the originating onUserInputRequested
     * @param answered True if user answered, false if user dismissed the input prompt
     * @param response User input response
     */
    public void processUserInputResponse(RaceHandle handle, boolean answered, String response);

    /**
     * Process the acknowledgement of a previously requested alert.
     *
     * @param handle RACE handle from the originating onDisplayAlert
     */
    public void acknowledgeAlert(RaceHandle handle);

    /**
     * Prepare to bootstrap a new device into the RACE network.
     *
     * @param platform Platform of the device to be bootstrapped
     * @param arch Architecture of the device to be bootstrapped
     * @param nodeType Client or server role for the device to be bootstrapped
     * @param passphrase Secret passphrase to use for bootstrapping
     * @param bootstrapChannelId ID of the channel to use to communicate with the bootstrapped
     *     device
     */
    public void prepareToBootstrap(
            String platform,
            String arch,
            String nodeType,
            String passphrase,
            String bootstrapChannelId);

    /**
     * Check if the user has set a passphrase.
     *
     * @return true if the user has set a passphrase, else false.
     */
    public boolean isPassphraseSet();

    /**
     * Check if the user has provided an invalid passphrase.
     *
     * @return true if the user has provided an invalid passphrase, else false.
     */
    public boolean receivedInvalidPassphrase();
}
