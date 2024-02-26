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

// Not repr["C"] because we want to use Strings instead of char ptrs
#[derive(Debug, Clone)]
pub struct PluginConfig {
    pub etc_directory: String,
    pub logging_directory: String,
    pub aux_data_directory: String,
    pub tmp_directory: String,
    pub plugin_directory: String,
}

impl PluginConfig {
    pub fn new(
        etc_directory: &str,
        logging_directory: &str,
        aux_data_directory: &str,
        tmp_directory: &str,
        plugin_directory: &str,
    ) -> PluginConfig {
        PluginConfig {
            etc_directory: String::from(etc_directory),
            logging_directory: String::from(logging_directory),
            aux_data_directory: String::from(aux_data_directory),
            tmp_directory: String::from(tmp_directory),
            plugin_directory: String::from(plugin_directory),
        }
    }
}

impl Default for PluginConfig {
    fn default() -> PluginConfig {
        PluginConfig {
            etc_directory: String::new(),
            logging_directory: String::new(),
            aux_data_directory: String::new(),
            tmp_directory: String::new(),
            plugin_directory: String::new(),
        }
    }
}
