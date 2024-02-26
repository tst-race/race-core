
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
# comms plugin rust library
################################################################################

set(PLUGIN_COMMS_TWOSIX_RUST_PLUGIN_RLIB
    ${CMAKE_CURRENT_BINARY_DIR}/target/${CARGO_TARGET}debug/libplugin_comms_twosix_rust.rlib
)

file(GLOB_RECURSE PLUGIN_SOURCES CONFIGURE_DEPENDS src/*.rs)

add_custom_command(
    COMMAND ${CMAKE_COMMAND} -E env ${CARGO_ENV} cargo build
        --manifest-path=${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
        --target-dir=${CMAKE_CURRENT_BINARY_DIR}/target
        ${CARGO_FLAGS}
    DEPENDS ${PLUGIN_SOURCES} PluginCommsTwoSixRustShims
    OUTPUT
        ${PLUGIN_COMMS_TWOSIX_RUST_PLUGIN_RLIB}
        ${CMAKE_CURRENT_BINARY_DIR}/target/${CARGO_TARGET}debug/libplugin_comms_twosix_rust.d
)

add_custom_target(PluginCommsTwoSixRustPluginRlib ALL
    DEPENDS ${PLUGIN_COMMS_TWOSIX_RUST_PLUGIN_RLIB}
)

add_library(PluginCommsTwoSixRustPlugin STATIC IMPORTED GLOBAL)
set_target_properties(PluginCommsTwoSixRustPlugin PROPERTIES
    IMPORTED_LOCATION ${PLUGIN_COMMS_TWOSIX_RUST_PLUGIN_RLIB}
)
add_dependencies(PluginCommsTwoSixRustPlugin PluginCommsTwoSixRustPluginRlib)

setup_rustfmt(PluginCommsTwoSixRustPlugin)