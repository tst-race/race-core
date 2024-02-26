
# 
# Copyright 2023 Two Six Technologies
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 

################################################################################
# plugin wrapper rust library
################################################################################

set(PLUGIN_COMMS_TWOSIX_RUST_WRAPPER_LIB
    ${CMAKE_CURRENT_BINARY_DIR}/target/${CARGO_TARGET}debug/libpluginwrapper.a
)

file(GLOB_RECURSE PLUGIN_SOURCES CONFIGURE_DEPENDS src/*.rs)

add_custom_command(
    COMMAND ${CMAKE_COMMAND} -E env ${CARGO_ENV} cargo build
        --manifest-path=${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
        --target-dir=${CMAKE_CURRENT_BINARY_DIR}/target
        ${CARGO_FLAGS}
    DEPENDS ${PLUGIN_SOURCES} PluginCommsTwoSixRustPlugin
    OUTPUT
        ${PLUGIN_COMMS_TWOSIX_RUST_WRAPPER_LIB}
        ${CMAKE_CURRENT_BINARY_DIR}/target/${CARGO_TARGET}debug/libpluginwrapper.d
)

add_custom_target(PluginCommsTwoSixRustWrapperLib ALL
    DEPENDS ${PLUGIN_COMMS_TWOSIX_RUST_WRAPPER_LIB}
)

add_library(PluginCommsTwoSixRustWrapper STATIC IMPORTED GLOBAL)
set_target_properties(PluginCommsTwoSixRustWrapper PROPERTIES
    IMPORTED_LOCATION ${PLUGIN_COMMS_TWOSIX_RUST_WRAPPER_LIB}
)
add_dependencies(PluginCommsTwoSixRustWrapper PluginCommsTwoSixRustWrapperLib)

setup_rustfmt(PluginCommsTwoSixRustWrapperLib)