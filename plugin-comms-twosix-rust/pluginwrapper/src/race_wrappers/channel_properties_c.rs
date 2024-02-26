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

use shims::race_common::channel_properties::ChannelProperties;
use shims::race_common::channel_properties::LinkDirection;
use shims::race_common::channel_role::ChannelRole;
use shims::race_common::channel_role::LinkSide;
use shims::race_common::channel_status::ChannelStatus;
use shims::race_common::link_properties::ConnectionType;
use shims::race_common::link_properties::LinkPropertyPair;
use shims::race_common::link_properties::TransmissionType;
use shims::race_common::send_type::SendType;
use std::ffi::c_void;
use std::ffi::CStr;
use std::ffi::CString;
use std::os::raw::c_char;
use std::slice;

extern "C" {
    fn create_channel_role(
        role_name: *const c_char,
        mechanical_tags: *const *const c_char,
        mechanical_tags_len: usize,
        behavioral_tags: *const *const c_char,
        behavioral_tags_len: usize,
        link_side: LinkSide,
    ) -> ChannelRoleC;

    fn destroy_channel_role(props: *mut ChannelRoleC);

    fn create_channel_properties() -> ChannelPropertiesC;

    fn destroy_channel_properties(props: *mut ChannelPropertiesC);

    fn add_supported_hint_to_channel_properties(
        props: *mut ChannelPropertiesC,
        hint: *const c_char,
    );

    fn set_channel_gid_for_channel_properties(
        props: *mut ChannelPropertiesC,
        channel_gid: *const c_char,
    );

    fn resize_roles_for_channel_properties(props: *mut ChannelPropertiesC, size: usize) -> c_void;

    fn get_supported_hints_for_channel_properties(
        props: *mut ChannelPropertiesC,
        vector_length: *mut usize,
    ) -> *mut *mut c_char;

    fn get_channel_gid_for_channel_properties(props: *mut ChannelPropertiesC) -> *mut c_char;

    fn sdk_delete_string_array(cstring: *mut *mut c_char, data_length: usize) -> c_void;
}

/// Need to use repr(C) since this struct is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Clone)]
pub struct ChannelRoleC {
    pub role_name: *mut c_char,
    pub mechanical_tags: *mut *mut c_char,
    pub mechanical_tags_len: usize,
    pub behavioral_tags: *mut *mut c_char,
    pub behavioral_tags_len: usize,
    pub link_side: LinkSide,
}

impl Default for ChannelRoleC {
    fn default() -> Self {
        ChannelRoleC {
            role_name: std::ptr::null_mut(),
            mechanical_tags: std::ptr::null_mut(),
            mechanical_tags_len: 0,
            behavioral_tags: std::ptr::null_mut(),
            behavioral_tags_len: 0,
            link_side: LinkSide::LsUndef,
        }
    }
}

impl From<&ChannelRole> for ChannelRoleC {
    fn from(channel_role_rust: &ChannelRole) -> ChannelRoleC {
        unsafe {
            let role_name_c =
                CString::new(channel_role_rust.role_name.clone()).expect("CString::new failed");
            let mechanical_tags: Vec<_> = channel_role_rust
                .mechanical_tags
                .iter()
                .map(|x| CString::new(x.clone()).expect("CString::new failed"))
                .collect();
            let mechanical_tags_ptr = mechanical_tags
                .iter()
                .map(|x| x.as_ptr())
                .collect::<Vec<_>>();
            let behavioral_tags: Vec<_> = channel_role_rust
                .behavioral_tags
                .iter()
                .map(|x| CString::new(x.clone()).expect("CString::new failed"))
                .collect();
            let behavioral_tags_ptr = behavioral_tags
                .iter()
                .map(|x| x.as_ptr())
                .collect::<Vec<_>>();
            create_channel_role(
                role_name_c.as_ptr(),
                mechanical_tags_ptr.as_ptr(),
                mechanical_tags.len(),
                behavioral_tags_ptr.as_ptr(),
                behavioral_tags.len(),
                channel_role_rust.link_side,
            )
        }
    }
}

impl From<&ChannelRoleC> for ChannelRole {
    fn from(channel_role_c: &ChannelRoleC) -> ChannelRole {
        unsafe {
            ChannelRole {
                role_name: String::from(
                    CStr::from_ptr(channel_role_c.role_name)
                        .to_str()
                        .expect("role_name to String failed"),
                ),
                mechanical_tags: slice::from_raw_parts(
                    channel_role_c.mechanical_tags,
                    channel_role_c.mechanical_tags_len,
                )
                .iter()
                .map(|x| {
                    String::from(
                        CStr::from_ptr(*x)
                            .to_str()
                            .expect("role_name to String failed"),
                    )
                })
                .collect(),
                behavioral_tags: slice::from_raw_parts(
                    channel_role_c.behavioral_tags,
                    channel_role_c.behavioral_tags_len,
                )
                .iter()
                .map(|x| {
                    String::from(
                        CStr::from_ptr(*x)
                            .to_str()
                            .expect("role_name to String failed"),
                    )
                })
                .collect(),
                link_side: channel_role_c.link_side,
            }
        }
    }
}

impl Drop for ChannelRoleC {
    fn drop(&mut self) {
        unsafe {
            destroy_channel_role(self);
        }
    }
}

/// Need to use repr(C) since this struct is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Clone)]
pub struct ChannelPropertiesC {
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
    /// Opaque pointer to a C/C++ object that contains supported hints. This pointer should not be
    /// modified directly, but only through the provided helper functions.
    pub supported_hints: *mut c_void,
    pub max_links: i32,
    pub creators_per_loader: i32,
    pub loaders_per_creator: i32,
    pub roles: *mut ChannelRoleC,
    pub roles_len: usize,
    pub current_role: ChannelRoleC,
    pub max_sends_per_interval: i32,
    pub seconds_per_interval: i32,
    pub interval_end_time: u64,
    pub sends_remaining_in_interval: i32,
    pub channel_gid: *mut c_void,
}

impl ChannelPropertiesC {
    pub fn new(channel_properties_rust: &ChannelProperties) -> ChannelPropertiesC {
        unsafe {
            let mut props = create_channel_properties();
            props.copy_channel_properties(channel_properties_rust);
            return props;
        }
    }

    pub fn copy_channel_properties(&mut self, channel_properties_rust: &ChannelProperties) {
        self.channel_status = channel_properties_rust.channel_status;
        self.link_direction = channel_properties_rust.link_direction;
        self.transmission_type = channel_properties_rust.transmission_type;
        self.connection_type = channel_properties_rust.connection_type;
        self.send_type = channel_properties_rust.send_type;
        self.multi_addressable = channel_properties_rust.multi_addressable;
        self.reliable = channel_properties_rust.reliable;
        self.bootstrap = channel_properties_rust.bootstrap;
        self.is_flushable = channel_properties_rust.is_flushable;
        self.duration_s = channel_properties_rust.duration_s;
        self.period_s = channel_properties_rust.period_s;
        self.mtu = channel_properties_rust.mtu;
        self.creator_expected = channel_properties_rust.creator_expected;
        self.loader_expected = channel_properties_rust.loader_expected;
        self.max_links = channel_properties_rust.max_links;
        self.creators_per_loader = channel_properties_rust.creators_per_loader;
        self.loaders_per_creator = channel_properties_rust.loaders_per_creator;

        self.current_role = ChannelRoleC::from(&channel_properties_rust.current_role);

        self.max_sends_per_interval = channel_properties_rust.max_sends_per_interval;
        self.seconds_per_interval = channel_properties_rust.seconds_per_interval;
        self.interval_end_time = channel_properties_rust.interval_end_time;
        self.sends_remaining_in_interval = channel_properties_rust.sends_remaining_in_interval;

        for hint in &channel_properties_rust.supported_hints {
            unsafe {
                let hint = CString::new(hint.clone()).expect("CString::new failed");
                add_supported_hint_to_channel_properties(self, hint.as_ptr());
            }
        }

        unsafe {
            let channel_gid = CString::new(channel_properties_rust.channel_gid.clone())
                .expect("CString::new failed");
            set_channel_gid_for_channel_properties(self, channel_gid.as_ptr());
        }

        unsafe {
            resize_roles_for_channel_properties(self, channel_properties_rust.roles.len());
            for (role_rust, role_c) in channel_properties_rust
                .roles
                .iter()
                .zip(slice::from_raw_parts_mut(self.roles, self.roles_len).iter_mut())
            {
                *role_c = ChannelRoleC::from(role_rust);
            }
        }
    }

    pub fn to_rust_channel_properties(&mut self) -> ChannelProperties {
        let mut channel_properties_rust: ChannelProperties = ChannelProperties::default();

        channel_properties_rust.channel_status = self.channel_status;
        channel_properties_rust.link_direction = self.link_direction;
        channel_properties_rust.transmission_type = self.transmission_type;
        channel_properties_rust.connection_type = self.connection_type;
        channel_properties_rust.send_type = self.send_type;
        channel_properties_rust.multi_addressable = self.multi_addressable;
        channel_properties_rust.reliable = self.reliable;
        channel_properties_rust.bootstrap = self.bootstrap;
        channel_properties_rust.is_flushable = self.is_flushable;
        channel_properties_rust.duration_s = self.duration_s;
        channel_properties_rust.period_s = self.period_s;
        channel_properties_rust.mtu = self.mtu;
        channel_properties_rust.creator_expected = self.creator_expected;
        channel_properties_rust.loader_expected = self.loader_expected;
        channel_properties_rust.max_links = self.max_links;
        channel_properties_rust.creators_per_loader = self.creators_per_loader;
        channel_properties_rust.loaders_per_creator = self.loaders_per_creator;
        channel_properties_rust.current_role = ChannelRole::from(&self.current_role);

        channel_properties_rust.max_sends_per_interval = self.max_sends_per_interval;
        channel_properties_rust.seconds_per_interval = self.seconds_per_interval;
        channel_properties_rust.interval_end_time = self.interval_end_time;
        channel_properties_rust.sends_remaining_in_interval = self.sends_remaining_in_interval;

        unsafe {
            let mut vector_length = 0;
            let hints = get_supported_hints_for_channel_properties(self, &mut vector_length);
            defer! {{
                sdk_delete_string_array(hints, vector_length);
            }};

            let mut result: Vec<String> = vec![];
            let vector_length = vector_length as isize;
            for index in 0..vector_length {
                let dir = String::from(
                    CStr::from_ptr(*hints.offset(index))
                        .to_str()
                        .expect("list_dir failed"),
                );
                result.push(dir);
            }
            channel_properties_rust.supported_hints = result
        }

        unsafe {
            channel_properties_rust.channel_gid = String::from(
                CStr::from_ptr(get_channel_gid_for_channel_properties(self))
                    .to_str()
                    .expect("list_dir failed"),
            );
        }

        unsafe {
            channel_properties_rust.roles = slice::from_raw_parts(self.roles, self.roles_len)
                .iter()
                .map(|x| ChannelRole::from(x))
                .collect()
        }
        channel_properties_rust
    }
}

impl Default for ChannelPropertiesC {
    fn default() -> ChannelPropertiesC {
        ChannelPropertiesC {
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
            supported_hints: std::ptr::null_mut(),
            max_links: -1,
            creators_per_loader: -1,
            loaders_per_creator: -1,
            roles: std::ptr::null_mut(),
            roles_len: 0,
            current_role: ChannelRoleC::default(),
            max_sends_per_interval: -1,
            seconds_per_interval: -1,
            interval_end_time: 0,
            sends_remaining_in_interval: -1,
            channel_gid: std::ptr::null_mut(),
        }
    }
}

impl Drop for ChannelPropertiesC {
    fn drop(&mut self) {
        unsafe {
            destroy_channel_properties(self);
        }
    }
}
