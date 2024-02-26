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
/// Rust equivalent of the C++ RaceLog class.
/// For reference, the C++ source can be found here: racesdk/common/include/RaceLog.h
///
use std::ffi::CString;
use std::os::raw::c_char;

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub enum LogLevel {
    LlDebug = 0,
    LlInfo = 1,
    LlWarning = 2,
    LlError = 3,
}

extern "C" {
    fn race_log_debug(
        plugin_name: *const c_char,
        message: *const c_char,
        stack_trace: *const c_char,
    );
    fn race_log_info(
        plugin_name: *const c_char,
        message: *const c_char,
        stack_trace: *const c_char,
    );
    fn race_log_warning(
        plugin_name: *const c_char,
        message: *const c_char,
        stack_trace: *const c_char,
    );
    fn race_log_error(
        plugin_name: *const c_char,
        message: *const c_char,
        stack_trace: *const c_char,
    );
}

fn arg_convert(plugin_name: &str, message: &str, stack_trace: &str) -> (CString, CString, CString) {
    let plugin_name_c =
        CString::new(plugin_name).expect("plugin_name needs to be valid as a C String");
    let message_c = CString::new(message).expect("message needs to be valid as a C String");
    let stack_trace_c =
        CString::new(stack_trace).expect("stack_trace needs to be valid as a C String");
    return (plugin_name_c, message_c, stack_trace_c);
}

pub struct RaceLog {}

impl RaceLog {
    pub fn log_debug(plugin_name: &str, message: &str, stack_trace: &str) {
        let (plugin_name_c, message_c, stack_trace_c) =
            arg_convert(plugin_name, message, stack_trace);
        unsafe {
            race_log_debug(
                plugin_name_c.as_ptr(),
                message_c.as_ptr(),
                stack_trace_c.as_ptr(),
            );
        }
    }

    pub fn log_info(plugin_name: &str, message: &str, stack_trace: &str) {
        let (plugin_name_c, message_c, stack_trace_c) =
            arg_convert(plugin_name, message, stack_trace);
        unsafe {
            race_log_info(
                plugin_name_c.as_ptr(),
                message_c.as_ptr(),
                stack_trace_c.as_ptr(),
            );
        }
    }

    pub fn log_warning(plugin_name: &str, message: &str, stack_trace: &str) {
        let (plugin_name_c, message_c, stack_trace_c) =
            arg_convert(plugin_name, message, stack_trace);
        unsafe {
            race_log_warning(
                plugin_name_c.as_ptr(),
                message_c.as_ptr(),
                stack_trace_c.as_ptr(),
            );
        }
    }

    pub fn log_error(plugin_name: &str, message: &str, stack_trace: &str) {
        let (plugin_name_c, message_c, stack_trace_c) =
            arg_convert(plugin_name, message, stack_trace);
        unsafe {
            race_log_error(
                plugin_name_c.as_ptr(),
                message_c.as_ptr(),
                stack_trace_c.as_ptr(),
            );
        }
    }
}
