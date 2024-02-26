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

use shims::race_common::link_properties::ConnectionType;
///
/// The link_properties_c module defines a C compatible version of
/// `shims::race_common::link_properties::LinkProperties` for passing over the FFI (foreign
/// function interface). This struct can be thought of as a bridge between Rust and C. The native
/// Rust struct `LinkProperties` can be converted to `LinkPropertiesC` which is C compatible. This
/// must be done since `LinkPropertiesC` has members of dynamic size that are unsupported in native
/// C.
use shims::race_common::link_properties::LinkProperties;
use shims::race_common::link_properties::LinkPropertyPair;
use shims::race_common::link_properties::LinkType;
use shims::race_common::link_properties::TransmissionType;
use shims::race_common::send_type::SendType;
use std::ffi::c_void;
use std::ffi::CString;
use std::os::raw::c_char;

extern "C" {
    fn create_link_properties() -> LinkPropertiesC;

    fn destroy_link_properties(props: *mut LinkPropertiesC);

    fn add_supported_hint_to_link_properties(props: *mut LinkPropertiesC, hint: *const c_char);

    fn set_channel_gid_for_link_properties(props: *mut LinkPropertiesC, channel_gid: *const c_char);

    fn set_link_address_for_link_properties(
        props: *mut LinkPropertiesC,
        link_address: *const c_char,
    );
}

/// Need to use repr(C) since this struct is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Clone)]
pub struct LinkPropertiesC {
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
    /// Opaque pointer to a C/C++ object that contains supported hints. This pointer should not be
    /// modified directly, but only through the provided helper functions.
    pub supported_hints: *mut c_void,
    pub channel_gid: *mut c_void,
    pub link_address: *mut c_void,
}

impl LinkPropertiesC {
    pub fn new(link_properties_rust: &LinkProperties) -> LinkPropertiesC {
        unsafe {
            let mut props = create_link_properties();
            props.copy_link_properties(link_properties_rust);
            return props;
        }
    }

    pub fn copy_link_properties(&mut self, link_properties_rust: &LinkProperties) {
        self.link_type = link_properties_rust.link_type;
        self.transmission_type = link_properties_rust.transmission_type;
        self.connection_type = link_properties_rust.connection_type;
        self.send_type = link_properties_rust.send_type;
        self.reliable = link_properties_rust.reliable;
        self.is_flushable = link_properties_rust.is_flushable;
        self.duration_s = link_properties_rust.duration_s;
        self.period_s = link_properties_rust.period_s;
        self.mtu = link_properties_rust.mtu;
        self.worst = link_properties_rust.worst;
        self.best = link_properties_rust.best;
        self.expected = link_properties_rust.expected;
        for hint in &link_properties_rust.supported_hints {
            unsafe {
                let hint = CString::new(hint.clone()).expect("CString::new failed");
                add_supported_hint_to_link_properties(self, hint.as_ptr());
            }
        }
        unsafe {
            let channel_gid = CString::new(link_properties_rust.channel_gid.clone())
                .expect("CString::new failed");
            set_channel_gid_for_link_properties(self, channel_gid.as_ptr());

            let link_address = CString::new(link_properties_rust.link_address.clone())
                .expect("CString::new failed");
            set_link_address_for_link_properties(self, link_address.as_ptr());
        }
    }
}

impl Default for LinkPropertiesC {
    fn default() -> LinkPropertiesC {
        LinkPropertiesC {
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
            supported_hints: std::ptr::null_mut(),
            channel_gid: std::ptr::null_mut(),
            link_address: std::ptr::null_mut(),
        }
    }
}

impl Drop for LinkPropertiesC {
    fn drop(&mut self) {
        unsafe {
            destroy_link_properties(self);
        }
    }
}
