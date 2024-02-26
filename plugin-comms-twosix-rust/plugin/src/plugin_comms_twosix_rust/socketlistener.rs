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
use std::io::Read;
use std::net::{Shutdown, TcpListener, TcpStream};
use std::sync::mpsc;
use std::thread;
use std::time;

#[derive(Debug)]
pub struct Listener {}

impl Listener {
    /// Purpose:
    ///     Handle a Connection from a client sending a message
    /// Args:
    ///     TcpStream: A TCP Stream object to handle
    /// Returns:
    ///     N/A
    fn handle_connection(mut stream: TcpStream) -> Vec<u8> {
        let mut buffer = [0_u8; 4096]; // using 4096 byte buffer
        let mut data: Vec<u8> = Vec::new();
        loop {
            match stream.read(&mut buffer) {
                Ok(n) => {
                    if n <= 0 {
                        stream.shutdown(Shutdown::Both).unwrap();
                        return data;
                    } else {
                        data.extend_from_slice(&buffer[..n]);
                    }
                }
                Err(_) => {
                    log_error!(
                        "An error occurred, terminating connection with {}",
                        stream.peer_addr().unwrap()
                    );
                    stream.shutdown(Shutdown::Both).unwrap();
                    break;
                }
            }
        }

        return b"ERROR!!!!!!!".to_vec();
    }

    pub fn start_server(
        &self,
        complete_channel_rx: mpsc::Receiver<bool>,
        hostname: String,
        port: String,
        mut message_callback: Box<dyn FnMut(Vec<u8>)>,
    ) {
        let hostname_port = format!("{}:{}", hostname, port);
        log_info!("Server listening");
        log_debug!("    hostname:port: {}", hostname_port);

        // accept connections and process them, spawning a new thread for each one
        let listener = TcpListener::bind(&hostname_port).unwrap();
        listener
            .set_nonblocking(true)
            .expect("Cannot set non-blocking");

        // Handle incoming connections to the listener
        for stream in listener.incoming() {
            match stream {
                Ok(stream) => {
                    let connection_id = stream.peer_addr().unwrap();
                    log_info!("New connection: {}", connection_id);

                    // thread::Builder::new()
                    //     .name(connection_id.to_string())
                    //     .spawn(move || {
                    //         let message = Listener::handle_connection(stream);
                    //         tx.send(message).unwrap();
                    //     })
                    //     .unwrap();
                    // log_debug!("Connection Complete: {}", connection_id);

                    log_debug!("waiting for message...");
                    let message = Listener::handle_connection(stream);
                    log_info!("received message, len = {}", message.len());
                    message_callback(message);
                }
                Err(_err) => {
                    // log_error!("Error Listening to Incoming Connections: {}", err);
                }
            }

            // Check for stop message from the main thread
            if !complete_channel_rx.try_recv().is_err() {
                log_debug!("server thread shutting down");
                break;
            }

            // Since the socket listener is opened in non-blocking mode (so that we can
            // control shut down of the listener), the for-loop will repeatedly execute
            // very rapidly when there is no incoming connection. This results in a very
            // high CPU usage for this process (almost 100%). By sleeping just a little,
            // we yield to the CPU scheduler and usage drops significantly (<5%).
            thread::sleep(time::Duration::from_millis(10));
        }

        // close the socket server
        log_info!("Closing the Listener on {}:{}", hostname, port);
        drop(listener);
    }
}
