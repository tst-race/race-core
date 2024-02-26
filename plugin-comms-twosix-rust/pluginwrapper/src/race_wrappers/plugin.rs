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

extern crate shims;
use shims::race_common::i_race_plugin_comms::IRacePluginComms;
use shims::race_common::link_properties::LinkType;
use shims::race_common::plugin_config::PluginConfig;
use shims::race_common::plugin_response::PluginResponse;
use shims::race_common::race_log::RaceLog;
use std::ffi::c_void;
use std::ffi::CStr;
use std::os::raw::c_char;
use std::slice;
extern crate libc;

// Converts input from a C string (const char *) to a rust string (String).
// arg_name is a string used in logging to uniquely identify what is being
// converted in case there is an error in the conversion.
fn c_char_to_string(input: *const c_char, arg_name: &str) -> Option<String> {
    let input = unsafe { CStr::from_ptr(input) };
    match input.to_str() {
        Ok(output) => {
            return Some(String::from(output));
        }
        Err(_) => {
            RaceLog::log_error(
                "Rust Wrapper",
                &format!(
                    "c_char_to_string failed to convert {} to Rust string",
                    arg_name
                ),
                "",
            );
            return None;
        }
    }
}

pub struct PluginWrapper {
    pub plugin: Box<dyn IRacePluginComms>,
}

#[no_mangle]
pub extern "C" fn plugin_init(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    etc_directory: *const c_char,
    logging_directory: *const c_char,
    aux_data_directory: *const c_char,
    tmp_directory: *const c_char,
    plugin_directory: *const c_char,
) {
    if plugin.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "plugin pointer pass to plugin_init is NULL!",
            "",
        );
        return;
    }
    if response.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "response pointer pass to plugin_init is NULL!",
            "",
        );
        return;
    }

    let plugin_config = PluginConfig::new(
        &c_char_to_string(etc_directory, "etc_directory").unwrap(),
        &c_char_to_string(logging_directory, "logging_directory").unwrap(),
        &c_char_to_string(aux_data_directory, "aux_data_directory").unwrap(),
        &c_char_to_string(tmp_directory, "tmp_directory").unwrap(),
        &c_char_to_string(plugin_directory, "plugin_directory").unwrap(),
    );

    unsafe {
        let plugin = &mut (*plugin).plugin;
        *response = plugin.init(&plugin_config);
    }
}

#[no_mangle]
pub extern "C" fn plugin_shutdown(plugin: *mut PluginWrapper, response: *mut PluginResponse) {
    if plugin.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "plugin pointer pass to plugin_shutdown is NULL!",
            "",
        );
        return;
    }
    if response.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "response pointer pass to plugin_shutdown is NULL!",
            "",
        );
        return;
    }

    unsafe {
        let plugin = &mut (*plugin).plugin;
        *response = plugin.shutdown();
    }
}

#[no_mangle]
pub extern "C" fn plugin_send_package(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    handle: u64,
    connection_id: *const c_char,
    cipher_text: *const c_void,
    cipher_text_size: usize,
    timeout_timestamp: f64,
    batch_id: u64,
) {
    if plugin.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "plugin pointer pass to plugin_send_package is NULL!",
            "",
        );
        return;
    }
    if response.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "response pointer pass to plugin_send_package is NULL!",
            "",
        );
        return;
    }
    if connection_id.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "connection_id pointer pass to plugin_send_package is NULL!",
            "",
        );
        return;
    }
    if cipher_text.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "cipher_text pointer pass to plugin_send_package is NULL!",
            "",
        );
        return;
    }

    let connection_id = c_char_to_string(connection_id, "connection_id (send_package)").unwrap();
    unsafe {
        let ctext = slice::from_raw_parts(cipher_text as *const u8, cipher_text_size).to_vec();
        let plugin = &mut (*plugin).plugin;
        *response =
            plugin.send_package(handle, &connection_id, &ctext, timeout_timestamp, batch_id);
    }
}

#[no_mangle]
pub extern "C" fn plugin_open_connection(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    handle: u64,
    link_type: LinkType,
    link_id: *const c_char,
    link_hints: *const c_char,
    send_timeout: i32,
) {
    if plugin.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "plugin pointer pass to plugin_open_connection is NULL!",
            "",
        );
        return;
    }
    if response.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "response pointer pass to plugin_open_connection is NULL!",
            "",
        );
        return;
    }
    if link_id.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "link_id pointer pass to plugin_open_connection is NULL!",
            "",
        );
        return;
    }
    if link_hints.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "link_hints pointer pass to plugin_open_connection is NULL!",
            "",
        );
        return;
    }

    let link_id = c_char_to_string(link_id, "link_id (open_connection)").unwrap();
    let link_hints = c_char_to_string(link_hints, "link_hints (open_connection)").unwrap();
    unsafe {
        let plugin = &mut (*plugin).plugin;
        *response = plugin.open_connection(handle, link_type, &link_id, &link_hints, send_timeout);
    }
}

// virtual PluginResponse destroyLink(RaceHandle handle, LinkID linkId) = 0;
#[no_mangle]
pub extern "C" fn plugin_destroy_link(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    handle: u64,
    link_id: *const c_char,
) {
    if link_id.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "link_id pointer pass to plugin_destroy_link is NULL!",
            "",
        );
        return;
    }

    let link_id = c_char_to_string(link_id, "link_id (plugin_destroy_link)").unwrap();
    unsafe {
        let plugin = &mut (*plugin).plugin;
        *response = plugin.destroy_link(handle, &link_id);
    }
}

#[no_mangle]
pub extern "C" fn plugin_create_link(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    handle: u64,
    channel_gid: *const c_char,
) {
    if channel_gid.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "channel_gid pointer pass to plugin_create_link is NULL!",
            "",
        );
        return;
    }

    let channel_gid = c_char_to_string(channel_gid, "channel_gid (plugin_create_link)").unwrap();
    unsafe {
        let plugin = &mut (*plugin).plugin;
        *response = plugin.create_link(handle, &channel_gid);
    }
}

#[no_mangle]
pub extern "C" fn plugin_create_link_from_address(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    handle: u64,
    channel_gid: *const c_char,
    link_address: *const c_char,
) {
    if channel_gid.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "channel_gid pointer pass to plugin_load_link_address is NULL!",
            "",
        );
        return;
    }
    if link_address.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "link_address pointer pass to plugin_load_link_address is NULL!",
            "",
        );
        return;
    }

    let channel_gid =
        c_char_to_string(channel_gid, "channel_gid (plugin_load_link_address)").unwrap();
    let link_address =
        c_char_to_string(link_address, "link_address (plugin_load_link_address)").unwrap();
    unsafe {
        let plugin = &mut (*plugin).plugin;
        *response = plugin.create_link_from_address(handle, &channel_gid, &link_address);
    }
}

#[no_mangle]
pub extern "C" fn plugin_load_link_address(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    handle: u64,
    channel_gid: *const c_char,
    link_address: *const c_char,
) {
    if channel_gid.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "channel_gid pointer pass to plugin_load_link_address is NULL!",
            "",
        );
        return;
    }
    if link_address.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "link_address pointer pass to plugin_load_link_address is NULL!",
            "",
        );
        return;
    }

    let channel_gid =
        c_char_to_string(channel_gid, "channel_gid (plugin_load_link_address)").unwrap();
    let link_address =
        c_char_to_string(link_address, "link_address (plugin_load_link_address)").unwrap();
    unsafe {
        let plugin = &mut (*plugin).plugin;
        *response = plugin.load_link_address(handle, &channel_gid, &link_address);
    }
}

#[no_mangle]
pub extern "C" fn plugin_load_link_addresses(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    handle: u64,
    channel_gid: *const c_char,
    link_addresses: *const *const c_char,
    link_addresses_size: usize,
) {
    if channel_gid.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "channel_gid pointer pass to plugin_load_link_addresses is NULL!",
            "",
        );
        return;
    }
    if link_addresses.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "link_addresses pointer pass to plugin_load_link_addresses is NULL!",
            "",
        );
        return;
    }

    let channel_gid =
        c_char_to_string(channel_gid, "channel_gid (plugin_load_link_addresses)").unwrap();
    let mut link_addresses_vec = Vec::new();
    unsafe {
        for index in 0..link_addresses_size {
            let link_address = c_char_to_string(
                *link_addresses.offset(index as isize),
                "link_addresses.offset(index) (plugin_load_link_addresses)",
            )
            .unwrap();
            link_addresses_vec.push(link_address);
        }
        let half_owned: Vec<_> = link_addresses_vec.iter().map(String::as_str).collect();
        let plugin = &mut (*plugin).plugin;
        *response = plugin.load_link_addresses(handle, &channel_gid, &half_owned);
    }
}

#[no_mangle]
pub extern "C" fn plugin_activate_channel(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    handle: u64,
    channel_gid: *const c_char,
    role_name: *const c_char,
) {
    if plugin.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "plugin pointer pass to plugin_activate_channel is NULL!",
            "",
        );
        return;
    }
    if response.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "response pointer pass to plugin_activate_channel is NULL!",
            "",
        );
        return;
    }
    if channel_gid.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "channel_gid pointer pass to plugin_activate_channel is NULL!",
            "",
        );
        return;
    }
    if role_name.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "role_name pointer pass to plugin_activate_channel is NULL!",
            "",
        );
        return;
    }

    let channel_gid =
        c_char_to_string(channel_gid, "channel_gid (plugin_activate_channel)").unwrap();
    let role_name = c_char_to_string(role_name, "role_name (plugin_activate_channel)").unwrap();
    unsafe {
        let plugin = &mut (*plugin).plugin;
        *response = plugin.activate_channel(handle, &channel_gid, &role_name);
    }
}

#[no_mangle]
pub extern "C" fn plugin_deactivate_channel(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    handle: u64,
    channel_gid: *const c_char,
) {
    if channel_gid.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "channel_gid pointer pass to plugin_deactivate_channel is NULL!",
            "",
        );
        return;
    }

    let channel_gid =
        c_char_to_string(channel_gid, "channel_gid (plugin_deactivate_channel)").unwrap();
    unsafe {
        let plugin = &mut (*plugin).plugin;
        *response = plugin.deactivate_channel(handle, &channel_gid);
    }
}

#[no_mangle]
pub extern "C" fn plugin_close_connection(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    handle: u64,
    connection_id: *const c_char,
) {
    if plugin.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "plugin pointer pass to plugin_close_connection is NULL!",
            "",
        );
        return;
    }
    if response.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "response pointer pass to plugin_close_connection is NULL!",
            "",
        );
        return;
    }
    if connection_id.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "connection_id pointer pass to plugin_close_connection is NULL!",
            "",
        );
        return;
    }

    let connection_id =
        c_char_to_string(connection_id, "connection_id (close_connection)").unwrap();
    unsafe {
        let plugin = &mut (*plugin).plugin;
        *response = plugin.close_connection(handle, &connection_id);
    }
}

#[no_mangle]
pub extern "C" fn plugin_on_user_input_received(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    handle: u64,
    answered: bool,
    user_response: *const c_char,
) {
    if plugin.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "plugin pointer pass to plugin_on_user_input_received is NULL!",
            "",
        );
        return;
    }
    if response.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "response pointer pass to plugin_on_user_input_received is NULL!",
            "",
        );
        return;
    }
    if user_response.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "user_response pointer pass to plugin_on_user_input_received is NULL!",
            "",
        );
        return;
    }

    let user_response =
        c_char_to_string(user_response, "user_response (on_user_input_received)").unwrap();
    unsafe {
        let plugin = &mut (*plugin).plugin;
        *response = plugin.on_user_input_received(handle, answered, &user_response);
    }
}

#[no_mangle]
pub extern "C" fn plugin_flush_channel(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    handle: u64,
    channel_id: *const c_char,
    batch_id: u64,
) {
    if plugin.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "plugin pointer passed to plugin_flush_channel is NULL!",
            "",
        );
        return;
    }
    if response.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "response pointer passed to plugin_flush_channel is NULL!",
            "",
        );
        return;
    }
    if channel_id.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "channel_id pointer passed to plugin_flush_channel is NULL!",
            "",
        );
        return;
    }

    let channel_id = c_char_to_string(channel_id, "channel_id (plugin_flush_channel)").unwrap();
    unsafe {
        let plugin = &mut (*plugin).plugin;
        *response = plugin.plugin_flush_channel(handle, &channel_id, batch_id);
    }
}

#[no_mangle]
pub extern "C" fn plugin_on_user_acknowledgment_received(
    plugin: *mut PluginWrapper,
    response: *mut PluginResponse,
    handle: u64,
) {
    if plugin.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "plugin pointer passed to plugin_on_user_acknowledgment_received is NULL!",
            "",
        );
        return;
    }
    if response.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "response pointer passed to plugin_on_user_acknowledgment_received is NULL!",
            "",
        );
        return;
    }

    unsafe {
        let plugin = &mut (*plugin).plugin;
        *response = plugin.plugin_on_user_acknowledgment_received(handle);
    }
}
