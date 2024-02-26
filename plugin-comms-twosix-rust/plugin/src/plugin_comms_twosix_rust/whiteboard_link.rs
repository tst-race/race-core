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
use std::convert::TryInto;
use std::fmt;
use std::fmt::{Display, Formatter};
use std::io;
use std::sync::atomic::{AtomicI32, Ordering};
use std::sync::{Arc, Mutex, Weak};
use std::time::Duration;

use base64::{decode, encode};

use clokwerk::{Interval, ScheduleHandle, Scheduler};

use serde::{Deserialize, Serialize};

use shims::race_common::i_race_sdk_comms::{IRaceSdkComms, RACE_BLOCKING};
use shims::race_common::link_properties::{
    ConnectionType, LinkProperties, LinkType, TransmissionType,
};
use shims::race_common::sdk_response::SdkStatus;
use shims::race_common::send_type::SendType;

use super::link::{Connection, Link, LinkConfig, LinkProfileParser};
use super::whiteboard_client::WhiteboardClient;

/// Whiteboard/multicast link profile
#[derive(Clone, Serialize, Deserialize)]
pub struct LinkProfile {
    pub hostname: String,
    pub port: u32,
    pub hashtag: String,
    #[serde(rename = "checkFrequency")]
    pub check_frequency_ms: u32,
}

impl Display for LinkProfile {
    /// Creates a formatted string for the link profile, used in log messages
    /// when connecting and sending or receiving messages
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        write!(f, "http://{}:{} {}", self.hostname, self.port, self.hashtag)
    }
}

/// A whiteboard/multicast link
pub struct WhiteboardLink {
    /// Pointer to self so it can be given to connections created by this link
    self_ptr: Option<Weak<Mutex<Self>>>,

    /// Link ID assigned to this link
    link_id: String,
    /// Properties of this link
    link_properties: LinkProperties,
    /// Profile parameters of this link
    profile: LinkProfile,

    /// REST client to whiteboard service
    client: Arc<Mutex<WhiteboardClient>>,
    /// Index of last message retrieved from the whiteboard service
    last_index: Arc<AtomicI32>,

    /// Reference to SDK
    sdk: Arc<Mutex<dyn IRaceSdkComms + Send>>,

    /// Receiving connection IDs
    receive_connection_ids: Arc<Mutex<HashSet<String>>>,
    /// Scheduler thread handle
    scheduler_thread_handle: Option<ScheduleHandle>,
}

impl WhiteboardLink {
    /// Creates a whiteboard link instance
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
            profile: profile.clone(),
            client: Arc::new(Mutex::new(WhiteboardClient::new(
                profile.hostname,
                profile.port,
                profile.hashtag,
            ))),
            last_index: Arc::new(AtomicI32::new(0)),
            sdk,
            receive_connection_ids: Arc::new(Mutex::new(HashSet::new())),
            scheduler_thread_handle: None,
        }));
        arc.lock().unwrap().self_ptr = Some(Arc::downgrade(&arc));
        arc
    }

    /// Schedules a recurring task to poll the whiteboard service for new messages
    fn schedule_polling(&mut self) {
        let rate_sec = self.profile.check_frequency_ms / 1000;
        log_debug!("Scheduling whiteboard service poll every {}s", rate_sec);

        self.last_index.store(
            self.client.lock().unwrap().get_last_post_index(),
            Ordering::Relaxed,
        );

        // Clone all the variables needed in the scheduled task
        let sdk = self.sdk.clone();
        let client = self.client.clone();
        let last_index = self.last_index.clone();
        let connection_ids = self.receive_connection_ids.clone();

        let mut scheduler = Scheduler::new();
        scheduler.every(Interval::Seconds(rate_sec)).run(move || {
            let prev_last_index = last_index.load(Ordering::Relaxed);
            let messages = client.lock().unwrap().get_new_posts(prev_last_index);

            let expected_num: usize = (messages.length - prev_last_index).try_into().unwrap();
            let actual_num = messages.data.len();
            if actual_num < expected_num {
                let lost_num = expected_num - actual_num;
                log_error!(
                    "Expected {} posts, but only got {}. {} posts may have been lost.",
                    expected_num,
                    actual_num,
                    lost_num
                );
            }

            last_index.store(messages.length, Ordering::Relaxed);

            let copy_of_ids = { connection_ids.lock().unwrap().clone() };
            let copy_of_ids: Vec<&str> = copy_of_ids.iter().map(std::ops::Deref::deref).collect();

            for message in messages.data {
                match decode(message) {
                    Ok(enc_pkg) => {
                        sdk.lock().unwrap().receive_enc_pkg(
                            &enc_pkg,
                            &copy_of_ids[..],
                            RACE_BLOCKING,
                        );
                    }
                    Err(err) => {
                        log_error!("Unable to base64 decode message: {}", err);
                    }
                }
            }
        });

        // Start scheduler thread to execute scheduled tasks, checking every 100ms
        // (which is somewhat arbitrary, it just needs to be some rate higher than the
        // fastest scheduled task--which is 1Hz)
        self.scheduler_thread_handle = Some(scheduler.watch_thread(Duration::from_millis(100)));
    }
}

impl Link for WhiteboardLink {
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
                log_info!("Opening send link to {}", self.profile);
                log_debug!("Currently does nothing");
            }
            LinkType::LtRecv | LinkType::LtBidi => {
                log_info!("Opening receive link on {}", self.profile);

                {
                    self.receive_connection_ids
                        .lock()
                        .unwrap()
                        .insert(String::from(connection_id));
                }

                if self.scheduler_thread_handle.is_some() {
                    log_debug!("Connection already open for {}", self.profile);
                } else {
                    self.schedule_polling();
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
        log_debug!("close_connection called");
        log_debug!("    ID: {}", connection_id);

        let should_stop = {
            let mut conn_ids = self.receive_connection_ids.lock().unwrap();
            conn_ids.remove(connection_id) && conn_ids.is_empty()
        };

        if should_stop {
            match self.scheduler_thread_handle.take() {
                Some(thread_handle) => {
                    log_debug!("Stopping task scheduler");
                    thread_handle.stop();
                }
                None => {}
            };
        }
    }

    // See Link::send_package
    fn send_package(
        &mut self,
        _handle: u64,
        connection_id: &str,
        package: &[u8],
    ) -> Result<(), io::Error> {
        log_debug!("Sending multicast package on {}", connection_id);
        self.client.lock().unwrap().post_message(&encode(package))
    }
}

/// Configuration parser and factory for whiteboard/multicast links
pub struct WhiteboardLinkParser {
    config: LinkConfig,
}

impl WhiteboardLinkParser {
    /// Creates a new whiteboard/multicast link parser/factory
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

impl LinkProfileParser for WhiteboardLinkParser {
    /// See LinkProfileParser::create_link
    fn create_link(
        &self,
        sdk: Arc<Mutex<dyn IRaceSdkComms + Send>>,
        channel_gid: &str,
    ) -> Option<Arc<Mutex<dyn Link>>> {
        log_debug!("Creating whiteobard link instance: {:?}", self.config);

        let locked_sdk = sdk.lock().unwrap();

        let link_id = locked_sdk.generate_link_id(channel_gid);
        if link_id.is_empty() {
            log_error!(
                "create_link: failed to generate link ID for channel with GID: {:?}",
                channel_gid
            );
            return None;
        }

        let mut props = self.config.get_link_properties();
        props.transmission_type = TransmissionType::TtMulticast;
        props.connection_type = ConnectionType::CtIndirect;
        props.send_type = SendType::StStoredAsync;
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
                    "Unable to parse whiteboard link profile from: {}",
                    self.config.profile
                );
                return None;
            }
        };

        // Need to unlock the mutex on the SDK so we can move the Arc to the link
        drop(locked_sdk);
        Some(WhiteboardLink::new(&link_id, props, profile, sdk))
    }
}
