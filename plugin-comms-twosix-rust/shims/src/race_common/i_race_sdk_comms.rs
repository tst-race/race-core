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

use super::channel_properties::ChannelProperties;
use super::channel_status::ChannelStatus;
use super::connection_status::ConnectionStatus;
///
/// Rust equivalent of the C++ IRaceSdkComms class.
/// For reference, the C++ source can be found here: racesdk/common/include/IRaceSdkComms.h
///
use super::link_properties::LinkProperties;
use super::link_status::LinkStatus;
use super::package_status::PackageStatus;
use super::plugin_response::PluginResponse;
use super::race_enums::BootstrapActionType;
use super::race_enums::UserDisplayType;
use super::sdk_response::SdkResponse;

// This constant is may be used to specify that a function taking a timeout should never time out.
pub const RACE_BLOCKING: i32 = i32::MIN;

/// Trait that defines the SDK functions available to a comms plugin.
///
/// All comms plugin implementations will use this to interface with the SDK.
pub trait IRaceSdkComms {
    /// Queries the SDK for entropy from both system calls as well as external
    /// sources.
    ///
    /// # Arguments
    ///
    /// * `num_bytes` - The number of bytes to generate
    ///
    /// # Return Value
    ///
    /// * `Vec<u8>` - Random bytes of length num_bytes
    fn get_entropy(&self, num_bytes: u32) -> Vec<u8>;

    /// Returns the String ID of the active persona for the RACE system.
    ///
    /// # Arguments
    ///
    /// * None
    ///
    /// # Return Value
    ///
    /// * `String` - The name of the active persona
    fn get_active_persona(&self) -> String;

    /// Notify the SDK of an error that occured in an asynchronous call
    ///
    /// # Arguments
    ///
    /// * `race_handle` - The handle associated with the async call
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn async_error(&self, race_handle: u64, plugin_response: PluginResponse) -> SdkResponse;

    /// Get the ChannelProperties for a particular channel
    ///
    /// # Arguments
    ///
    /// * `channelGid` - The name of the channel
    ///
    /// # Return Value
    ///
    /// * `ChannelProperties` - The properties for the channel
    fn get_channel_properties(&self, channel_gid: &str) -> ChannelProperties;

    // Notify network manager via the SDK that the status of a package has changed.
    ///
    /// # Arguments
    ///
    /// * `handle` - The RaceHandle identifying the sendPackage call it corresponds to
    /// * `status` - The new ConnectionStatus
    /// * `timeout` - Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
    ///     RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
    ///     arguments
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn on_package_status_changed(
        &self,
        handle: u64,
        status: PackageStatus,
        timeout: i32,
    ) -> SdkResponse;

    /// Notify network manager via the SDK that the stauts of a connection has changed
    ///
    /// # Arguments
    ///
    /// * `handle` - The RaceHandle identifying the network manager call it corresponds to
    /// * `connection_id` - The connection that as changed
    /// * `status` - The new ConnectionStatus
    /// * `properties` - The properties of the changed connection
    /// * `timeout` - Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
    ///     RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
    ///     arguments
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn on_connection_status_changed(
        &self,
        handle: u64,
        connection_id: &str,
        status: ConnectionStatus,
        properties: &LinkProperties,
        timeout: i32,
    ) -> SdkResponse;

    fn on_channel_status_changed(
        &self,
        handle: u64,
        channel_gid: &str,
        status: ChannelStatus,
        properties: &ChannelProperties,
        timeout: i32,
    ) -> SdkResponse;

    fn on_link_status_changed(
        &self,
        handle: u64,
        link_id: &str,
        status: LinkStatus,
        properties: &LinkProperties,
        timeout: i32,
    ) -> SdkResponse;

    /// Notify the SDK and network manager of a change in LinkProperties
    ///
    /// # Arguments
    ///
    /// * `link_id` - The LinkID identifying the link with updated properties
    /// * `properties` - The new LinkProperties
    /// * `timeout` - Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
    ///     RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
    ///     arguments
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn update_link_properties(
        &self,
        link_id: &str,
        properties: &LinkProperties,
        timeout: i32,
    ) -> SdkResponse;

    /// Request the SDK to create a new ConnectionID for the plugin
    ///
    /// # Arguments
    ///
    /// * `link_id` - The link to generate the connection id for
    ///
    /// # Return Value
    ///
    /// * `String` - The generated connection id
    fn generate_connection_id(&self, link_id: &str) -> String;

    // Request the SDK to create a new LinkID for the plugin
    ///
    /// # Arguments
    ///
    /// * `channel_gid` - The channel to create a link ID for
    ///
    /// # Return Value
    ///
    /// * `String` - The generated link id
    fn generate_link_id(&self, channel_gid: &str) -> String;

    /// Notify network manager via the SDK of a new EncPkg that was received
    ///
    /// # Arguments
    ///
    /// * `raw_package` - The EncPkg that was received
    /// * `connection_ids` - The Vector of ConnectionIDs the pkg was received on
    /// * `timeout` - Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
    ///     RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
    ///     arguments
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn receive_enc_pkg(
        &self,
        raw_package: &[u8],
        connection_ids: &[&str],
        timeout: i32,
    ) -> SdkResponse;

    /// Make a directory in this plugin's storage
    ///
    /// # Arguments
    ///
    /// * `dirpath` - The path of the directory
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn make_dir(&self, dirpath: &str) -> SdkResponse;

    /// Remove a file or directory from this plugin's storage
    ///
    /// # Arguments
    ///
    /// * `dirpath` - The path of the directory
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn remove_dir(&self, dirpath: &str) -> SdkResponse;

    /// List contents of a plugin storage directory
    ///
    /// # Arguments
    ///
    /// * `dirpath` - The path of the directory
    ///
    /// # Return Value
    ///
    /// * `Vec<&str>` - vector of filenames (empty vector if no directory or it is empty)
    fn list_dir(&self, dirpath: &str) -> Vec<String>;

    /// Read a file from this plugin's storage
    ///
    /// # Arguments
    ///
    /// * `filename` - The name of the file
    ///
    /// # Return Value
    ///
    /// * `unsigned byte` - the data of the file (empty-string on error or empty file)
    fn read_file(&self, filename: &str) -> Vec<u8>;

    /// Write a file to this plugin's storage area, overwriting if it exists.
    ///
    /// # Arguments
    ///
    /// * `filename` - The name of the file
    /// * `data` - The data to write to the file
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn write_file(&self, filename: &str, data: Vec<u8>) -> SdkResponse;

    /// Write a file to this plugin's storage area, appending if it exists.
    ///
    /// # Arguments
    ///
    /// * `filename` - The name of the file
    /// * `data` - The data to write to the file
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn append_file(&self, filename: &str, data: Vec<u8>) -> SdkResponse;

    /// Request plugin-specific input from the user with the specified prompt message.
    /// The response may be cached in persistent storage, in which case the user will not be
    /// re-prompted if a cached response exists for the given prompt.
    ///
    /// The response will be provided in the userInputReceived callback with the handle matching
    /// the handle returned in this SdkResponse object.
    ///
    /// # Arguments
    ///
    /// * `key` - Prompt identifier for the user input request
    /// * `prompt` - Message to be presented to the user
    /// * `cache` - If true, the response will be cached in persistent storage
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - indicator of success or failure of the request
    fn request_plugin_user_input(&self, key: &str, prompt: &str, cache: bool) -> SdkResponse;

    /// Request application-wide input from the user associated with the given key.
    /// The key identifies a common user input prompt and must be a key supported by the
    /// RACE SDK. The response is cached in persistent storage, so the user will not be
    /// re-prompted if a cached response exists for the given key.
    ///
    /// The response will be provided in the userInputReceived callback with the handle matching
    /// the handle returned in this SdkResponse object.
    ///
    /// # Arguments
    ///
    /// * `key` - Prompt identifier for the application-wide user input request
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - indicator of success or failure of the request
    fn request_common_user_input(&self, key: &str) -> SdkResponse;

    /// Displays information to the User
    ///
    /// Notification of the user receiving the notification will be sent in
    /// on_user_acknowledgement_received
    ///
    /// # Arguments
    ///
    /// * `data` - Data to display
    /// * `user_display_type` - Type of user display to display data in
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - indicator of success or failure of the request
    fn display_info_to_user(&self, data: &str, user_display_type: UserDisplayType) -> SdkResponse;

    /// Displays information to the User and forward information to target node for
    /// automated testing
    ///
    /// Notification of the user receiving the notification will be sent in
    /// on_user_acknowledgement_received
    ///
    /// # Arguments
    ///
    /// * `data` - Data to display
    /// * `user_display_type` - Type of user display to display data in
    /// * `action_type` - Type of action the Daemon must take
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - indicator of success or failure of the request
    fn display_bootstrap_info_to_user(
        &self,
        data: &str,
        user_display_type: UserDisplayType,
        action_type: BootstrapActionType,
    ) -> SdkResponse;

    /// Unblock the queue for a connection previously blocked by a return value of
    /// PLUGIN_TEMP_ERROR from sendPackage.
    ///
    /// # Arguments
    ///
    /// * `connId` - The connection to unblock
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - indicator of success or failure of the request
    fn unblock_queue(&self, connection_id: &str) -> SdkResponse;
}
