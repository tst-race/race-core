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

use std::io;
use std::sync::{Arc, Mutex, Weak};

use serde::Deserialize;

use shims::race_common::i_race_sdk_comms::IRaceSdkComms;
use shims::race_common::link_properties::{LinkProperties, LinkType};

use super::configloader::ConfigLoader;

/// An individual connection opened for a particular link
pub struct Connection {
    /// Reference to parent link
    link: Weak<Mutex<dyn Link>>,
    /// ID assigned to the connection
    pub connection_id: String,
    /// Link type (send, recv)
    pub link_type: LinkType,
}

impl Connection {
    /// Creates a connection with the given parameters
    ///
    /// # Arguments
    ///
    /// * `connection_id` - The connection ID created to identify the connection
    /// * `link_type` - Type of link connection (send, recv)
    /// * `link` - The parent link
    pub fn new(connection_id: &str, link_type: LinkType, link: Arc<Mutex<dyn Link>>) -> Connection {
        Connection {
            link: Arc::downgrade(&link),
            connection_id: String::from(connection_id),
            link_type,
        }
    }

    /// Obtains a reference to the parent link instance
    pub fn get_link(&self) -> Option<Arc<Mutex<dyn Link>>> {
        self.link.upgrade()
    }
}

/// A particular type of link
pub trait Link {
    /// Gets the ID of this link instance
    fn get_link_id(&self) -> String;

    /// Gets the type of this link instance
    fn get_link_type(&self) -> LinkType;

    /// Gets the properties of this link instance
    fn get_link_properties(&self) -> LinkProperties;

    /// Creates a new connection on this link
    ///
    /// If this is a new receive link and there were previous receive links, this starts the monitor thread.
    ///
    /// # Arguments
    ///
    /// * `link_type` - The type of link to open (send, recv, bidi)
    /// * `connection_id` - The ID to give the new connection
    /// * `link_hints` - Hints, in the form of JSON, to be used for opening the link connection
    fn open_connection(
        &mut self,
        link_type: LinkType,
        connection_id: &str,
        link_hints: &str,
    ) -> Connection;

    /// Closes and removes the identified link connection
    ///
    /// If there are no remaining open receive connections, this stops the monitor thread.
    ///
    /// # Arguments
    ///
    /// * `connection_id` - The ID of the connection to close
    fn close_connection(&mut self, connection_id: &str);

    /// Sends a package over this link
    ///
    /// # Arguments
    ///
    /// * `handle` - The handle with which to issue status callbacks
    /// * `connection_id` - The ID of the sending connection
    /// * `package` - Raw encrypted package contents
    fn send_package(
        &mut self,
        handle: u64,
        connection_id: &str,
        package: &[u8],
    ) -> Result<(), io::Error>;
}

/// Link configuration, as represented by JSON
#[derive(Deserialize, Clone, Debug)]
pub struct LinkConfig {
    /// List of personas that use this link
    #[serde(rename = "utilizedBy")]
    pub utilized_by: Vec<String>,

    /// List of personas to which this link allows communication
    #[serde(rename = "connectedTo")]
    pub connected_to: Vec<String>,

    /// Link-specific connection profile
    pub profile: String,

    /// Link properties
    #[serde(default)]
    pub properties: serde_json::Map<String, serde_json::Value>,
}

impl LinkConfig {
    /// Gets the list of personas to which this link allows communication
    pub fn get_connected_to(&self) -> Vec<&str> {
        let mut connected_to = Vec::new();
        for persona in &self.connected_to {
            connected_to.push(persona.as_str())
        }
        connected_to
    }

    /// Gets the link properties
    pub fn get_link_properties(&self) -> LinkProperties {
        ConfigLoader::parse_link_properties(&self.properties)
    }
}

/// A parser/factory for a particular type of link
pub trait LinkProfileParser {
    /// Creates a link instance
    ///
    /// # Arguments
    ///
    /// * `sdk` - Reference to SDK
    fn create_link(
        &self,
        sdk: Arc<Mutex<dyn IRaceSdkComms + Send>>,
        channel_gid: &str,
    ) -> Option<Arc<Mutex<dyn Link>>>;
}
