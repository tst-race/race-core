
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

#include "../../../include/PluginDef.h"
#include "../../../include/RaceExceptions.h"
#include "gtest/gtest.h"

/**
 * @brief If the plugin type is Python and the "python_module" key is either missing or set to empty
 * string then the parser should throw an exception.
 *
 */
TEST(PluginDef, python_plugin_throws_if_missing_module) {
    json input = nlohmann::json::parse(R"({
                "file_path": "PluginNMTwoSixPython",
                "plugin_type": "network-manager",
                "file_type": "python",
                "node_type": "client",
                "python_class": "somepythonclass",
                "platform": "linux"
            })");

    ASSERT_THROW(PluginDef::pluginJsonToPluginDef(input), parsing_error);
}

/**
 * @brief If the plugin type is Python and the "python_class" key is either missing or set to empty
 * string then the parser should throw an exception.
 *
 */
TEST(PluginDef, python_plugin_throws_if_missing_class) {
    json input = nlohmann::json::parse(R"(
            {
                "file_path": "PluginNMTwoSixPython",
                "plugin_type": "network-manager",
                "file_type": "python",
                "node_type": "client",
                "python_module": "PluginNMTwoSixPython.somepythonmodule",
                "platform": "linux"
            }
        )");

    ASSERT_THROW(PluginDef::pluginJsonToPluginDef(input), parsing_error);
}

/**
 * @brief If the file path is missing then the parser should throw an exception.
 *
 */
TEST(PluginDef, plugin_throws_if_missing_file_path) {
    json input = nlohmann::json::parse(R"(
            {
                "plugin_type": "network-manager",
                "file_type": "shared_library",
                "node_type": "client",
                "platform": "linux"
            }
        )");

    ASSERT_THROW(PluginDef::pluginJsonToPluginDef(input), parsing_error);
}

/**
 * @brief If the plugin type is missing or invalid then the parser should throw an exception.
 *
 */
TEST(PluginDef, plugin_throws_if_invalid_type) {
    {
        json input = nlohmann::json::parse(R"(
            {
                "file_path": "libPluginNMServerTwoSixStub.so",
                "file_type": "shared_library",
                "node_type": "client",
                "platform": "linux"
            }
        )");

        ASSERT_THROW(PluginDef::pluginJsonToPluginDef(input), parsing_error);
    }

    {
        json input = nlohmann::json::parse(R"(
            {
                "file_path": "libPluginNMServerTwoSixStub.so",
                "plugin_type": "core",
                "file_type": "shared_library",
                "node_type": "client",
                "platform": "linux"
            }
        )");

        ASSERT_THROW(PluginDef::pluginJsonToPluginDef(input), parsing_error);
    }
}

/**
 * @brief If the plugin file type is missing or invalid then the parser should throw an exception.
 *
 */
TEST(PluginDef, plugin_throws_if_invalid_file_type) {
    {
        json input = nlohmann::json::parse(R"(
            {
                "file_path": "libPluginNMServerTwoSixStub.so",
                "plugin_type": "network-manager",
                "node_type": "client",
                "platform": "linux"
            }
        )");

        ASSERT_THROW(PluginDef::pluginJsonToPluginDef(input), parsing_error);
    }

    {
        json input = nlohmann::json::parse(R"(
            {
                "file_path": "libPluginNMServerTwoSixStub.so",
                "plugin_type": "network-manager",
                "file_type": "fortran",
                "node_type": "client",
                "platform": "linux"
            }
        )");

        ASSERT_THROW(PluginDef::pluginJsonToPluginDef(input), parsing_error);
    }
}

/**
 * @brief If the plugin node type is missing or invalid then the parser should throw an exception.
 *
 */
TEST(PluginDef, plugin_throws_if_invalid_node_type) {
    {
        json input = nlohmann::json::parse(R"(
            {
                "file_path": "libPluginNMServerTwoSixStub.so",
                "plugin_type": "network-manager",
                "file_type": "shared_library",
                "platform": "linux"
            }
        )");

        ASSERT_THROW(PluginDef::pluginJsonToPluginDef(input), parsing_error);
    }

    {
        json input = nlohmann::json::parse(R"(
            {
                "file_path": "libPluginNMServerTwoSixStub.so",
                "plugin_type": "network-manager",
                "file_type": "fortran",
                "node_type": "bob",
                "platform": "linux"
            }
        )");

        ASSERT_THROW(PluginDef::pluginJsonToPluginDef(input), parsing_error);
    }
}
