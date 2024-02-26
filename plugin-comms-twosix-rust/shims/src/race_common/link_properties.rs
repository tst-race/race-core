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
/// Rust equivalent of the C++ LinkProperties class.
/// For reference, the C++ source can be found here: racesdk/common/include/LinkProperties.h
///
use super::send_type::SendType;

/// Need to use repr(C) since this enum is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub enum LinkType {
    LtUndef = 0,
    LtSend = 1,
    LtRecv = 2,
    LtBidi = 3,
}

/// Need to use repr(C) since this enum is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub enum TransmissionType {
    TtUndef = 0,
    TtUnicast = 1,
    TtMulticast = 2,
}

/// Need to use repr(C) since this enum is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub enum ConnectionType {
    CtUndef = 0,
    CtDirect = 1,
    CtIndirect = 2,
}

/// Need to use repr(C) since this struct is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct LinkPropertySet {
    pub bandwidth_bps: i32,
    pub latency_ms: i32,
    pub loss: f32,
}

impl Default for LinkPropertySet {
    fn default() -> LinkPropertySet {
        LinkPropertySet {
            bandwidth_bps: -1,
            latency_ms: -1,
            loss: -1.0,
        }
    }
}

/// Need to use repr(C) since this struct is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct LinkPropertyPair {
    pub send: LinkPropertySet,
    pub receive: LinkPropertySet,
}

impl Default for LinkPropertyPair {
    fn default() -> LinkPropertyPair {
        LinkPropertyPair {
            send: LinkPropertySet::default(),
            receive: LinkPropertySet::default(),
        }
    }
}

/// Do NOT need to use repr(C) since this struct uses dynamic memory (Vec<String>) and can't be
/// passed over FFI into C.
#[derive(Debug, Clone)]
pub struct LinkProperties {
    pub link_type: LinkType,
    pub transmission_type: TransmissionType,
    pub connection_type: ConnectionType,
    pub send_type: SendType,
    pub reliable: bool,
    pub is_flushable: bool,
    pub duration_s: i32,
    pub period_s: i32,
    pub mtu: i32,
    pub worst: LinkPropertyPair,
    pub best: LinkPropertyPair,
    pub expected: LinkPropertyPair,
    pub supported_hints: Vec<String>,
    pub channel_gid: String,
    pub link_address: String,
}

impl LinkProperties {
    pub fn new(
        link_type: LinkType,
        transmission_type: TransmissionType,
        connection_type: ConnectionType,
        send_type: SendType,
        reliable: bool,
        is_flushable: bool,
        duration_s: i32,
        period_s: i32,
        mtu: i32,
        worst: LinkPropertyPair,
        best: LinkPropertyPair,
        expected: LinkPropertyPair,
        supported_hints: Vec<String>,
        channel_gid: String,
        link_address: String,
    ) -> LinkProperties {
        LinkProperties {
            link_type: link_type,
            transmission_type: transmission_type,
            connection_type: connection_type,
            send_type: send_type,
            reliable: reliable,
            is_flushable: is_flushable,
            duration_s: duration_s,
            period_s: period_s,
            mtu: mtu,
            worst: worst,
            best: best,
            expected: expected,
            supported_hints: supported_hints,
            channel_gid: channel_gid,
            link_address: link_address,
        }
    }
}

impl Default for LinkProperties {
    fn default() -> LinkProperties {
        LinkProperties {
            link_type: LinkType::LtUndef,
            transmission_type: TransmissionType::TtUndef,
            connection_type: ConnectionType::CtUndef,
            send_type: SendType::StUndef,
            reliable: false,
            is_flushable: false,
            duration_s: -1,
            period_s: -1,
            mtu: -1,
            worst: LinkPropertyPair::default(),
            best: LinkPropertyPair::default(),
            expected: LinkPropertyPair::default(),
            supported_hints: Vec::new(),
            channel_gid: String::new(),
            link_address: String::new(),
        }
    }
}
