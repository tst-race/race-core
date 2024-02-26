
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

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}

#[macro_use(defer)]
extern crate scopeguard;

pub mod race_wrappers {
    mod channel_properties_c;
    mod link_properties_c;
    pub mod plugin;
    pub mod sdk;
}
use race_wrappers::plugin::PluginWrapper;
use race_wrappers::sdk::RaceSdkWrapper;
extern crate shims;
use shims::race_common::race_log::RaceLog;
use std::ffi::c_void;
use std::sync::{Arc, Mutex};

// Import the plugin to wrap.
extern crate plugin_comms_twosix_rust;
use plugin_comms_twosix_rust::plugin_comms_twosix_rust::PluginCommsTwoSixRust as CommsPlugin;

#[no_mangle]
pub extern "C" fn create_plugin(sdk: *mut c_void) -> *mut PluginWrapper {
    if sdk.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "sdk pointer pass to create_plugin is NULL!",
            "",
        );
    }
    let plugin = Box::new(CommsPlugin::new(Arc::new(Mutex::new(RaceSdkWrapper::new(
        sdk,
    )))));
    let plugin = PluginWrapper { plugin };
    Arc::into_raw(Arc::new(plugin)) as *mut PluginWrapper
}

#[no_mangle]
pub extern "C" fn destroy_plugin(plugin: *mut PluginWrapper) {
    if plugin.is_null() {
        RaceLog::log_error(
            "Rust Wrapper",
            "plugin pointer pass to destroy_plugin is NULL!",
            "",
        );
        return;
    }
    unsafe {
        let _ = Arc::from_raw(plugin);
    }
}
