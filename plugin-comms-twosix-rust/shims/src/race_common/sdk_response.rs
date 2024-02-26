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
/// Rust equivalent of the C++ SdkResponse class.
/// For reference, the C++ source can be found here: racesdk/common/include/SdkResponse.h
///

pub const NULL_RACE_HANDLE: u64 = 0;

/// Need to use repr(C) since this enum is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub enum SdkStatus {
    SdkUndef = 0,
    SdkOk = 1,
    SdkShuttingDown = 2,
    SdkPluginMissing = 3,
    SdkInvalidArgument = 4,
    SdkQueueFull = 5,
}

/// Need to use repr(C) since this struct is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct SdkResponse {
    pub status: SdkStatus,
    pub queue_utilization: f64,
    pub handle: u64,
}