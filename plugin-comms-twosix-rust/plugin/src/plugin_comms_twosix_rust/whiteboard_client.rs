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

use restson::{RestClient, RestPath};
use serde::{Deserialize, Serialize};

/// New message payload
#[derive(Serialize)]
struct NewMessage {
    data: String,
}

impl RestPath<&String> for NewMessage {
    fn get_path(hashtag: &String) -> Result<String, restson::Error> {
        Ok(format!("post/{}", hashtag))
    }
}

/// Latest index payload
#[derive(Deserialize)]
struct LatestIndex {
    latest: i32,
}

impl RestPath<&String> for LatestIndex {
    fn get_path(hashtag: &String) -> Result<String, restson::Error> {
        Ok(format!("latest/{}", hashtag))
    }
}

/// Messages payload
#[derive(Deserialize)]
pub struct Messages {
    pub data: Vec<String>,
    pub length: i32,
}

impl RestPath<(&String, i32)> for Messages {
    fn get_path((hashtag, latest): (&String, i32)) -> Result<String, restson::Error> {
        Ok(format!("get/{}/{}/-1", hashtag, latest))
    }
}

/// REST client to a whiteboard service
pub struct WhiteboardClient {
    client: RestClient,
    hashtag: String,
}

impl WhiteboardClient {
    /// Creates a new client to a whiteboard service at a particlar URL and with
    /// a particular hashtag
    ///
    /// # Arguments
    ///
    /// * `hostname` - Hostname or IP address of the whiteboard service
    /// * `port` - Port of the whiteboard service
    /// * `hashtag` - Hashtag to be used for all requests
    pub fn new(hostname: String, port: u32, hashtag: String) -> WhiteboardClient {
        Self {
            client: RestClient::new(&format!("http://{}:{}", hostname, port)).unwrap(),
            hashtag,
        }
    }

    /// Fetches the index of the last post at the whiteboard service
    pub fn get_last_post_index(&mut self) -> i32 {
        match self.client.get::<&String, LatestIndex>(&self.hashtag) {
            Ok(response) => response.latest,
            Err(error) => {
                log_error!("Unable to get last post index: {:?}", error);
                0
            }
        }
    }

    /// Fetches the posts after the given message index
    ///
    /// # Arguments
    ///
    /// * `oldest` - Message index of the oldest message to retrieve
    pub fn get_new_posts(&mut self, oldest: i32) -> Messages {
        match self
            .client
            .get::<(&String, i32), Messages>((&self.hashtag, oldest))
        {
            Ok(response) => response,
            Err(error) => {
                log_error!("Unable to get new posts: {:?}", error);
                Messages {
                    data: vec![],
                    length: 0,
                }
            }
        }
    }

    /// Posts the given message to the whiteboard service
    ///
    /// # Arguments
    ///
    /// * `data` - Message to be posted
    pub fn post_message(&mut self, data: &String) -> Result<(), io::Error> {
        let payload = NewMessage { data: data.clone() };
        match self.client.post(&self.hashtag, &payload) {
            Ok(_) => Ok(()),
            Err(error) => {
                log_error!("Unable to post message: {:?}", error);
                Err(io::Error::new(io::ErrorKind::Other, "Failed POST"))
            }
        }
    }
}
