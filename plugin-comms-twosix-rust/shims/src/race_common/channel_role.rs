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
/// Rust equivalent of the C++ ChannelRole enum.
/// For reference, the C++ source can be found here: racesdk/common/include/ChannelRole.h
///

/// Need to use repr(C) since this enum is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub enum LinkSide {
    LsUndef = 0,
    LsCreator = 1,
    LsLoader = 2,
    LsBoth = 3,
}

/// Need to use repr(C) since this enum is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Clone)]
pub struct ChannelRole {
    pub role_name: String,
    pub mechanical_tags: Vec<String>,
    pub behavioral_tags: Vec<String>,
    pub link_side: LinkSide,
}

impl Default for ChannelRole {
    fn default() -> Self {
        ChannelRole {
            role_name: String::new(),
            mechanical_tags: Vec::new(),
            behavioral_tags: Vec::new(),
            link_side: LinkSide::LsUndef,
        }
    }
}
