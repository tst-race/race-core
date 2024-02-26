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

extern crate shims;
use shims::race_common::channel_properties::ChannelProperties;
use shims::race_common::channel_status::ChannelStatus;
use shims::race_common::link_properties::LinkProperties;
use shims::race_common::link_properties::LinkType;
use shims::race_common::link_status::LinkStatus;

use shims::race_common::connection_status::ConnectionStatus;
use shims::race_common::package_status::PackageStatus;
use shims::race_common::plugin_config::PluginConfig;
use shims::race_common::plugin_response::PluginResponse;
use shims::race_common::race_enums::UserDisplayType::UdToast;
use shims::race_common::sdk_response::{SdkResponse, SdkStatus, NULL_RACE_HANDLE};

use shims::race_common::i_race_plugin_comms::IRacePluginComms;
use shims::race_common::i_race_sdk_comms::IRaceSdkComms;
use shims::race_common::i_race_sdk_comms::RACE_BLOCKING;

#[macro_use]
mod log;

mod configloader;

mod direct_link;

mod link;
use link::{Connection, Link};

mod link_profile_parser;

mod socketlistener;
mod socketsender;

mod whiteboard_client;
mod whiteboard_link;

mod channels;
use channels::{
    get_default_channel_properties_for_channel, get_default_link_properties_for_channel,
    DIRECT_CHANNEL_GID, INDIRECT_CHANNEL_GID,
};

use std::collections::HashMap;
use std::collections::HashSet;
use std::sync::{Arc, Mutex};

fn expect_sdk_ok(response: SdkResponse) -> bool {
    match response.status {
        SdkStatus::SdkOk => true,
        _ => {
            log_error!(
                "Received unexpected response from the sdk: {:?}",
                response.status
            );
            false
        }
    }
}

///
/// PluginCommsTwoSixRust is the TwoSix Implementation of a
/// Comms Plugin. Will be used as the stub implementation when
/// testing network manager or as a reference for implementing a Comms
/// Plugin written in Rust
///
pub struct PluginCommsTwoSixRust {
    sdk: Arc<Mutex<dyn IRaceSdkComms + Send>>,
    /// Connections, indexed by connection ID
    connections: HashMap<String, Connection>,
    /// Links, indexed by link ID
    links: HashMap<String, Arc<Mutex<dyn Link>>>,
    /// activePersona
    active_persona: String,
    // Map of channel GID keys to the current status of that channel.
    channel_statuses: HashMap<&'static str, ChannelStatus>,
    // The next available port for creating direct links.
    next_available_port: u32,
    // The next available hashtag for creating indirect links.
    next_available_hashtag: u32,
    // The hostname of the whiteboard used for indirect channels.
    whiteboard_hostname: String,
    // The port of the whiteboard used for indirect channels.
    whiteboard_port: u32,
    // The node hostname to use with direct links.
    hostname: String,
    // All user input request handles
    direct_channel_user_input_requests: HashSet<u64>,
    // Handle for hostname user input request
    request_hostname_handle: u64,
    // Handle for direct link start port user input request
    request_start_port_handle: u64,
}

impl PluginCommsTwoSixRust {
    pub fn new(_sdk: Arc<Mutex<dyn IRaceSdkComms + Send>>) -> PluginCommsTwoSixRust {
        PluginCommsTwoSixRust {
            sdk: _sdk,
            connections: HashMap::new(),
            links: HashMap::new(),
            active_persona: String::from(""),
            channel_statuses: [
                (DIRECT_CHANNEL_GID, ChannelStatus::ChannelUnavailable),
                (INDIRECT_CHANNEL_GID, ChannelStatus::ChannelUnavailable),
            ]
            .iter()
            .cloned()
            .collect(),
            next_available_port: 10000,
            next_available_hashtag: 0,
            whiteboard_hostname: "twosix-whiteboard".to_string(),
            whiteboard_port: 5000,
            hostname: "no-hostname-provided-by-user".to_string(),
            direct_channel_user_input_requests: HashSet::new(),
            request_hostname_handle: 0,
            request_start_port_handle: 0,
        }
    }
}

impl IRacePluginComms for PluginCommsTwoSixRust {
    fn init(&mut self, plugin_config: &PluginConfig) -> PluginResponse {
        log_info!("init called");
        log_debug!("    etc_directory = {}", plugin_config.etc_directory);
        log_debug!(
            "    logging_directory = {}",
            plugin_config.logging_directory
        );
        log_debug!(
            "    aux_data_directory = {}",
            plugin_config.aux_data_directory
        );
        log_debug!("    tmp_directory = {}", plugin_config.tmp_directory);
        log_debug!("    plugin_directory = {}", plugin_config.plugin_directory);

        let sdk = self.sdk.as_ref().lock().unwrap();
        self.active_persona = sdk.get_active_persona();
        log_debug!("    active persona: {}", self.active_persona);

        sdk.write_file(
            "initialized.txt",
            "Comms Rust Plugin Initialized\n".as_bytes().to_vec(),
        );
        log_debug!(
            "Read Initialization File: {:?}",
            std::str::from_utf8(&sdk.read_file("initialized.txt")[..])
        );

        sdk.make_dir("testdir");
        sdk.remove_dir("testdir");
        let parent_dir_to_list_contents = "/code/";
        let all_the_dirs = sdk.list_dir(parent_dir_to_list_contents);
        if all_the_dirs.len() > 0 {
            log_debug!("Files found in {}:", parent_dir_to_list_contents);
            for dir in all_the_dirs {
                log_debug!("    {:?}", dir);
            }
        } else {
            log_debug!("List Dir: nothing found in {}", parent_dir_to_list_contents);
        }

        // Set channels unavailable (internal status)
        self.channel_statuses
            .insert(DIRECT_CHANNEL_GID, ChannelStatus::ChannelUnavailable);
        self.channel_statuses
            .insert(INDIRECT_CHANNEL_GID, ChannelStatus::ChannelUnavailable);

        // Need to release the lock on the SDK
        drop(sdk);

        return PluginResponse::PluginOk;
    }

    fn shutdown(&mut self) -> PluginResponse {
        log_info!("shutdown: called");

        self.channel_statuses
            .insert(DIRECT_CHANNEL_GID, ChannelStatus::ChannelAvailable);

        self.channel_statuses
            .insert(INDIRECT_CHANNEL_GID, ChannelStatus::ChannelAvailable);

        log_debug!("shutdown: Closing all connections");
        let keys: Vec<String> = self.connections.keys().cloned().collect();
        for connection_id in keys {
            self.close_connection(NULL_RACE_HANDLE, &connection_id);
        }

        self.connections.clear();
        self.links.clear();

        log_info!("shutdown: returned");
        return PluginResponse::PluginOk;
    }

    fn send_package(
        &mut self,
        handle: u64,
        connection_id: &str,
        pkg: &[u8],
        _timeout_timestamp: f64,
        _batch_id: u64,
    ) -> PluginResponse {
        let log_prefix = format!(
            "send_package (handle: {}, connection ID: {}): ",
            handle, connection_id
        );
        log_info!("{}called", log_prefix);
        log_debug!("{}    handle: {}", log_prefix, handle);
        log_debug!("{}    connection_id: {}", log_prefix, connection_id);
        defer! {{
            log_info!("{}sendPackage returned",log_prefix);
        }};

        let sdk = self.sdk.as_ref().lock().unwrap();

        let connection = match self.connections.get(connection_id) {
            Some(connection) => connection,
            None => {
                log_error!(
                    "{}Cannot send message on connection {}, is NOT a valid connection",
                    log_prefix,
                    connection_id
                );
                expect_sdk_ok(sdk.on_package_status_changed(
                    handle,
                    PackageStatus::PackageFailedGeneric,
                    RACE_BLOCKING,
                ));
                return PluginResponse::PluginError;
            }
        };

        let link = connection.get_link().unwrap();
        let mut link = link.lock().unwrap();
        match link.get_link_type() {
            LinkType::LtSend => {}
            LinkType::LtBidi => {}
            _ => {
                log_error!(
                    "{}Trying to send on a connection with invalid link type: {:?}",
                    log_prefix,
                    link.get_link_type()
                );
                expect_sdk_ok(sdk.on_package_status_changed(
                    handle,
                    PackageStatus::PackageFailedGeneric,
                    RACE_BLOCKING,
                ));
                return PluginResponse::PluginError;
            }
        }

        log_debug!(
            "{}Sending package on connection {}",
            log_prefix,
            connection_id
        );

        match link.send_package(handle, connection_id, pkg) {
            Ok(_) => {
                if !expect_sdk_ok(sdk.on_package_status_changed(
                    handle,
                    PackageStatus::PackageSent,
                    RACE_BLOCKING,
                )) {
                    return PluginResponse::PluginError;
                }
            }
            Err(_) => {
                expect_sdk_ok(sdk.on_package_status_changed(
                    handle,
                    PackageStatus::PackageFailedGeneric,
                    RACE_BLOCKING,
                ));
                return PluginResponse::PluginError;
            }
        }

        return PluginResponse::PluginOk;
    }

    fn open_connection(
        &mut self,
        handle: u64,
        link_type: LinkType,
        link_id: &str,
        link_hints: &str,
        _send_timeout: i32,
    ) -> PluginResponse {
        log_info!("open_connection called");
        log_debug!("    handle:    {}", handle);
        log_debug!("    link_type:    {:?}", link_type);
        log_debug!("    link_id:      {}", link_id);
        log_debug!("    link_hints: {}", link_hints);
        defer! {{
            log_info!("open_connection returned");
        }};

        let sdk = self.sdk.as_ref().lock().unwrap();

        let conn_id = sdk.generate_connection_id(link_id);

        let mut link = match self.links.get(link_id) {
            Some(link) => link.lock().unwrap(),
            None => {
                log_error!("No link with ID: {}", link_id);
                expect_sdk_ok(sdk.on_connection_status_changed(
                    handle,
                    conn_id.as_str(),
                    ConnectionStatus::ConnectionClosed,
                    &LinkProperties::default(),
                    RACE_BLOCKING,
                ));
                return PluginResponse::PluginError;
            }
        };

        // Make sure requested link and requested link type are compatible
        match (link.get_link_type(), link_type) {
            (LinkType::LtBidi, _) => {}
            (LinkType::LtSend, LinkType::LtSend) => {}
            (LinkType::LtRecv, LinkType::LtRecv) => {}
            (_, _) => {
                log_error!(
                    "Tried to open link with mismatched link type: {:?} vs {:?}",
                    link.get_link_type(),
                    link_type
                );
                expect_sdk_ok(sdk.on_connection_status_changed(
                    handle,
                    conn_id.as_str(),
                    ConnectionStatus::ConnectionClosed,
                    &link.get_link_properties(),
                    RACE_BLOCKING,
                ));
                return PluginResponse::PluginError;
            }
        }

        let connection = link.open_connection(link_type, conn_id.as_str(), link_hints);
        self.connections.insert(conn_id.clone(), connection);

        expect_sdk_ok(sdk.on_connection_status_changed(
            handle,
            conn_id.as_str(),
            ConnectionStatus::ConnectionOpen,
            &link.get_link_properties(),
            RACE_BLOCKING,
        ));

        return PluginResponse::PluginOk;
    }

    fn close_connection(&mut self, handle: u64, connection_id: &str) -> PluginResponse {
        log_info!("close_connection called");
        log_debug!("    handle: {}", handle);
        log_debug!("    connection_id: {}", connection_id);

        let sdk = self.sdk.as_ref().lock().unwrap();

        let connection = match self.connections.remove(connection_id) {
            Some(connection) => connection,
            None => {
                log_error!("Cannot close {}, is NOT a valid connection", connection_id);
                return PluginResponse::PluginError;
            }
        };

        let link = connection.get_link().unwrap();
        let mut link = link.lock().unwrap();
        link.close_connection(connection_id);
        expect_sdk_ok(sdk.on_connection_status_changed(
            handle,
            connection_id,
            ConnectionStatus::ConnectionClosed,
            &link.get_link_properties(),
            RACE_BLOCKING,
        ));

        return PluginResponse::PluginOk;
    }

    fn destroy_link(&mut self, handle: u64, link_id: &str) -> PluginResponse {
        let log_prefix = format!("destroy_link (handle: {}, link_id: {}): ", handle, link_id);
        log_debug!("{}called", log_prefix);

        if !self.links.contains_key(link_id) {
            log_error!("{}unknown link ID", log_prefix);
            return PluginResponse::PluginError;
        }

        let sdk = self.sdk.as_ref().lock().unwrap();
        expect_sdk_ok(sdk.on_link_status_changed(
            handle,
            &link_id,
            LinkStatus::LinkDestroyed,
            &LinkProperties::default(),
            RACE_BLOCKING,
        ));
        drop(sdk);

        let connection_ids: Vec<String> = self.connections.keys().cloned().collect();
        for connection_id in connection_ids {
            let connection = self.connections.get(&connection_id).unwrap();
            let link = connection.get_link().unwrap();
            let link_id_for_connection = link.lock().unwrap().get_link_id();
            if link_id_for_connection == link_id {
                self.close_connection(NULL_RACE_HANDLE, &connection_id);
            }
        }

        // Remove the link.
        self.links.remove(link_id);

        log_debug!("{}returned", log_prefix);
        return PluginResponse::PluginOk;
    }

    fn create_link(&mut self, handle: u64, channel_gid: &str) -> PluginResponse {
        let log_prefix = format!(
            "create_link (handle: {}, channel GID: {}): ",
            handle, channel_gid
        );
        log_debug!("{}called", log_prefix);

        let sdk = self.sdk.as_ref().lock().unwrap();
        let link_id = sdk.generate_link_id(channel_gid);

        if !self.channel_statuses.contains_key(&channel_gid) {
            log_error!("{}unknown channel GID.", log_prefix);
            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkDestroyed,
                &LinkProperties::default(),
                RACE_BLOCKING,
            ));
            return PluginResponse::PluginError;
        } else if !matches!(
            self.channel_statuses.get(&channel_gid).unwrap(),
            ChannelStatus::ChannelAvailable
        ) {
            log_error!("{}channel not available", log_prefix);
            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkDestroyed,
                &LinkProperties::default(),
                RACE_BLOCKING,
            ));
            return PluginResponse::PluginError;
        }

        let mut link_props = match get_default_link_properties_for_channel(&*sdk, &channel_gid) {
            Ok(props) => props,
            Err(_) => {
                log_error!("{}unknown channel GID.", log_prefix);
                expect_sdk_ok(sdk.on_link_status_changed(
                    handle,
                    &link_id,
                    LinkStatus::LinkDestroyed,
                    &LinkProperties::default(),
                    RACE_BLOCKING,
                ));
                return PluginResponse::PluginError;
            }
        };

        if channel_gid == DIRECT_CHANNEL_GID {
            link_props.link_type = LinkType::LtRecv;
            let link_profile = direct_link::LinkProfile {
                hostname: self.hostname.clone(),
                port: self.next_available_port,
            };
            self.next_available_port += 1;
            link_props.link_address = serde_json::to_string(&link_profile).unwrap();

            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkCreated,
                &link_props,
                RACE_BLOCKING,
            ));
            sdk.update_link_properties(&link_id, &link_props, RACE_BLOCKING);

            log_debug!(
                "{}link created with link ID: {} and address: {}",
                log_prefix,
                link_id,
                link_props.link_address
            );

            drop(sdk);
            let link =
                direct_link::DirectLink::new(&link_id, link_props, link_profile, self.sdk.clone());
            self.links.insert(link_id, link);
        } else if channel_gid == INDIRECT_CHANNEL_GID {
            link_props.link_type = LinkType::LtBidi;
            let link_profile = whiteboard_link::LinkProfile {
                hostname: self.whiteboard_hostname.clone(),
                port: self.whiteboard_port,
                hashtag: format!(
                    "rust_{}_{}",
                    self.active_persona, self.next_available_hashtag
                ),
                check_frequency_ms: 1000,
            };
            self.next_available_hashtag += 1;
            link_props.link_address = serde_json::to_string(&link_profile).unwrap();

            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkCreated,
                &link_props,
                RACE_BLOCKING,
            ));
            sdk.update_link_properties(&link_id, &link_props, RACE_BLOCKING);

            log_debug!(
                "{}link created with link ID: {} and address: {}",
                log_prefix,
                link_id,
                link_props.link_address
            );

            drop(sdk);
            let link = whiteboard_link::WhiteboardLink::new(
                &link_id,
                link_props,
                link_profile,
                self.sdk.clone(),
            );
            self.links.insert(link_id, link);
        } else {
            log_error!("{}invalid channel GID.", log_prefix);
            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkDestroyed,
                &LinkProperties::default(),
                RACE_BLOCKING,
            ));
            return PluginResponse::PluginError;
        }

        log_debug!("{}returned", log_prefix);
        return PluginResponse::PluginOk;
    }

    fn create_link_from_address(
        &mut self,
        handle: u64,
        channel_gid: &str,
        link_address: &str,
    ) -> PluginResponse {
        let log_prefix = format!(
            "load_link_address (handle: {}, channel GID: {}): ",
            handle, channel_gid
        );
        log_debug!(
            "{}called: creating link address: {}",
            log_prefix,
            link_address
        );

        let sdk = self.sdk.as_ref().lock().unwrap();
        let link_id = sdk.generate_link_id(channel_gid);

        if !self.channel_statuses.contains_key(&channel_gid) {
            log_error!("{}unknown channel GID.", log_prefix);
            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkDestroyed,
                &LinkProperties::default(),
                RACE_BLOCKING,
            ));
            return PluginResponse::PluginError;
        } else if !matches!(
            self.channel_statuses.get(&channel_gid).unwrap(),
            ChannelStatus::ChannelAvailable
        ) {
            log_error!("{}channel not available", log_prefix);
            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkDestroyed,
                &LinkProperties::default(),
                RACE_BLOCKING,
            ));
            return PluginResponse::PluginError;
        }

        let mut link_props = match get_default_link_properties_for_channel(&*sdk, &channel_gid) {
            Ok(props) => props,
            Err(_) => {
                log_error!("{}unknown channel GID.", log_prefix);
                expect_sdk_ok(sdk.on_link_status_changed(
                    handle,
                    &link_id,
                    LinkStatus::LinkDestroyed,
                    &LinkProperties::default(),
                    RACE_BLOCKING,
                ));
                return PluginResponse::PluginError;
            }
        };

        link_props.link_address = link_address.to_owned();
        if channel_gid == DIRECT_CHANNEL_GID {
            link_props.link_type = LinkType::LtRecv;

            let link_profile: direct_link::LinkProfile =
                match serde_json::from_str(&link_props.link_address) {
                    Ok(link_profile) => link_profile,
                    Err(_) => {
                        log_error!(
                            "{}invalid link address: {}",
                            log_prefix,
                            link_props.link_address
                        );
                        expect_sdk_ok(sdk.on_link_status_changed(
                            handle,
                            &link_id,
                            LinkStatus::LinkDestroyed,
                            &LinkProperties::default(),
                            RACE_BLOCKING,
                        ));
                        return PluginResponse::PluginError;
                    }
                };

            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkCreated,
                &link_props,
                RACE_BLOCKING,
            ));
            expect_sdk_ok(sdk.update_link_properties(&link_id, &link_props, RACE_BLOCKING));

            log_debug!(
                "{}link loaded with link ID: {} and address: {}",
                log_prefix,
                link_id,
                link_props.link_address
            );

            drop(sdk);
            let link =
                direct_link::DirectLink::new(&link_id, link_props, link_profile, self.sdk.clone());
            self.links.insert(link_id, link);
        } else if channel_gid == INDIRECT_CHANNEL_GID {
            link_props.link_type = LinkType::LtBidi;
            let link_profile: whiteboard_link::LinkProfile =
                match serde_json::from_str(&link_props.link_address) {
                    Ok(link_profile) => link_profile,
                    Err(_) => {
                        log_error!(
                            "{}invalid link address: {}",
                            log_prefix,
                            link_props.link_address
                        );
                        expect_sdk_ok(sdk.on_link_status_changed(
                            handle,
                            &link_id,
                            LinkStatus::LinkDestroyed,
                            &LinkProperties::default(),
                            RACE_BLOCKING,
                        ));
                        return PluginResponse::PluginError;
                    }
                };

            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkCreated,
                &link_props,
                RACE_BLOCKING,
            ));
            sdk.update_link_properties(&link_id, &link_props, RACE_BLOCKING);

            log_debug!(
                "{}link loaded with link ID: {} and address: {}",
                log_prefix,
                link_id,
                link_props.link_address
            );

            drop(sdk);
            let link = whiteboard_link::WhiteboardLink::new(
                &link_id,
                link_props,
                link_profile,
                self.sdk.clone(),
            );
            self.links.insert(link_id, link);
        } else {
            log_error!("{}invalid channel GID.", log_prefix);
            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkDestroyed,
                &LinkProperties::default(),
                RACE_BLOCKING,
            ));
            return PluginResponse::PluginError;
        }

        log_debug!("{}returned", log_prefix);
        return PluginResponse::PluginOk;
    }

    fn load_link_address(
        &mut self,
        handle: u64,
        channel_gid: &str,
        link_address: &str,
    ) -> PluginResponse {
        let log_prefix = format!(
            "load_link_address (handle: {}, channel GID: {}): ",
            handle, channel_gid
        );
        log_debug!(
            "{}called: loading link address: {}",
            log_prefix,
            link_address
        );

        let sdk = self.sdk.as_ref().lock().unwrap();
        let link_id = sdk.generate_link_id(channel_gid);

        if !self.channel_statuses.contains_key(&channel_gid) {
            log_error!("{}unknown channel GID.", log_prefix);
            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkDestroyed,
                &LinkProperties::default(),
                RACE_BLOCKING,
            ));
            return PluginResponse::PluginError;
        } else if !matches!(
            self.channel_statuses.get(&channel_gid).unwrap(),
            ChannelStatus::ChannelAvailable
        ) {
            log_error!("{}channel not available", log_prefix);
            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkDestroyed,
                &LinkProperties::default(),
                RACE_BLOCKING,
            ));
            return PluginResponse::PluginError;
        }

        let mut link_props = match get_default_link_properties_for_channel(&*sdk, &channel_gid) {
            Ok(props) => props,
            Err(_) => {
                log_error!("{}unknown channel GID.", log_prefix);
                expect_sdk_ok(sdk.on_link_status_changed(
                    handle,
                    &link_id,
                    LinkStatus::LinkDestroyed,
                    &LinkProperties::default(),
                    RACE_BLOCKING,
                ));
                return PluginResponse::PluginError;
            }
        };

        if channel_gid == DIRECT_CHANNEL_GID {
            link_props.link_type = LinkType::LtSend;

            let link_profile: direct_link::LinkProfile = match serde_json::from_str(&link_address) {
                Ok(link_profile) => link_profile,
                Err(_) => {
                    log_error!("{}invalid link address: {}", log_prefix, link_address);
                    expect_sdk_ok(sdk.on_link_status_changed(
                        handle,
                        &link_id,
                        LinkStatus::LinkDestroyed,
                        &LinkProperties::default(),
                        RACE_BLOCKING,
                    ));
                    return PluginResponse::PluginError;
                }
            };

            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkLoaded,
                &link_props,
                RACE_BLOCKING,
            ));
            sdk.update_link_properties(&link_id, &link_props, RACE_BLOCKING);

            log_debug!(
                "{}link loaded with link ID: {} and address: {}",
                log_prefix,
                link_id,
                link_address
            );

            drop(sdk);
            let link =
                direct_link::DirectLink::new(&link_id, link_props, link_profile, self.sdk.clone());
            self.links.insert(link_id, link);
        } else if channel_gid == INDIRECT_CHANNEL_GID {
            link_props.link_type = LinkType::LtBidi;
            let link_profile: whiteboard_link::LinkProfile =
                match serde_json::from_str(&link_address) {
                    Ok(link_profile) => link_profile,
                    Err(_) => {
                        log_error!("{}invalid link address: {}", log_prefix, link_address);
                        expect_sdk_ok(sdk.on_link_status_changed(
                            handle,
                            &link_id,
                            LinkStatus::LinkDestroyed,
                            &LinkProperties::default(),
                            RACE_BLOCKING,
                        ));
                        return PluginResponse::PluginError;
                    }
                };

            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkLoaded,
                &link_props,
                RACE_BLOCKING,
            ));
            sdk.update_link_properties(&link_id, &link_props, RACE_BLOCKING);

            log_debug!(
                "{}link loaded with link ID: {} and address: {}",
                log_prefix,
                link_id,
                link_address
            );

            drop(sdk);
            let link = whiteboard_link::WhiteboardLink::new(
                &link_id,
                link_props,
                link_profile,
                self.sdk.clone(),
            );
            self.links.insert(link_id, link);
        } else {
            log_error!("{}invalid channel GID.", log_prefix);
            expect_sdk_ok(sdk.on_link_status_changed(
                handle,
                &link_id,
                LinkStatus::LinkDestroyed,
                &LinkProperties::default(),
                RACE_BLOCKING,
            ));
            return PluginResponse::PluginError;
        }

        log_debug!("{}returned", log_prefix);
        return PluginResponse::PluginOk;
    }

    fn load_link_addresses(
        &mut self,
        handle: u64,
        channel_gid: &str,
        link_addresses: &[&str],
    ) -> PluginResponse {
        let log_prefix = format!(
            "load_link_addresses (handle: {}, channel GID: {}): ",
            handle, channel_gid
        );
        log_debug!(
            "{}called: loading link addresses: {:?}",
            log_prefix,
            link_addresses
        );
        log_warning!("load_link_addresses: API not compatible with any existing Two Six channels! handle: {} channel GID: {} link addresses: {:?}", handle, channel_gid, link_addresses);
        let sdk = self.sdk.as_ref().lock().unwrap();
        expect_sdk_ok(sdk.on_link_status_changed(
            handle,
            "",
            LinkStatus::LinkDestroyed,
            &LinkProperties::default(),
            RACE_BLOCKING,
        ));
        log_debug!("{}returned", log_prefix);
        return PluginResponse::PluginError;
    }

    fn activate_channel(
        &mut self,
        handle: u64,
        channel_gid: &str,
        _role_name: &str,
    ) -> PluginResponse {
        let log_prefix = format!(
            "activate_channel (handle: {}, channel GID: {}): ",
            handle, channel_gid
        );
        log_debug!("{}called", log_prefix);

        if !self.channel_statuses.contains_key(&channel_gid) {
            log_error!("{}unknown channel GID.", log_prefix);
            return PluginResponse::PluginError;
        }

        let sdk = self.sdk.as_ref().lock().unwrap();

        if channel_gid == INDIRECT_CHANNEL_GID {
            self.channel_statuses
                .insert(INDIRECT_CHANNEL_GID, ChannelStatus::ChannelAvailable);
            expect_sdk_ok(sdk.on_channel_status_changed(
                NULL_RACE_HANDLE,
                INDIRECT_CHANNEL_GID,
                ChannelStatus::ChannelAvailable,
                &get_default_channel_properties_for_channel(&*sdk, &INDIRECT_CHANNEL_GID).unwrap(),
                RACE_BLOCKING,
            ));
            let user_display_info = format!("{} is ready", INDIRECT_CHANNEL_GID);
            expect_sdk_ok(sdk.display_info_to_user(&*user_display_info, UdToast));
        } else if channel_gid == DIRECT_CHANNEL_GID {
            self.channel_statuses
                .insert(DIRECT_CHANNEL_GID, ChannelStatus::ChannelStarting);

            let response = sdk.request_common_user_input("hostname");
            match response.status {
                SdkStatus::SdkOk => {
                    self.request_hostname_handle = response.handle;
                    self.direct_channel_user_input_requests
                        .insert(response.handle);
                }
                _ => {
                    log_error!(
                        "Failed to request hostname from user: {:?}, direct channel cannot be used",
                        response.status
                    );
                    self.channel_statuses
                        .insert(DIRECT_CHANNEL_GID, ChannelStatus::ChannelFailed);
                    expect_sdk_ok(sdk.on_channel_status_changed(
                        NULL_RACE_HANDLE,
                        DIRECT_CHANNEL_GID,
                        ChannelStatus::ChannelFailed,
                        &ChannelProperties::default(),
                        RACE_BLOCKING,
                    ));
                    // Don't continue
                    return PluginResponse::PluginOk;
                }
            }

            let response = sdk.request_plugin_user_input(
                "startPort",
                "What is the first available port?",
                true,
            );
            match response.status {
                SdkStatus::SdkOk => {
                    self.request_start_port_handle = response.handle;
                    self.direct_channel_user_input_requests
                        .insert(response.handle);
                }
                _ => {
                    log_warning!(
                        "Failed to request start port from user: {:?}",
                        response.status
                    );
                }
            }
        }

        log_debug!("{}returned", log_prefix);
        return PluginResponse::PluginOk;
    }

    fn deactivate_channel(&mut self, handle: u64, channel_gid: &str) -> PluginResponse {
        let log_prefix = format!(
            "deactivate_channel (handle: {}, channel GID: {}): ",
            handle, channel_gid
        );
        log_debug!("{}called", log_prefix);

        if !self.channel_statuses.contains_key(&channel_gid) {
            log_error!("{}unknown channel GID.", log_prefix);
            return PluginResponse::PluginError;
        }

        if channel_gid == DIRECT_CHANNEL_GID {
            self.channel_statuses
                .insert(DIRECT_CHANNEL_GID, ChannelStatus::ChannelUnavailable);
        } else if channel_gid == INDIRECT_CHANNEL_GID {
            self.channel_statuses
                .insert(INDIRECT_CHANNEL_GID, ChannelStatus::ChannelUnavailable);
        }

        let sdk = self.sdk.as_ref().lock().unwrap();
        sdk.on_channel_status_changed(
            handle,
            channel_gid,
            ChannelStatus::ChannelUnavailable,
            &ChannelProperties::default(),
            RACE_BLOCKING,
        );
        drop(sdk);

        let mut links_to_destroy = vec![];
        for link_id in self.links.keys() {
            if self
                .links
                .get(link_id)
                .unwrap()
                .lock()
                .unwrap()
                .get_link_properties()
                .channel_gid
                == channel_gid
            {
                links_to_destroy.push(link_id.clone());
            }
        }

        for link_id in links_to_destroy {
            self.destroy_link(handle, &link_id);
        }

        log_debug!("{}returned", log_prefix);
        return PluginResponse::PluginOk;
    }

    fn on_user_input_received(
        &mut self,
        handle: u64,
        answered: bool,
        response: &str,
    ) -> PluginResponse {
        let log_prefix = format!("on_user_input_received (handle: {}): ", handle);
        log_debug!("{}called", log_prefix);

        let mut was_direct_channel_response = false;
        if handle == self.request_hostname_handle {
            if answered {
                self.hostname = response.to_string();
                log_info!("{}using hostname {}", log_prefix, self.hostname);
            } else {
                log_error!(
                    "{}direct channel not available without the hostname",
                    log_prefix
                );
                self.channel_statuses
                    .insert(DIRECT_CHANNEL_GID, ChannelStatus::ChannelDisabled);
                let sdk = self.sdk.as_ref().lock().unwrap();
                sdk.on_channel_status_changed(
                    NULL_RACE_HANDLE,
                    DIRECT_CHANNEL_GID,
                    ChannelStatus::ChannelDisabled,
                    &get_default_channel_properties_for_channel(&*sdk, &DIRECT_CHANNEL_GID)
                        .unwrap(),
                    RACE_BLOCKING,
                );
                // Don't continue processing
                return PluginResponse::PluginOk;
            }
            was_direct_channel_response = true;
            self.direct_channel_user_input_requests.remove(&handle);
        } else if handle == self.request_start_port_handle {
            if answered {
                match response.parse::<u32>() {
                    Ok(port) => {
                        self.next_available_port = port;
                        log_info!("{}using start port {}", log_prefix, port);
                    }
                    Err(_) => {
                        log_warning!("{}error parsing start port, {}", log_prefix, response);
                    }
                }
            } else {
                log_warning!("{}no answer, using default start port", log_prefix);
            }
            was_direct_channel_response = true;
            self.direct_channel_user_input_requests.remove(&handle);
        } else {
            log_warning!("{}handle is not recognized", log_prefix);
            return PluginResponse::PluginError;
        }

        // If this was the last user input request for the direct channel, make it available
        if was_direct_channel_response && self.direct_channel_user_input_requests.is_empty() {
            self.channel_statuses
                .insert(DIRECT_CHANNEL_GID, ChannelStatus::ChannelAvailable);
            let sdk = self.sdk.as_ref().lock().unwrap();
            expect_sdk_ok(sdk.on_channel_status_changed(
                NULL_RACE_HANDLE,
                DIRECT_CHANNEL_GID,
                ChannelStatus::ChannelAvailable,
                &get_default_channel_properties_for_channel(&*sdk, &DIRECT_CHANNEL_GID).unwrap(),
                RACE_BLOCKING,
            ));
            let user_display_info = format!("{} is ready", DIRECT_CHANNEL_GID);
            expect_sdk_ok(sdk.display_info_to_user(&*user_display_info, UdToast));
        }

        log_debug!("{}returned", log_prefix);
        return PluginResponse::PluginOk;
    }

    fn plugin_flush_channel(
        &mut self,
        _handle: u64,
        _channel_gid: &str,
        _batch_id: u64,
    ) -> PluginResponse {
        log_error!("plugin_flush_channel: plugin does not support flushing");
        return PluginResponse::PluginError;
    }

    fn plugin_on_user_acknowledgment_received(&mut self, _handle: u64) -> PluginResponse {
        log_debug!("plugin_on_user_acknowledgment_received: called");
        return PluginResponse::PluginOk;
    }
}
