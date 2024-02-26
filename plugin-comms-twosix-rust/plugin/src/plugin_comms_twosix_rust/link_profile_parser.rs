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

use serde::Deserialize;

use super::direct_link::DirectLinkParser;
use super::link::{LinkConfig, LinkProfileParser};
use super::whiteboard_link::WhiteboardLinkParser;

const TWO_SIX_WHITEBOARD_SERVICE_NAME: &str = "twosix-whiteboard";

/// Returns the default is-multicast value for links
fn default_multicast() -> bool {
    false
}

/// Common link profile parameters
#[derive(Deserialize)]
struct LinkProfile {
    #[serde(default = "default_multicast")]
    multicast: bool,
    #[serde(default)]
    service_name: String,
}

/// Errors that could have occured during the parsing of profile configuration
pub enum ParseError {
    /// JSON parsing error
    JsonError(serde_json::Error),
    /// Link type is not recognized or supported
    NotRecognized,
    /// Link is not utilized by the current active persona
    NotUtilized,
    /// Incorrect channel type (e.g., unicast link with indirect channel)
    IncorrectChannelType,
}

impl From<serde_json::Error> for ParseError {
    fn from(error: serde_json::Error) -> ParseError {
        log_warning!("Unable to parse link config: {}", error);
        ParseError::JsonError(error)
    }
}

/// Parses the given link configuration and creates a parser/factory according to the
/// described link
///
/// # Arguments
///
/// * `link_config` - Link configuration from JSON
/// * `active_persona` - Current active persona
/// * `direct` - Expect direct links from link configuration
pub fn parse(
    link_config: &serde_json::Value,
    active_persona: &String,
    direct: bool,
) -> Result<Box<dyn LinkProfileParser>, ParseError> {
    log_debug!("Parsing link: {}", link_config);

    let config: LinkConfig = serde_json::from_value(link_config.clone())?;
    if !config.utilized_by.contains(active_persona) {
        log_debug!("Link is not utilized by the active persona");
        return Err(ParseError::NotUtilized);
    }

    log_debug!("Link is utilized by the active persona");

    let profile: LinkProfile = serde_json::from_str(&config.profile)?;

    if !profile.multicast {
        if direct {
            log_debug!("Creating unicast link");
            return Ok(DirectLinkParser::new(&config));
        }
        log_error!("Unicast link configuration found in indirect channel config");
        return Err(ParseError::IncorrectChannelType);
    } else if profile.service_name == TWO_SIX_WHITEBOARD_SERVICE_NAME {
        if !direct {
            log_debug!("Creating multicast link");
            return Ok(WhiteboardLinkParser::new(&config));
        }
        log_error!("Multicast link configuration found in direct channel config");
        return Err(ParseError::IncorrectChannelType);
    }

    log_error!("Unknown service: {}", profile.service_name);
    Err(ParseError::NotRecognized)
}
