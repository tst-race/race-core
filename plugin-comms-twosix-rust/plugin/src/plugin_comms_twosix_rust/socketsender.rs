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

// Imports
use std::io::{Error, Write};
use std::net::{Shutdown, TcpStream};

pub type SendError = Error;

/// Sender ...
#[derive(Debug)]
pub struct Sender {}

impl Sender {
    /// Purpose:
    ///     Start a client side of a connection (sending messages to
    ///     and already started server)
    /// Args:
    ///     hostname (String): Hostname to connect to
    ///     port (String): Port to connect to
    ///     msg (Vec<u8>): data to send to the server
    /// Returns:
    ///     N/A
    pub fn start_client(
        &self,
        hostname: &String,
        port: &String,
        msg: &Vec<u8>,
    ) -> Result<(), SendError> {
        let hostname_port = format!("{}:{}", hostname, port);
        log_info!("Starting Client");
        log_debug!("    hostname & port: {}", hostname_port);

        defer! {{
            log_debug!("Client Terminated.");
        }}

        match TcpStream::connect(&hostname_port) {
            Ok(mut stream) => {
                log_debug!("Successfully connected to server {}", hostname_port);

                stream.write(msg).unwrap();
                stream.shutdown(Shutdown::Both).unwrap();
                log_debug!("Sent message");
                return Ok(());
            }

            Err(err) => {
                log_error!("Failed to connect to {}: {}", hostname_port, err);
                return Err(err);
            }
        }
    }
}
