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
use super::channel_properties_c::ChannelPropertiesC;
use super::link_properties_c::LinkPropertiesC;
use shims::race_common::i_race_sdk_comms::IRaceSdkComms;
use shims::race_common::link_properties::LinkProperties;

use shims::race_common::channel_properties::ChannelProperties;
use shims::race_common::channel_status::ChannelStatus;
use shims::race_common::connection_status::ConnectionStatus;
use shims::race_common::link_status::LinkStatus;
use shims::race_common::package_status::PackageStatus;
use shims::race_common::plugin_response::PluginResponse;
use shims::race_common::race_enums::BootstrapActionType;
use shims::race_common::race_enums::UserDisplayType;
use shims::race_common::sdk_response::SdkResponse;

use std::ffi::c_void;
use std::ffi::CStr;
use std::ffi::CString;
use std::os::raw::c_char;
use std::slice;

extern "C" {
    fn sdk_get_entropy(sdk: *mut c_void, buffer: *mut c_void, numBytes: u32) -> c_void;

    fn sdk_get_active_persona(sdk: *mut c_void) -> *mut c_char;

    fn sdk_async_error(sdk: *mut c_void, handle: u64, status: PluginResponse) -> SdkResponse;

    fn sdk_get_channel_properties(
        sdk: *mut c_void,
        channel_gid: *const c_char,
    ) -> ChannelPropertiesC;

    fn sdk_on_package_status_changed(
        sdk: *mut c_void,
        handle: u64,
        status: PackageStatus,
        timeout: i32,
    ) -> SdkResponse;

    fn sdk_on_connection_status_changed(
        sdk: *mut c_void,
        handle: u64,
        connId: *const c_char,
        status: ConnectionStatus,
        props: &LinkPropertiesC,
        timeout: i32,
    ) -> SdkResponse;

    fn sdk_on_channel_status_changed(
        sdk: *mut c_void,
        handle: u64,
        channel_gid: *const c_char,
        status: ChannelStatus,
        properties: &ChannelPropertiesC,
        timeout: i32,
    ) -> SdkResponse;

    fn sdk_on_link_status_changed(
        sdk: *mut c_void,
        handle: u64,
        link_id: *const c_char,
        status: LinkStatus,
        properties: &LinkPropertiesC,
        timeout: i32,
    ) -> SdkResponse;

    fn sdk_update_link_properties(
        sdk: *mut c_void,
        linkId: *const c_char,
        props: &LinkPropertiesC,
        timeout: i32,
    ) -> SdkResponse;

    fn sdk_receive_enc_pkg(
        sdk: *mut c_void,
        cipherText: *const c_void,
        cipherTextSize: usize,
        connIDs: *const *const c_char,
        timeout: i32,
    ) -> SdkResponse;

    fn sdk_generate_connection_id(sdk: *mut c_void, linkId: *const c_char) -> *mut c_char;

    fn sdk_generate_link_id(sdk: *mut c_void, channel_gid: *const c_char) -> *mut c_char;

    fn sdk_make_dir(sdk: *mut c_void, dirpath: *const c_char) -> SdkResponse;

    fn sdk_remove_dir(sdk: *mut c_void, dirpath: *const c_char) -> SdkResponse;

    fn sdk_list_dir(
        sdk: *mut c_void,
        dirpath: *const c_char,
        vector_length: *mut usize,
    ) -> *mut *mut c_char;

    fn sdk_read_file(sdk: *mut c_void, filename: *const c_char, data_length: *mut usize)
        -> *mut u8;

    fn sdk_write_file(
        sdk: *mut c_void,
        filename: *const c_char,
        data: *const u8,
        dataLength: usize,
    ) -> SdkResponse;

    fn sdk_append_file(
        sdk: *mut c_void,
        filename: *const c_char,
        data: *const u8,
        dataLength: usize,
    ) -> SdkResponse;

    fn sdk_request_plugin_user_input(
        sdk: *mut c_void,
        key: *const c_char,
        prompt: *const c_char,
        cache: bool,
    ) -> SdkResponse;

    fn sdk_request_common_user_input(sdk: *mut c_void, key: *const c_char) -> SdkResponse;
    fn sdk_display_info_to_user(
        sdk: *mut c_void,
        data: *const c_char,
        displayType: UserDisplayType,
    ) -> SdkResponse;
    fn sdk_display_bootstrap_info_to_user(
        sdk: *mut c_void,
        data: *const c_char,
        displayType: UserDisplayType,
        actionType: BootstrapActionType,
    ) -> SdkResponse;

    fn sdk_unblock_queue(sdk: *mut c_void, conn_id: *const c_char) -> SdkResponse;

    fn sdk_release_string(cstring: *mut c_char) -> c_void;
    fn sdk_delete_string_array(cstring: *mut *mut c_char, data_length: usize) -> c_void;
    fn sdk_release_buffer(vector: *mut u8) -> c_void;
}

pub struct RaceSdkWrapper {
    sdk: *mut c_void,
}

unsafe impl Send for RaceSdkWrapper {}

impl RaceSdkWrapper {
    pub fn new(sdk: *mut c_void) -> RaceSdkWrapper {
        RaceSdkWrapper { sdk }
    }
}

impl IRaceSdkComms for RaceSdkWrapper {
    /// Queries the SDK for entropy from both system calls as well as external
    /// sources.
    ///
    /// # Arguments
    ///
    /// * `num_bytes` - The number of bytes to generate
    ///
    /// # Return Value
    ///
    /// * `Vec<u8>` - Random bytes of length num_bytes
    fn get_entropy(&self, num_bytes: u32) -> Vec<u8> {
        let mut buffer = vec![0_u8; num_bytes as usize];
        unsafe {
            sdk_get_entropy(self.sdk, buffer.as_mut_ptr() as *mut c_void, num_bytes);
        }
        return buffer;
    }

    /// Returns the String ID of the active persona for the RACE system.
    ///
    /// # Arguments
    ///
    /// * None
    ///
    /// # Return Value
    ///
    /// * `String` - The name of the active persona
    fn get_active_persona(&self) -> String {
        unsafe {
            let cstring = sdk_get_active_persona(self.sdk);
            defer! {{
                sdk_release_string(cstring);
            }};
            String::from(
                CStr::from_ptr(cstring)
                    .to_str()
                    .expect("get_active_persona failed"),
            )
        }
    }

    /// Notify the SDK of an error that occured in an asynchronous call
    ///
    /// # Arguments
    ///
    /// * `race_handle` - The handle associated with the async call
    /// * `plugin_response` - The status of the async call
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn async_error(&self, handle: u64, status: PluginResponse) -> SdkResponse {
        unsafe { sdk_async_error(self.sdk, handle, status) }
    }

    /// Get the ChannelProperties for a particular channel
    ///
    /// # Arguments
    ///
    /// * `channelGid` - The name of the channel
    ///
    /// # Return Value
    ///
    /// * `ChannelProperties` - The properties for the channel
    fn get_channel_properties(&self, channel_gid: &str) -> ChannelProperties {
        unsafe {
            let channel_gid = CString::new(channel_gid.clone()).expect("CString::new failed");
            sdk_get_channel_properties(self.sdk, channel_gid.as_ptr()).to_rust_channel_properties()
        }
    }

    /// Notify network manager via the SDK that the status of a package has changed.
    ///
    /// # Arguments
    ///
    /// * `handle` - The RaceHandle identifying the sendPackage call it corresponds to
    /// * `status` - The new ConnectionStatus
    /// * `timeout` - Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
    ///     RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
    ///     arguments
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn on_package_status_changed(
        &self,
        handle: u64,
        status: PackageStatus,
        timeout: i32,
    ) -> SdkResponse {
        unsafe { sdk_on_package_status_changed(self.sdk, handle, status, timeout) }
    }

    /// Notify network manager via the SDK that the stauts of a connection has changed
    ///
    /// # Arguments
    ///
    /// * `handle` - The RaceHandle identifying the network manager call it corresponds to
    /// * `connection_id` - The connection that as changed
    /// * `status` - The new ConnectionStatus
    /// * `properties` - The properties of the changed connection
    /// * `timeout` - Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
    ///     RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
    ///     arguments
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn on_connection_status_changed(
        &self,
        handle: u64,
        connection_id: &str,
        status: ConnectionStatus,
        props: &LinkProperties,
        timeout: i32,
    ) -> SdkResponse {
        unsafe {
            let connection_id = CString::new(connection_id.clone()).expect("CString::new failed");
            sdk_on_connection_status_changed(
                self.sdk,
                handle,
                connection_id.as_ptr(),
                status,
                &LinkPropertiesC::new(props),
                timeout,
            )
        }
    }

    fn on_channel_status_changed(
        &self,
        handle: u64,
        channel_gid: &str,
        status: ChannelStatus,
        properties: &ChannelProperties,
        timeout: i32,
    ) -> SdkResponse {
        unsafe {
            let channel_gid = CString::new(channel_gid.clone()).expect("CString::new failed");
            sdk_on_channel_status_changed(
                self.sdk,
                handle,
                channel_gid.as_ptr(),
                status,
                &ChannelPropertiesC::new(properties),
                timeout,
            )
        }
    }

    fn on_link_status_changed(
        &self,
        handle: u64,
        link_id: &str,
        status: LinkStatus,
        properties: &LinkProperties,
        timeout: i32,
    ) -> SdkResponse {
        unsafe {
            let link_id = CString::new(link_id.clone()).expect("CString::new failed");
            sdk_on_link_status_changed(
                self.sdk,
                handle,
                link_id.as_ptr(),
                status,
                &LinkPropertiesC::new(properties),
                timeout,
            )
        }
    }

    /// Notify the SDK and network manager of a change in LinkProperties
    ///
    /// # Arguments
    ///
    /// * `link_id` - The LinkID identifying the link with updated properties
    /// * `properties` - The new LinkProperties
    /// * `timeout` - Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
    ///     RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
    ///     arguments
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn update_link_properties(
        &self,
        link_id: &str,
        props: &LinkProperties,
        timeout: i32,
    ) -> SdkResponse {
        unsafe {
            let link_id = CString::new(link_id.clone()).expect("CString::new failed");
            sdk_update_link_properties(
                self.sdk,
                link_id.as_ptr(),
                &LinkPropertiesC::new(props),
                timeout,
            )
        }
    }

    /// Notify network manager via the SDK of a new EncPkg that was received
    ///
    /// # Arguments
    ///
    /// * `raw_package` - The EncPkg that was received
    /// * `connection_ids` - The Vector of ConnectionIDs the pkg was received on
    /// * `timeout` - Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
    ///     RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
    ///     arguments
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn receive_enc_pkg(
        &self,
        cipher_text: &[u8],
        connection_ids: &[&str],
        timeout: i32,
    ) -> SdkResponse {
        let connection_ids: Vec<&str> = connection_ids.to_vec();

        // TODO: has to be a better way to do this?
        // TODO: create helper function to convert Vec<string> to *const *const c_char since it is
        // also used in add_link_profile_record(). This is easier said than done since
        // `connection_ids: Vec<CString>` must remain in scope so that the memory gets cleaned up.
        let connection_ids: Vec<CString> = connection_ids
            .iter()
            .map(|connection_id| CString::new(connection_id.clone()).unwrap())
            .collect();
        // This variable shadows the variable `connection_ids: Vec<CString>`, but does not push it
        // out of scope. This is why creating a list of raw pointers from that Vec is safe.
        let mut connection_ids: Vec<*const c_char> =
            connection_ids.iter().map(|arg| arg.as_ptr()).collect();
        connection_ids.push(std::ptr::null()); // null terminate the array of pointers.

        unsafe {
            sdk_receive_enc_pkg(
                self.sdk,
                cipher_text.as_ptr() as *mut c_void,
                cipher_text.len(),
                connection_ids.as_ptr(),
                timeout,
            )
        }
    }

    /// Request the SDK to create a new ConnectionID for the plugin
    ///
    /// # Arguments
    ///
    /// * `link_id` - The link to generate the connection id for
    ///
    /// # Return Value
    ///
    /// * `String` - The generated connection id
    fn generate_connection_id(&self, link_id: &str) -> String {
        unsafe {
            let link_id = CString::new(link_id.clone()).expect("CString::new failed");
            let cstring = sdk_generate_connection_id(self.sdk, link_id.as_ptr());
            defer! {{
                sdk_release_string(cstring);
            }};
            String::from(
                CStr::from_ptr(cstring)
                    .to_str()
                    .expect("generate_connection_id failed"),
            )
        }
    }

    /// Make a directory in this plugin's storage area
    ///
    /// # Arguments
    ///
    /// * `dirpath` - The path of the directory
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn make_dir(&self, dirpath: &str) -> SdkResponse {
        unsafe {
            let dirpath = CString::new(dirpath.clone()).expect("CString::new failed");
            sdk_make_dir(self.sdk, dirpath.as_ptr())
        }
    }

    /// Recursively remove a directory in this plugin's storage area
    ///
    /// # Arguments
    ///
    /// * `dirpath` - The path of the directory
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn remove_dir(&self, dirpath: &str) -> SdkResponse {
        unsafe {
            let dirpath = CString::new(dirpath.clone()).expect("CString::new failed");
            sdk_remove_dir(self.sdk, dirpath.as_ptr())
        }
    }

    /// List the contents of a directory in this plugin's storage space
    ///
    /// # Arguments
    ///
    /// * `dirpath` - The path of the directory
    ///
    /// # Return Value
    ///
    /// * `unsigned byte` - the data of the file (empty-string on error or empty file)
    fn list_dir(&self, dirpath: &str) -> Vec<String> {
        unsafe {
            let dirpath = CString::new(dirpath.clone()).expect("CString::new failed");
            let mut vector_length = 0;
            let listed_dirs = sdk_list_dir(self.sdk, dirpath.as_ptr(), &mut vector_length);
            defer! {{
                sdk_delete_string_array(listed_dirs, vector_length);
            }};

            let mut result: Vec<String> = vec![];
            let vector_length = vector_length as isize;
            for index in 0..vector_length {
                let dir = String::from(
                    CStr::from_ptr(*listed_dirs.offset(index))
                        .to_str()
                        .expect("list_dir failed"),
                );
                result.push(dir);
            }
            return result;
        }
    }

    /// Read a file from this plugin's storage
    ///
    /// # Arguments
    ///
    /// * `filename` - The name of the file
    ///
    /// # Return Value
    ///
    /// * `unsigned byte` - the data of the file (empty-string on error or empty file)
    fn read_file(&self, filename: &str) -> Vec<u8> {
        unsafe {
            let filename = CString::new(filename.clone()).expect("CString::new failed");
            let mut data_length = 0;
            let data_ptr = sdk_read_file(self.sdk, filename.as_ptr(), &mut data_length);
            defer! {{
                sdk_release_buffer(data_ptr);
            }};
            slice::from_raw_parts(data_ptr as *const u8, data_length).to_vec()
        }
    }

    /// Write a file to this plugin's storage area, overwriting if it exists.
    ///
    /// # Arguments
    ///
    /// * `filename` - The name of the file
    /// * `data` - The data to write to the file
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn write_file(&self, filename: &str, data: Vec<u8>) -> SdkResponse {
        unsafe {
            let filename = CString::new(filename.clone()).expect("CString::new failed");
            sdk_write_file(self.sdk, filename.as_ptr(), data.as_ptr(), data.len())
        }
    }

    /// Write a file to this plugin's storage area, appending if it exists.
    ///
    /// # Arguments
    ///
    /// * `filename` - The name of the file
    /// * `data` - The data to write to the file
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - The status of the SDK in response to this call
    fn append_file(&self, filename: &str, data: Vec<u8>) -> SdkResponse {
        unsafe {
            let filename = CString::new(filename.clone()).expect("CString::new failed");
            sdk_append_file(self.sdk, filename.as_ptr(), data.as_ptr(), data.len())
        }
    }

    /// Request the SDK to create a new LinkID for the plugin
    ///
    /// # Arguments
    ///
    /// * None
    ///
    /// # Return Value
    ///
    /// * `String` - The generated link id
    fn generate_link_id(&self, channel_gid: &str) -> String {
        unsafe {
            let channel_gid = CString::new(channel_gid.clone()).expect("CString::new failed");
            let cstring = sdk_generate_link_id(self.sdk, channel_gid.as_ptr());
            defer! {{
                sdk_release_string(cstring);
            }};
            String::from(
                CStr::from_ptr(cstring)
                    .to_str()
                    .expect("generate_link_id failed"),
            )
        }
    }

    /// Request plugin-specific input from the user with the specified prompt message.
    /// The response may be cached in persistent storage, in which case the user will not be
    /// re-prompted if a cached response exists for the given prompt.
    ///
    /// The response will be provided in the userInputReceived callback with the handle matching
    /// the handle returned in this SdkResponse object.
    ///
    /// # Arguments
    ///
    /// * `key` - Prompt identifier for the user input request
    /// * `prompt` - Message to be presented to the user
    /// * `cache` - If true, the response will be cached in persistent storage
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - indicator of success or failure of the request
    fn request_plugin_user_input(&self, key: &str, prompt: &str, cache: bool) -> SdkResponse {
        unsafe {
            let key = CString::new(key.clone()).expect("CString::new failed");
            let prompt = CString::new(prompt.clone()).expect("CString::new failed");

            sdk_request_plugin_user_input(self.sdk, key.as_ptr(), prompt.as_ptr(), cache)
        }
    }

    /// Request application-wide input from the user associated with the given key.
    /// The key identifies a common user input prompt and must be a key supported by the
    /// RACE SDK. The response is cached in persistent storage, so the user will not be
    /// re-prompted if a cached response exists for the given key.
    ///
    /// The response will be provided in the userInputReceived callback with the handle matching
    /// the handle returned in this SdkResponse object.
    ///
    /// # Arguments
    ///
    /// * `key` - Prompt identifier for the application-wide user input request
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - indicator of success or failure of the request
    fn request_common_user_input(&self, key: &str) -> SdkResponse {
        unsafe {
            let key = CString::new(key.clone()).expect("CString::new failed");

            sdk_request_common_user_input(self.sdk, key.as_ptr())
        }
    }

    /// Displays information to the User
    ///
    /// Notification of the user receiving the notification will be sent in
    /// on_user_acknowledgement_received
    ///
    /// # Arguments
    ///
    /// * `data` - Data to display
    /// * `user_display_type` - Type of user display to display data in
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - indicator of success or failure of the request
    fn display_info_to_user(&self, data: &str, user_display_type: UserDisplayType) -> SdkResponse {
        unsafe {
            let data = CString::new(data.clone()).expect("CString::new failed");

            sdk_display_info_to_user(self.sdk, data.as_ptr(), user_display_type)
        }
    }

    /// Displays information to the User and forward information to target node for
    /// automated testing
    ///
    /// Notification of the user receiving the notification will be sent in
    /// on_user_acknowledgement_received
    ///
    /// # Arguments
    ///
    /// * `data` - Data to display
    /// * `user_display_type` - Type of user display to display data in
    /// * `action_type` - Type of action the Daemon must take
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - indicator of success or failure of the request
    fn display_bootstrap_info_to_user(
        &self,
        data: &str,
        user_display_type: UserDisplayType,
        action_type: BootstrapActionType,
    ) -> SdkResponse {
        unsafe {
            let data = CString::new(data.clone()).expect("CString::new failed");

            sdk_display_bootstrap_info_to_user(
                self.sdk,
                data.as_ptr(),
                user_display_type,
                action_type,
            )
        }
    }

    /// Unblock the queue for a connection previously blocked by a return value of
    /// PLUGIN_TEMP_ERROR from sendPackage.
    ///
    /// # Arguments
    ///
    /// * `connId` - The connection to unblock
    ///
    /// # Return Value
    ///
    /// * `SdkResponse` - indicator of success or failure of the request
    fn unblock_queue(&self, connection_id: &str) -> SdkResponse {
        unsafe {
            let connection_id = CString::new(connection_id.clone()).expect("CString::new failed");

            sdk_unblock_queue(self.sdk, connection_id.as_ptr())
        }
    }
}
