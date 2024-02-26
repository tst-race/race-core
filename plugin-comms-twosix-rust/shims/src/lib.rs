
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

pub mod race_common {
    pub mod channel_properties;
    pub mod channel_role;
    pub mod channel_status;
    pub mod connection_status;
    pub mod i_race_plugin_comms;
    pub mod i_race_sdk_comms;
    pub mod link_properties;
    pub mod link_status;
    pub mod package_status;
    pub mod plugin_config;
    pub mod plugin_response;
    pub mod race_log;
    pub mod race_enums;
    pub mod sdk_response;
    pub mod send_type;
}
