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

use std::collections::HashSet;
use std::io;
use std::sync::{mpsc, Arc, Mutex, Weak};
use std::thread;

use serde::{Deserialize, Serialize};

use shims::race_common::i_race_sdk_comms::{IRaceSdkComms, RACE_BLOCKING};
use shims::race_common::link_properties::{
    ConnectionType, LinkProperties, LinkType, TransmissionType,
};
use shims::race_common::sdk_response::SdkStatus;
use shims::race_common::send_type::SendType;

use super::link::{Connection, Link, LinkConfig, LinkProfileParser};
use super::socketlistener;
use super::socketsender;

const PLACEHOLDER_ADDR: &str = "0.0.0.0";

/// Direct/unicast link profile
#[derive(Serialize, Deserialize)]
pub struct LinkProfile {
    pub hostname: String,
    pub port: u32,
}

/// A direct/unicast link (TCP)
pub struct DirectLink {
    /// Pointer to self so it can be given to connections created by this link
    self_ptr: Option<Weak<Mutex<Self>>>,

    /// Link ID assigned to this link
    link_id: String,
    /// Properties of this link
    link_properties: LinkProperties,
    /// Profile parameters of this link
    profile: LinkProfile,

    /// Reference to SDK
    sdk: Arc<Mutex<dyn IRaceSdkComms + Send>>,

    /// Receiving connection IDs
    receive_connection_ids: Arc<Mutex<HashSet<String>>>,
    /// Sender side of channel to receiving thread to be used for sending terminate commands
    server_thread_channel_tx: Option<mpsc::Sender<bool>>,
    /// Receiving thread join handle
    server_thread_join_handle: Option<thread::JoinHandle<()>>,
}

impl DirectLink {
    /// Creates a direct link instance
    ///
    /// # Arguments
    ///
    /// * `link_id` - The link ID assigned to the link
    /// * `link_properties` - The properties of the link
    /// * `profile` - The profile parameters of the link
    /// * `sdk` - SDK reference
    pub fn new(
        link_id: &String,
        link_properties: LinkProperties,
        profile: LinkProfile,
        sdk: Arc<Mutex<dyn IRaceSdkComms + Send>>,
    ) -> Arc<Mutex<dyn Link>> {
        let arc = Arc::new(Mutex::new(Self {
            self_ptr: None,
            link_id: link_id.clone(),
            link_properties,
            profile,
            sdk,
            receive_connection_ids: Arc::new(Mutex::new(HashSet::new())),
            server_thread_channel_tx: None,
            server_thread_join_handle: None,
        }));
        arc.lock().unwrap().self_ptr = Some(Arc::downgrade(&arc));
        arc
    }

    /// Starts the receiving thread, opening up a TCP server socket
    fn start_receive_connection(&mut self) {
        log_debug!("Creating server thread");

        // Clone all the variables we need to move to the receive thread
        let sdk = self.sdk.clone();
        let hostname = String::from(PLACEHOLDER_ADDR);
        let port = self.profile.port.clone();
        let connection_ids = self.receive_connection_ids.clone();

        let (channel_tx, channel_rx) = mpsc::channel::<bool>();

        let join_handle = thread::Builder::new()
            .name(format!("{}:{}", hostname, port))
            .spawn(move || {
                let listener = socketlistener::Listener {};
                listener.start_server(
                    channel_rx,
                    hostname,
                    port.to_string(),
                    Box::new(move |message| {
                        log_info!("Received message of length {}", message.len());
                        let copy_of_ids = { connection_ids.lock().unwrap().clone() };
                        let copy_of_ids: Vec<&str> =
                            copy_of_ids.iter().map(std::ops::Deref::deref).collect();
                        let response = sdk.lock().unwrap().receive_enc_pkg(
                            &message,
                            &copy_of_ids[..],
                            RACE_BLOCKING,
                        );
                        match response.status {
                            SdkStatus::SdkOk => {}
                            _ => log_error!(
                                "Received unexpected response from SDK: {:?}",
                                response.status
                            ),
                        };
                    }),
                );
            });

        self.server_thread_join_handle = Some(join_handle.unwrap());
        self.server_thread_channel_tx = Some(channel_tx);
    }
}

impl Link for DirectLink {
    /// See Link::get_link_id
    fn get_link_id(&self) -> String {
        self.link_id.clone()
    }

    /// See Link::get_link_type
    fn get_link_type(&self) -> LinkType {
        self.link_properties.link_type
    }

    /// See Link::get_link_properties
    fn get_link_properties(&self) -> LinkProperties {
        self.link_properties.clone()
    }

    /// See Link::open_connection
    fn open_connection(
        &mut self,
        link_type: LinkType,
        connection_id: &str,
        _link_hints: &str,
    ) -> Connection {
        log_debug!("open_connection called");
        log_debug!("    type:    {:?}", link_type);
        log_debug!("    ID:      {}", connection_id);

        match link_type {
            LinkType::LtSend => {
                log_info!(
                    "Opening send link to {}:{}",
                    self.profile.hostname,
                    self.profile.port
                );
            }
            LinkType::LtRecv | LinkType::LtBidi => {
                log_info!(
                    "Opening receive link on {}:{}",
                    PLACEHOLDER_ADDR,
                    self.profile.port
                );

                {
                    self.receive_connection_ids
                        .lock()
                        .unwrap()
                        .insert(String::from(connection_id));
                }

                if self.server_thread_join_handle.is_some() {
                    log_debug!(
                        "Connection already open on {}:{}",
                        PLACEHOLDER_ADDR,
                        self.profile.port
                    );
                } else {
                    self.start_receive_connection();
                }
            }
            // This is just precautionary since the plugin open_connection should have caught this already
            _ => log_error!("Received invalid link type to open: {:?}", link_type),
        }

        Connection::new(
            connection_id,
            link_type,
            self.self_ptr.as_ref().unwrap().upgrade().unwrap(),
        )
    }

    /// See Link::close_connection
    fn close_connection(&mut self, connection_id: &str) {
        let log_prefix = format!(
            "DirectLink::close_connection (connection ID: {}): ",
            connection_id
        );
        log_debug!("{}called", log_prefix);
        log_debug!("{}    ID: {}", connection_id, log_prefix);

        let should_stop = {
            let mut conn_ids = self.receive_connection_ids.lock().unwrap();
            conn_ids.remove(connection_id) && conn_ids.is_empty()
        };

        if should_stop {
            log_debug!("{}stopping thread", log_prefix);
            match &self.server_thread_channel_tx {
                Some(channel_tx) => {
                    log_debug!("{}Sending the stop command to server thread", log_prefix);
                    match channel_tx.send(true) {
                        Ok(_) => {}
                        Err(_) => {
                            log_error!("{}Error sending stop command to server thread", log_prefix)
                        }
                    }
                }
                None => {}
            };
        }
    }

    /// See Link::send_package
    fn send_package(
        &mut self,
        _handle: u64,
        connection_id: &str,
        package: &[u8],
    ) -> Result<(), io::Error> {
        log_debug!("Sending unicast package on {}", connection_id);
        let sender = socketsender::Sender {};
        sender.start_client(
            &self.profile.hostname,
            &self.profile.port.to_string(),
            &package.to_vec(),
        )
    }
}

/// Configuration parser and factory for direct/unicast links
pub struct DirectLinkParser {
    config: LinkConfig,
}

impl DirectLinkParser {
    /// Creates a new direct/unicast link parser/factory
    ///
    /// # Arguments
    ///
    /// * `config` - Link configuration
    pub fn new(config: &LinkConfig) -> Box<Self> {
        Box::new(Self {
            config: config.clone(),
        })
    }
}

impl LinkProfileParser for DirectLinkParser {
    /// See LinkProfileParser::create_link
    fn create_link(
        &self,
        sdk: Arc<Mutex<dyn IRaceSdkComms + Send>>,
        channel_gid: &str,
    ) -> Option<Arc<Mutex<dyn Link>>> {
        log_debug!("Creating unicast link instance: {:?}", self.config);

        let locked_sdk = sdk.lock().unwrap();

        // TODO: Can the plugin get the personas connected to this link, generate the link ID, and
        // pass it into the link to decouple the link from IRaceSdkComms?
        let link_id = locked_sdk.generate_link_id(channel_gid);
        if link_id.is_empty() {
            log_error!(
                "create_link: failed to generate link ID for channel with GID: {:?}",
                channel_gid
            );
            return None;
        }

        let mut props = self.config.get_link_properties();
        props.transmission_type = TransmissionType::TtUnicast;
        props.connection_type = ConnectionType::CtDirect;
        props.send_type = SendType::StEphemSync;
        props.channel_gid = channel_gid.to_string();
        let response = locked_sdk.update_link_properties(&link_id, &props, RACE_BLOCKING);
        match response.status {
            SdkStatus::SdkOk => {}
            _ => log_error!(
                "Error updating link properties with SDK: {:?}",
                response.status
            ),
        };

        let profile: LinkProfile = match serde_json::from_str(self.config.profile.as_str()) {
            Ok(profile) => profile,
            _ => {
                log_error!(
                    "Unable to parse direct link profile from: {}",
                    self.config.profile
                );
                return None;
            }
        };

        // Need to unlock the mutex on the SDK so we can move the Arc to the link
        drop(locked_sdk);
        Some(DirectLink::new(&link_id, props, profile, sdk))
    }
}
