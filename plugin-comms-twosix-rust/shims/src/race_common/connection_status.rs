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
/// Rust equivalent of the C++ ConnectionStatus enum.
/// For reference, the C++ source can be found here: racesdk/common/include/ConnectionStatus.h
///

/// Need to use repr(C) since this enum is passed over FFI into C.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub enum ConnectionStatus {
    ConnectionUndef = 0,
    ConnectionOpen = 1,
    ConnectionClosed = 2,
    ConnectionAwaitingContact = 3,
    ConnectionInitFailed = 4,
    ConnectionAvailable = 5,
    ConnectionUnavailable = 6,
}
