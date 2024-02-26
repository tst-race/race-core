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

// #[allow(unused_macros)] is used so that modules that use this are not required to use all of log_debug, log info, log warning and log error

#[allow(unused_macros)]
macro_rules! log_debug {
    ($($args:expr),*) =>
        (shims::race_common::race_log::RaceLog::log_debug("Comms Two-Six Rust", &format!($($args),*), ""));
}

#[allow(unused_macros)]
macro_rules! log_info {
    ($($args:expr),*) =>
        (shims::race_common::race_log::RaceLog::log_info("Comms Two-Six Rust", &format!($($args),*), ""));
}

#[allow(unused_macros)]
macro_rules! log_warning {
    ($($args:expr),*) =>
        (shims::race_common::race_log::RaceLog::log_warning("Comms Two-Six Rust", &format!($($args),*), ""));
}

#[allow(unused_macros)]
macro_rules! log_error {
    ($($args:expr),*) =>
        (shims::race_common::race_log::RaceLog::log_error("Comms Two-Six Rust", &format!($($args),*), ""));
}
