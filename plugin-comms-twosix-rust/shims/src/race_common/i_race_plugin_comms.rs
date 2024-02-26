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

///
/// Rust equivalent of the C++ IRacePluginComms class.
/// For reference, the C++ source can be found here: racesdk/common/include/IRacePluginComms.h
///
use super::link_properties::LinkType;
use super::plugin_config::PluginConfig;
use super::plugin_response::PluginResponse;

/// Trait that defines a comms plugin.
///
/// All Comms Rust plugins must implement this trait.
pub trait IRacePluginComms {
    /// Intialize the plugin here. After this call, the plugin may receive other calls.
    ///
    /// # Arguments
    ///
    /// * `pluginConfig` - Config object containing dynamic config variables (e.g. paths)
    ///
    /// # Return Value
    ///
    /// * `PluginResponse` - the status of the Plugin in response to this call
    fn init(&mut self, plugin_config: &PluginConfig) -> PluginResponse;

    /// Shutdown the plugin. Close open connections, remove state, etc.
    ///
    /// # Arguments
    ///
    /// * None
    ///
    /// # Return Value
    ///
    /// * `PluginResponse` - the status of the Plugin in response to this call
    fn shutdown(&mut self) -> PluginResponse;

    /// Sends a package over an open connection specified by the connection ID.
    ///
    /// # Arguments
    ///
    /// * `handle` - The RaceHandle to use for updating package status in onPackageStatusChanged
    /// * `connectionId` - The ID of the connection to use to send the package.
    /// * `pkg` - The encrypted package to send.
    ///
    /// # Return Value
    ///
    /// * `PluginResponse` - the status of the Plugin in response to this call
    fn send_package(
        &mut self,
        handle: u64,
        connection_id: &str,
        pkg: &[u8],
        timeout_timestamp: f64,
        batch_id: u64,
    ) -> PluginResponse;

    /// Open a connection with a given type on the specified link. Additional configuration info can
    /// be provided via the linkHints param.
    ///
    /// # Arguments
    ///
    /// * `handle` - The RaceHandle to use for onConnectionStatusChanged calls
    /// * `link_type` - The ID of the connection to use to send the package.
    /// * `link_id` - The encrypted package to send.
    /// * `link_hints` - Additional optional configuration information provided by network manager as a stringified JSON Object.
    ///
    /// # Return Value
    ///
    /// * `PluginResponse` - the status of the Plugin in response to this call
    fn open_connection(
        &mut self,
        handle: u64,
        link_type: LinkType,
        link_id: &str,
        link_hints: &str,
        send_timeout: i32,
    ) -> PluginResponse;

    fn destroy_link(&mut self, handle: u64, link_id: &str) -> PluginResponse;

    fn create_link(&mut self, handle: u64, channel_gid: &str) -> PluginResponse;

    fn create_link_from_address(
        &mut self,
        handle: u64,
        channel_gid: &str,
        link_address: &str,
    ) -> PluginResponse;

    fn load_link_address(
        &mut self,
        handle: u64,
        channel_gid: &str,
        link_address: &str,
    ) -> PluginResponse;

    fn load_link_addresses(
        &mut self,
        handle: u64,
        channel_gid: &str,
        link_addresses: &[&str],
    ) -> PluginResponse;

    fn activate_channel(
        &mut self,
        handle: u64,
        channel_gid: &str,
        role_name: &str,
    ) -> PluginResponse;

    fn deactivate_channel(&mut self, handle: u64, channel_gid: &str) -> PluginResponse;

    /// Close a connection with a given ID.
    ///
    /// # Arguments
    ///
    /// * `handle` - The RaceHandle to use for onConnectionStatusChanged calls
    /// * `connectionId` - The ID of the connection to close.
    ///
    /// # Return Value
    ///
    /// * `PluginResponse` - the status of the Plugin in response to this call
    fn close_connection(&mut self, handle: u64, connection_id: &str) -> PluginResponse;

    /// Notify comms about received user input response
    ///
    /// # Arguments
    ///
    /// * `handle` - The handle for this callback
    /// * `answered` - True if the response contains an actual answer to the input prompt, otherwise
    ///     the response is an empty string and not valid
    /// * `response` - The user response answer to the input prompt
    ///
    /// # Return Value
    ///
    /// * `PluginResponse` - the status of the Plugin in response to this call
    fn on_user_input_received(
        &mut self,
        handle: u64,
        answered: bool,
        response: &str,
    ) -> PluginResponse;

    fn plugin_flush_channel(
        &mut self,
        handle: u64,
        connection_id: &str,
        batch_id: u64,
    ) -> PluginResponse;

    fn plugin_on_user_acknowledgment_received(&mut self, handle: u64) -> PluginResponse;
}
