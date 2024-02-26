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

use super::channel_role::ChannelRole;
use super::channel_status::ChannelStatus;
use super::link_properties::ConnectionType;
use super::link_properties::LinkPropertyPair;
///
/// Rust equivalent of the C++ ChannelProperties class.
/// For reference, the C++ source can be found here: racesdk/common/include/ChannelProperties.h
///
///
use super::link_properties::TransmissionType;
use super::send_type::SendType;

/// Need to use repr(C) since this enum is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub enum LinkDirection {
    LdUndef = 0,
    LdCreatorToLoader = 1,
    LdLoaderToCreator = 2,
    LdBidi = 3,
}

pub struct ChannelProperties {
    pub channel_status: ChannelStatus,
    pub link_direction: LinkDirection,
    pub transmission_type: TransmissionType,
    pub connection_type: ConnectionType,
    pub send_type: SendType,
    pub multi_addressable: bool,
    pub reliable: bool,
    pub bootstrap: bool,
    pub is_flushable: bool,
    pub duration_s: i32,
    pub period_s: i32,
    pub mtu: i32,
    pub creator_expected: LinkPropertyPair,
    pub loader_expected: LinkPropertyPair,
    pub supported_hints: Vec<String>,
    pub max_links: i32,
    pub creators_per_loader: i32,
    pub loaders_per_creator: i32,
    pub roles: Vec<ChannelRole>,
    pub current_role: ChannelRole,
    pub max_sends_per_interval: i32,
    pub seconds_per_interval: i32,
    pub interval_end_time: u64,
    pub sends_remaining_in_interval: i32,
    pub channel_gid: String,
}

impl Default for ChannelProperties {
    fn default() -> ChannelProperties {
        ChannelProperties {
            channel_status: ChannelStatus::ChannelUndef,
            link_direction: LinkDirection::LdUndef,
            transmission_type: TransmissionType::TtUndef,
            connection_type: ConnectionType::CtUndef,
            send_type: SendType::StUndef,
            multi_addressable: false,
            reliable: false,
            bootstrap: false,
            is_flushable: false,
            duration_s: -1,
            period_s: -1,
            mtu: -1,
            creator_expected: LinkPropertyPair::default(),
            loader_expected: LinkPropertyPair::default(),
            supported_hints: Vec::new(),
            max_links: -1,
            creators_per_loader: -1,
            loaders_per_creator: -1,
            roles: Vec::new(),
            current_role: ChannelRole::default(),
            max_sends_per_interval: -1,
            seconds_per_interval: -1,
            interval_end_time: 0,
            sends_remaining_in_interval: -1,
            channel_gid: String::new(),
        }
    }
}
