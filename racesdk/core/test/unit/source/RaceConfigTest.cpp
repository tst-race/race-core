
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

#include <nlohmann/json.hpp>

#include "../../../include/RaceConfig.h"
#include "../../common/race_printers.h"
#include "gtest/gtest.h"

using nlohmann::json;

// wrapper class to expose protected method
class RaceConfigWrap : public RaceConfig {
public:
    RaceConfigWrap() : RaceConfig() {
        appConfig.nodeType = RaceEnums::NodeType::NT_SERVER;
    }
    void wrapParseConfigString(std::string config) {
        parseConfigString(config, appConfig);
    }

    AppConfig appConfig;
};

const json compositions = {{
                               {"id", "twoSixIndirectComposition"},
                               {"transport", "twoSixIndirect"},
                               {"usermodel", "periodic"},
                               {"encodings", {"base64"}},
                               {"architecture", "x86_64"},
                               {"node_type", "client"},
                               {"platform", "linux"},
                           },
                           {
                               {"id", "twoSixIndirectComposition"},
                               {"transport", "twoSixIndirect"},
                               {"usermodel", "periodic"},
                               {"encodings", {"base64"}},
                               {"architecture", "x86_64"},
                               {"node_type", "server"},
                               {"platform", "linux"},
                           },
                           {
                               {"id", "twoSixIndirectComposition"},
                               {"transport", "twoSixIndirect"},
                               {"usermodel", "periodic"},
                               {"encodings", {"base64"}},
                               {"architecture", "arm64-v8a"},
                               {"node_type", "client"},
                               {"platform", "linux"},
                           },
                           {
                               {"id", "twoSixIndirectComposition"},
                               {"transport", "twoSixIndirect"},
                               {"usermodel", "periodic"},
                               {"encodings", {"base64"}},
                               {"architecture", "arm64-v8a"},
                               {"node_type", "server"},
                               {"platform", "linux"},
                           },
                           {
                               {"id", "twoSixIndirectComposition"},
                               {"transport", "twoSixIndirect"},
                               {"usermodel", "periodic"},
                               {"encodings", {"base64"}},
                               {"architecture", "x86_64"},
                               {"node_type", "client"},
                               {"platform", "android"},
                           }};

const json channels = {
    {{"bootstrap", true},
     {"isFlushable", true},
     {"channelGid", "twoSixIndirectCpp"},
     {"connectionType", "CT_INDIRECT"},
     {"creatorExpected",
      {{"send", {{"bandwidth_bps", 277200}, {"latency_ms", 3190}, {"loss", 0.1}}},
       {"receive", {{"bandwidth_bps", 277200}, {"latency_ms", 3190}, {"loss", 0.1}}}}},
     {"description",
      "Implementation of the Two Six Labs Indirect communications utilizing the Two Six "
      "Whiteboard"},
     {"duration_s", -1},
     {"linkDirection", "LD_BIDI"},
     {"loaderExpected",
      {{"send", {{"bandwidth_bps", 277200}, {"latency_ms", 3190}, {"loss", 0.1}}},
       {"receive", {{"bandwidth_bps", 277200}, {"latency_ms", 3190}, {"loss", 0.1}}}}},
     {"mtu", -1},
     {"multiAddressable", false},
     {"period_s", -1},
     {"reliable", false},
     {"sendType", "ST_STORED_ASYNC"},
     {"supported_hints", json::array()},
     {"transmissionType", "TT_MULTICAST"},
     {"maxLinks", -1},
     {"creatorsPerLoader", -1},
     {"loadersPerCreator", -1},
     {"roles", {}},
     {"maxSendsPerInterval", 42},
     {"secondsPerInterval", 3600},
     {"intervalEndTime", 0},
     {"sendsRemainingInInterval", -1}},
    {{"bootstrap", true},
     {"isFlushable", true},
     {"channelGid", "twoSixDirectCpp"},
     {"connectionType", "CT_DIRECT"},
     {"creatorExpected",
      {{"send", {{"bandwidth_bps", 25700000}, {"latency_ms", 16}, {"loss", -1.0}}},
       {"receive", {{"bandwidth_bps", 25700000}, {"latency_ms", 16}, {"loss", -1.0}}}}},
     {"description", "Implementation of the Two Six Labs Direct communications utilizing Sockets"},
     {"duration_s", -1},
     {"linkDirection", "LD_LOADER_TO_CREATOR"},
     {"loaderExpected",
      {{"send", {{"bandwidth_bps", 25700000}, {"latency_ms", 16}, {"loss", -1.0}}},
       {"receive", {{"bandwidth_bps", 25700000}, {"latency_ms", 16}, {"loss", -1.0}}}}},
     {"mtu", -1},
     {"multiAddressable", false},
     {"period_s", -1},
     {"reliable", false},
     {"sendType", "ST_EPHEM_SYNC"},
     {"supported_hints", json::array()},
     {"transmissionType", "TT_UNICAST"},
     {"maxLinks", -1},
     {"creatorsPerLoader", -1},
     {"loadersPerCreator", -1},
     {"roles", {}},
     {"maxSendsPerInterval", -1},
     {"secondsPerInterval", -1},
     {"intervalEndTime", 0},
     {"sendsRemainingInInterval", -1}}};

const json android_x86_64_client_network_manager = {
    {"architecture", "x86_64"},
    {"config_path", "PluginNMTwoSixStub/"},
    {"file_path", "PluginNMTwoSixStub"},
    {"file_type", "shared_library"},
    {"node_type", "client"},
    {"platform", "android"},
    {"plugin_type", "network-manager"},
    {"shared_library_path", "libPluginNMClientTwoSixStub.so"}};

const json android_arm64_client_network_manager = {
    {"architecture", "arm64-v8a"},
    {"config_path", "PluginNMTwoSixStub/"},
    {"file_path", "PluginNMTwoSixStub"},
    {"file_type", "shared_library"},
    {"node_type", "client"},
    {"platform", "android"},
    {"plugin_type", "network-manager"},
    {"shared_library_path", "libPluginNMClientTwoSixStub.so"}};

const json linux_x86_64_client_network_manager = {
    {"architecture", "x86_64"},
    {"config_path", "PluginNMTwoSixStub/"},
    {"file_path", "PluginNMTwoSixStub"},
    {"file_type", "shared_library"},
    {"node_type", "client"},
    {"platform", "linux"},
    {"plugin_type", "network-manager"},
    {"shared_library_path", "libPluginNMClientTwoSixStub.so"}};

const json linux_arm64_client_network_manager = {
    {"architecture", "arm64-v8a"},
    {"config_path", "PluginNMTwoSixStub/"},
    {"file_path", "PluginNMTwoSixStub"},
    {"file_type", "shared_library"},
    {"node_type", "client"},
    {"platform", "linux"},
    {"plugin_type", "network-manager"},
    {"shared_library_path", "libPluginNMClientTwoSixStub.so"}};

const json linux_x86_64_server_network_manager = {
    {"architecture", "x86_64"},
    {"config_path", "PluginNMTwoSixStub/"},
    {"file_path", "PluginNMTwoSixStub"},
    {"file_type", "shared_library"},
    {"node_type", "server"},
    {"platform", "linux"},
    {"plugin_type", "network-manager"},
    {"shared_library_path", "libPluginNMServerTwoSixStub.so"}};

const json linux_arm64_server_network_manager = {
    {"architecture", "arm64-v8a"},
    {"config_path", "PluginNMTwoSixStub/"},
    {"file_path", "PluginNMTwoSixStub"},
    {"file_type", "shared_library"},
    {"node_type", "server"},
    {"platform", "linux"},
    {"plugin_type", "network-manager"},
    {"shared_library_path", "libPluginNMServerTwoSixStub.so"}};

const json android_x86_64_client_comms = {{"architecture", "x86_64"},
                                          {"config_path", "PluginCommsTwoSixStub/"},
                                          {"file_path", "PluginCommsTwoSixStub"},
                                          {"file_type", "shared_library"},
                                          {"node_type", "client"},
                                          {"platform", "android"},
                                          {"plugin_type", "comms"},
                                          {"shared_library_path", "libPluginCommsTwoSixStub.so"},
                                          {"channels", {"some-comms-channel"}},
                                          {"transports", {"twoSixIndirect"}},
                                          {"usermodels", {"periodic"}},
                                          {"encodings", {"base64"}}};

const json android_arm64_client_comms = {{"architecture", "arm64-v8a"},
                                         {"config_path", "PluginCommsTwoSixStub/"},
                                         {"file_path", "PluginCommsTwoSixStub"},
                                         {"file_type", "shared_library"},
                                         {"node_type", "client"},
                                         {"platform", "android"},
                                         {"plugin_type", "comms"},
                                         {"shared_library_path", "libPluginCommsTwoSixStub.so"},
                                         {"channels", {"some-comms-channel"}},
                                         {"transports", {"twoSixIndirect"}},
                                         {"usermodels", {"periodic"}},
                                         {"encodings", {"base64"}}};

const json linux_x86_64_client_comms = {{"architecture", "x86_64"},
                                        {"config_path", "PluginCommsTwoSixStub/"},
                                        {"file_path", "PluginCommsTwoSixStub"},
                                        {"file_type", "shared_library"},
                                        {"node_type", "client"},
                                        {"platform", "linux"},
                                        {"plugin_type", "comms"},
                                        {"shared_library_path", "libPluginCommsTwoSixStub.so"},
                                        {"channels", {"some-comms-channel"}},
                                        {"transports", {"twoSixIndirect"}},
                                        {"usermodels", {"periodic"}},
                                        {"encodings", {"base64"}}};

const json linux_arm64_client_comms = {{"architecture", "arm64-v8a"},
                                       {"config_path", "PluginCommsTwoSixStub/"},
                                       {"file_path", "PluginCommsTwoSixStub"},
                                       {"file_type", "shared_library"},
                                       {"node_type", "client"},
                                       {"platform", "linux"},
                                       {"plugin_type", "comms"},
                                       {"shared_library_path", "libPluginCommsTwoSixStub.so"},
                                       {"channels", {"some-comms-channel"}},
                                       {"transports", {"twoSixIndirect"}},
                                       {"usermodels", {"periodic"}},
                                       {"encodings", {"base64"}}};

const json linux_x86_64_server_comms = {{"architecture", "x86_64"},
                                        {"config_path", "PluginCommsTwoSixStub/"},
                                        {"file_path", "PluginCommsTwoSixStub"},
                                        {"file_type", "shared_library"},
                                        {"node_type", "server"},
                                        {"platform", "linux"},
                                        {"plugin_type", "comms"},
                                        {"shared_library_path", "libPluginCommsTwoSixStub.so"},
                                        {"channels", {"some-comms-channel"}},
                                        {"transports", {"twoSixIndirect"}},
                                        {"usermodels", {"periodic"}},
                                        {"encodings", {"base64"}}};

const json linux_arm64_server_comms = {{"architecture", "arm64-v8a"},
                                       {"config_path", "PluginCommsTwoSixStub/"},
                                       {"file_path", "PluginCommsTwoSixStub"},
                                       {"file_type", "shared_library"},
                                       {"node_type", "server"},
                                       {"platform", "linux"},
                                       {"plugin_type", "comms"},
                                       {"shared_library_path", "libPluginCommsTwoSixStub.so"},
                                       {"channels", {"some-comms-channel"}},
                                       {"transports", {"twoSixIndirect"}},
                                       {"usermodels", {"periodic"}},
                                       {"encodings", {"base64"}}};

const json linux_x86_64_client_network_manager_python = {
    {"architecture", "x86_64"},
    {"file_path", "PluginNMTwoSixPython"},
    {"plugin_type", "network-manager"},
    {"file_type", "python"},
    {"node_type", "client"},
    {"python_module", "PluginNMTwoSixPython.somepythonmodule"},
    {"python_class", "somepythonclass"},
    {"platform", "linux"}};

const json linux_arm64_client_network_manager_python = {
    {"architecture", "arm64-v8a"},
    {"file_path", "PluginNMTwoSixPython"},
    {"plugin_type", "network-manager"},
    {"file_type", "python"},
    {"node_type", "client"},
    {"python_module", "PluginNMTwoSixPython.somepythonmodule"},
    {"python_class", "somepythonclass"},
    {"platform", "linux"}};

const json linux_x86_64_server_network_manager_python = {
    {"architecture", "x86_64"},
    {"file_path", "PluginNMTwoSixPython"},
    {"plugin_type", "network-manager"},
    {"file_type", "python"},
    {"node_type", "server"},
    {"python_module", "PluginNMTwoSixPython.somepythonmodule"},
    {"python_class", "somepythonclass"},
    {"platform", "linux"}};

const json linux_arm64_server_network_manager_python = {
    {"architecture", "arm64-v8a"},
    {"file_path", "PluginNMTwoSixPython"},
    {"plugin_type", "network-manager"},
    {"file_type", "python"},
    {"node_type", "server"},
    {"python_module", "PluginNMTwoSixPython.somepythonmodule"},
    {"python_class", "somepythonclass"},
    {"platform", "linux"}};

const json linux_x86_64_server_comms_python = {
    {"architecture", "x86_64"},
    {"config_path", "PluginCommsTwoSixStub/"},
    {"file_path", "PluginCommsTwoSixStub"},
    {"file_type", "shared_library"},
    {"node_type", "server"},
    {"platform", "linux"},
    {"plugin_type", "comms"},
    {"shared_library_path", "libPluginCommsTwoSixStub.so"}};

const json linux_arm64_server_comms_python = {
    {"architecture", "arm64-v8a"},
    {"config_path", "PluginCommsTwoSixStub/"},
    {"file_path", "PluginCommsTwoSixStub"},
    {"file_type", "shared_library"},
    {"node_type", "server"},
    {"platform", "linux"},
    {"plugin_type", "comms"},
    {"shared_library_path", "libPluginCommsTwoSixStub.so"}};

const json defaultPlugins = {android_x86_64_client_network_manager,
                             linux_x86_64_client_network_manager,
                             linux_x86_64_server_network_manager,
                             android_x86_64_client_comms,
                             linux_x86_64_client_comms,
                             linux_x86_64_server_comms,
                             linux_arm64_client_network_manager,
                             linux_arm64_server_network_manager,
                             linux_arm64_client_comms,
                             linux_arm64_server_comms,
                             android_arm64_client_network_manager,
                             android_arm64_client_comms};

const json base = {
    {"android_python_path",
     "/data/data/com.twosix.race/python3.7/:/data/data/com.twosix.race/python3.7/encodings/:/data/"
     "data/com.twosix.race/python3.7/lib-dynload/:/data/data/com.twosix.race/race/python/:/data/"
     "data/com.twosix.race/race/network-manager/:data/data/com.twosix.race/race/comms/:data/data/"
     "com.twosix.race/"
     "python3.7/ordered-set-4.0.2:data/data/com.twosix.race/python3.7/jaeger-client-4.3.0:data/"
     "data/com.twosix.race/python3.7/pycryptodome-3.9.9:/data/data/com.twosix.race/python3.7/"
     "simplejson-3.16.0:/data/data/com.twosix.race/python3.7/PyYAML-5.3.1:/data/data/"
     "com.twosix.race/python3.7/opentracing-2.4.0:/data/data/com.twosix.race/python3.7/"
     "thrift-0.13.0:/data/data/com.twosix.race/python3.7/tornado-6.1"},
    {"bandwidth", "-1"},
    {"debug", "false"},
    {"isPluginFetchOnStartEnabled", "false"},
    {"latency", "-1"},
    {"level", "DEBUG"},
    {"log-race-config", "true"},
    {"log-network-manager-config", "true"},
    {"log-comms-config", "true"},
    {"msg-log-length", "256"},
    {"storage-encryption", "aes"},
    {"plugins", defaultPlugins},
    {"channels", channels},
    {"compositions", std::vector<std::string>()},
    {"environment_tags", {{"", std::vector<std::string>()}}}};

TEST(RaceConfig, default_values) {
    RaceConfig raceConfig;
    EXPECT_EQ(raceConfig.androidPythonPath, "");
    EXPECT_EQ(raceConfig.plugins[RaceEnums::PluginType::PT_NM].size(), 0);
    EXPECT_EQ(raceConfig.plugins[RaceEnums::PluginType::PT_COMMS].size(), 0);
    EXPECT_EQ(raceConfig.plugins[RaceEnums::PluginType::PT_ARTIFACT_MANAGER].size(), 0);
    EXPECT_EQ(raceConfig.channels.size(), 0);
    EXPECT_EQ(raceConfig.isPluginFetchOnStartEnabled, false);
    EXPECT_EQ(raceConfig.isVoaEnabled, true);
    EXPECT_EQ(raceConfig.wrapperQueueMaxSize, 10 * 1024 * 1024);
    EXPECT_EQ(raceConfig.wrapperTotalMaxSize, 2048u * 1024 * 1024);
    EXPECT_EQ(raceConfig.logLevel, RaceLog::LL_DEBUG);
    EXPECT_EQ(raceConfig.logRaceConfig, true);
    EXPECT_EQ(raceConfig.logNMConfig, true);
    EXPECT_EQ(raceConfig.logCommsConfig, true);
    EXPECT_EQ(raceConfig.msgLogLength, 256);
}

TEST(RaceConfigWrap, parseMaxQueueSize) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    // we're testing with the config json having strings because if you use the RiB commands to edit
    // the config file RiB writes the values as strings
    raceJson["max_queue_size"] = "12";
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
    ASSERT_EQ(raceConfig.wrapperQueueMaxSize, 12);
}

TEST(RaceConfigWrap, parseMaxQueueSize2) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    // we're testing with the config json having strings because if you use the RiB commands to edit
    // the config file RiB writes the values as strings
    raceJson["max_queue_size"] = "1234567890";
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
    ASSERT_EQ(raceConfig.wrapperQueueMaxSize, 1234567890);
}

/**
 * @brief If the plugins section is missing then parsing should throw an error.
 *
 */
TEST(RaceConfigWrap, missing_channels_and_plugins_section) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson.erase("channels");
    raceJson.erase("plugins");
    std::string jsonString = raceJson.dump();
    ASSERT_THROW(raceConfig.wrapParseConfigString(jsonString),
                 RaceConfig::race_config_parsing_exception);

    ASSERT_EQ(raceConfig.getNMPluginDefs().size(), 0);
    ASSERT_EQ(raceConfig.getCommsPluginDefs().size(), 0);
    ASSERT_EQ(raceConfig.channels.size(), 0);
}

/**
 * @brief If the plugins section is missing then the parser should throw an error.
 *
 */
TEST(RaceConfigWrap, missing_plugins_section) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson.erase("plugins");
    std::string jsonString = raceJson.dump();
    ASSERT_THROW(raceConfig.wrapParseConfigString(jsonString),
                 RaceConfig::race_config_parsing_exception);

    ASSERT_EQ(raceConfig.getNMPluginDefs().size(), 0);
    ASSERT_EQ(raceConfig.getCommsPluginDefs().size(), 0);
}

/**
 * @brief If the value for plugins is an invalid type then the parser should throw an error.
 *
 */
TEST(RaceConfigWrap, plugins_section_invalid_type) {
    RaceConfigWrap raceConfig = RaceConfigWrap();

    {
        json raceJson = base;
        raceJson["plugins"] = json::object();
        std::string jsonString = raceJson.dump();
        EXPECT_THROW(raceConfig.wrapParseConfigString(jsonString),
                     RaceConfig::race_config_parsing_exception);
    }

    {
        json raceJson = base;
        raceJson["plugins"] = "some invalid string type";
        std::string jsonString = raceJson.dump();
        EXPECT_THROW(raceConfig.wrapParseConfigString(jsonString),
                     RaceConfig::race_config_parsing_exception);
    }

    {
        json raceJson = base;
        raceJson["plugins"] = 1234.5678;
        std::string jsonString = raceJson.dump();
        EXPECT_THROW(raceConfig.wrapParseConfigString(jsonString),
                     RaceConfig::race_config_parsing_exception);
    }

    {
        json raceJson = base;
        raceJson["plugins"] = true;
        std::string jsonString = raceJson.dump();
        EXPECT_THROW(raceConfig.wrapParseConfigString(jsonString),
                     RaceConfig::race_config_parsing_exception);
    }

    {
        json raceJson = base;
        raceJson["plugins"] = nullptr;
        std::string jsonString = raceJson.dump();
        EXPECT_THROW(raceConfig.wrapParseConfigString(jsonString),
                     RaceConfig::race_config_parsing_exception);
    }
}

/**
 * @brief If the plugins section is present, but is empty, the parser should throw an exception.
 *
 */
TEST(RaceConfigWrap, throws_if_the_plugins_section_is_empty) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["plugins"] = json::array();
    std::string jsonString = raceJson.dump();
    EXPECT_THROW(raceConfig.wrapParseConfigString(jsonString),
                 RaceConfig::race_config_parsing_exception);
}

/**
 * @brief The parser will filter out plugins that are not intended for the current node type. In
 * this example the parser is configured for a client, but the plugin node type is server. Since
 * this plugin is not meant for this node it will be filtered out. Since it is filtered out and
 * there are no available network manager plugins the parser will throw an exception
 *
 */
TEST(RaceConfigWrap, plugins_section_has_plugin_intended_for_different_node_type) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    raceConfig.appConfig.nodeType = RaceEnums::NodeType::NT_CLIENT;
    json raceJson = base;
    raceJson["plugins"] = {linux_x86_64_server_network_manager, linux_x86_64_server_comms,
                           linux_arm64_server_network_manager, linux_arm64_server_comms};

    std::string jsonString = raceJson.dump();
    ASSERT_THROW(raceConfig.wrapParseConfigString(jsonString),
                 RaceConfig::race_config_parsing_exception);
}

/**
 * @brief The parser will return all valid plugins from the configuration.
 *
 */
TEST(RaceConfigWrap, plugins_section_has_valid_plugin) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);

    ASSERT_EQ(raceConfig.getNMPluginDefs().size(), 1);
    {
        auto pluginDef = raceConfig.getNMPluginDefs()[0];
        EXPECT_EQ(pluginDef.filePath, "PluginNMTwoSixStub");
        EXPECT_EQ(pluginDef.type, RaceEnums::PluginType::PT_NM);
        EXPECT_EQ(pluginDef.fileType, RaceEnums::PluginFileType::PFT_SHARED_LIB);
        EXPECT_EQ(pluginDef.pythonModule, "");
        EXPECT_EQ(pluginDef.pythonClass, "");
        EXPECT_EQ(pluginDef.configPath, "PluginNMTwoSixStub/");
        EXPECT_EQ(pluginDef.sharedLibraryPath, "libPluginNMServerTwoSixStub.so");
    }

    {
        EXPECT_EQ(raceConfig.getCommsPluginDefs().size(), 1);
        auto pluginDef = raceConfig.getCommsPluginDefs()[0];
        EXPECT_EQ(pluginDef.filePath, "PluginCommsTwoSixStub");
        EXPECT_EQ(pluginDef.type, RaceEnums::PluginType::PT_COMMS);
        EXPECT_EQ(pluginDef.fileType, RaceEnums::PluginFileType::PFT_SHARED_LIB);
        EXPECT_EQ(pluginDef.pythonModule, "");
        EXPECT_EQ(pluginDef.pythonClass, "");
        EXPECT_EQ(pluginDef.configPath, "PluginCommsTwoSixStub/");
        EXPECT_EQ(pluginDef.sharedLibraryPath, "libPluginCommsTwoSixStub.so");
    }
}

TEST(RaceConfigWrap, plugins_section_parses_python_plugin) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["plugins"] = {
        linux_x86_64_server_network_manager_python, linux_x86_64_server_comms_python,
        linux_arm64_server_network_manager_python, linux_arm64_server_comms_python};
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);

    ASSERT_EQ(raceConfig.getNMPluginDefs().size(), 1);
    auto pluginDef = raceConfig.getNMPluginDefs()[0];
    EXPECT_EQ(pluginDef.filePath, "PluginNMTwoSixPython");
    EXPECT_EQ(pluginDef.type, RaceEnums::PluginType::PT_NM);
    EXPECT_EQ(pluginDef.fileType, RaceEnums::PluginFileType::PFT_PYTHON);
    EXPECT_EQ(pluginDef.pythonModule, "PluginNMTwoSixPython.somepythonmodule");
    EXPECT_EQ(pluginDef.pythonClass, "somepythonclass");
    EXPECT_EQ(pluginDef.configPath, "");

    EXPECT_EQ(raceConfig.getCommsPluginDefs().size(), 1);
}

TEST(RaceConfigWrap, channels_section_parses_channels) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
    ASSERT_EQ(raceConfig.channels.size(), 2);

    EXPECT_EQ(raceConfig.channels[0].bootstrap, true);
    EXPECT_EQ(raceConfig.channels[0].isFlushable, true);
    EXPECT_EQ(raceConfig.channels[0].channelGid, "twoSixIndirectCpp");
    EXPECT_EQ(raceConfig.channels[0].connectionType, CT_INDIRECT);
    EXPECT_EQ(raceConfig.channels[0].duration_s, -1);
    EXPECT_EQ(raceConfig.channels[0].linkDirection, LD_BIDI);
    EXPECT_EQ(raceConfig.channels[0].mtu, -1);
    EXPECT_EQ(raceConfig.channels[0].multiAddressable, false);
    EXPECT_EQ(raceConfig.channels[0].period_s, -1);
    EXPECT_EQ(raceConfig.channels[0].reliable, false);
    EXPECT_EQ(raceConfig.channels[0].sendType, ST_STORED_ASYNC);
    EXPECT_EQ(raceConfig.channels[0].supported_hints, std::vector<std::string>());
    EXPECT_EQ(raceConfig.channels[0].transmissionType, TT_MULTICAST);

    EXPECT_EQ(raceConfig.channels[0].creatorExpected.send.bandwidth_bps, 277200);
    EXPECT_EQ(raceConfig.channels[0].creatorExpected.send.latency_ms, 3190);
    EXPECT_NEAR(raceConfig.channels[0].creatorExpected.send.loss, 0.1, 0.0001);
    EXPECT_EQ(raceConfig.channels[0].creatorExpected.receive.bandwidth_bps, 277200);
    EXPECT_EQ(raceConfig.channels[0].creatorExpected.receive.latency_ms, 3190);
    EXPECT_NEAR(raceConfig.channels[0].creatorExpected.receive.loss, 0.1, 0.0001);

    EXPECT_EQ(raceConfig.channels[0].loaderExpected.send.bandwidth_bps, 277200);
    EXPECT_EQ(raceConfig.channels[0].loaderExpected.send.latency_ms, 3190);
    EXPECT_NEAR(raceConfig.channels[0].loaderExpected.send.loss, 0.1, 0.0001);
    EXPECT_EQ(raceConfig.channels[0].loaderExpected.receive.bandwidth_bps, 277200);
    EXPECT_EQ(raceConfig.channels[0].loaderExpected.receive.latency_ms, 3190);
    EXPECT_NEAR(raceConfig.channels[0].loaderExpected.receive.loss, 0.1, 0.0001);

    EXPECT_EQ(raceConfig.channels[0].maxSendsPerInterval, 42);
    EXPECT_EQ(raceConfig.channels[0].secondsPerInterval, 3600);
    EXPECT_EQ(raceConfig.channels[0].intervalEndTime, 0);
    EXPECT_EQ(raceConfig.channels[0].sendsRemainingInInterval, -1);

    EXPECT_EQ(raceConfig.channels[1].bootstrap, true);
    EXPECT_EQ(raceConfig.channels[1].isFlushable, true);
    EXPECT_EQ(raceConfig.channels[1].channelGid, "twoSixDirectCpp");
    EXPECT_EQ(raceConfig.channels[1].connectionType, CT_DIRECT);
    EXPECT_EQ(raceConfig.channels[1].duration_s, -1);
    EXPECT_EQ(raceConfig.channels[1].linkDirection, LD_LOADER_TO_CREATOR);
    EXPECT_EQ(raceConfig.channels[1].mtu, -1);
    EXPECT_EQ(raceConfig.channels[1].multiAddressable, false);
    EXPECT_EQ(raceConfig.channels[1].period_s, -1);
    EXPECT_EQ(raceConfig.channels[1].reliable, false);
    EXPECT_EQ(raceConfig.channels[1].sendType, ST_EPHEM_SYNC);
    EXPECT_EQ(raceConfig.channels[1].supported_hints, std::vector<std::string>());
    EXPECT_EQ(raceConfig.channels[1].transmissionType, TT_UNICAST);

    EXPECT_EQ(raceConfig.channels[1].creatorExpected.send.bandwidth_bps, 25700000);
    EXPECT_EQ(raceConfig.channels[1].creatorExpected.send.latency_ms, 16);
    EXPECT_NEAR(raceConfig.channels[1].creatorExpected.send.loss, -1., 0.0001);
    EXPECT_EQ(raceConfig.channels[1].creatorExpected.receive.bandwidth_bps, 25700000);
    EXPECT_EQ(raceConfig.channels[1].creatorExpected.receive.latency_ms, 16);
    EXPECT_NEAR(raceConfig.channels[1].creatorExpected.receive.loss, -1, 0.0001);

    EXPECT_EQ(raceConfig.channels[1].loaderExpected.send.bandwidth_bps, 25700000);
    EXPECT_EQ(raceConfig.channels[1].loaderExpected.send.latency_ms, 16);
    EXPECT_NEAR(raceConfig.channels[1].loaderExpected.send.loss, -1., 0.0001);
    EXPECT_EQ(raceConfig.channels[1].loaderExpected.receive.bandwidth_bps, 25700000);
    EXPECT_EQ(raceConfig.channels[1].loaderExpected.receive.latency_ms, 16);
    EXPECT_NEAR(raceConfig.channels[1].loaderExpected.receive.loss, -1, 0.0001);

    EXPECT_EQ(raceConfig.channels[1].maxSendsPerInterval, -1);
    EXPECT_EQ(raceConfig.channels[1].secondsPerInterval, -1);
    EXPECT_EQ(raceConfig.channels[1].intervalEndTime, 0);
    EXPECT_EQ(raceConfig.channels[1].sendsRemainingInInterval, -1);
}

TEST(RaceConfigWrap, throws_if_multiple_network_manager_plugins_are_specified) {
    {
        // client
        RaceConfigWrap raceConfig = RaceConfigWrap();
        raceConfig.appConfig.nodeType = RaceEnums::NodeType::NT_CLIENT;
        json raceJson = base;
        raceJson["plugins"] = {linux_x86_64_client_network_manager,
                               linux_x86_64_client_network_manager_python,
                               linux_x86_64_client_comms,
                               linux_arm64_client_network_manager,
                               linux_arm64_client_network_manager_python,
                               linux_arm64_client_comms};
        std::string jsonString = raceJson.dump();
        ASSERT_THROW(raceConfig.wrapParseConfigString(jsonString),
                     RaceConfigWrap::race_config_parsing_exception);
    }

    {
        // server
        RaceConfigWrap raceConfig = RaceConfigWrap();
        json raceJson = base;
        raceJson["plugins"] = {linux_x86_64_server_network_manager,
                               linux_x86_64_server_network_manager_python,
                               linux_x86_64_server_comms,
                               linux_arm64_server_network_manager,
                               linux_arm64_server_network_manager_python,
                               linux_arm64_server_comms};
        std::string jsonString = raceJson.dump();
        ASSERT_THROW(raceConfig.wrapParseConfigString(jsonString),
                     RaceConfigWrap::race_config_parsing_exception);
    }
}

TEST(RaceConfigWrap, compositions_section_has_valid_compositions) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["compositions"] = compositions;
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);

    ASSERT_EQ(raceConfig.compositions.size(), 1);
    {
        auto composition = raceConfig.compositions[0];
        EXPECT_EQ(composition.id, "twoSixIndirectComposition");
        EXPECT_EQ(composition.transport, "twoSixIndirect");
        EXPECT_EQ(composition.usermodel, "periodic");
        nlohmann::json encodings = composition.encodings;
        EXPECT_EQ(encodings.dump(), "[\"base64\"]");
    }
}

TEST(RaceConfigWrap, compositions_section_with_plugin_missing_components) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["compositions"] = compositions;
    for (auto &plugin : raceJson["plugins"]) {
        plugin["channels"] = std::vector<std::string>();
        plugin["transports"] = std::vector<std::string>{};
        plugin["usermodels"] = std::vector<std::string>{};
        plugin["encodings"] = std::vector<std::string>{};
    }
    std::string jsonString = raceJson.dump();

    ASSERT_THROW(raceConfig.wrapParseConfigString(jsonString),
                 RaceConfigWrap::race_config_parsing_exception);
}

TEST(RaceConfigWrap, compositions_section_missing) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson.erase("compositions");
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);

    ASSERT_EQ(raceConfig.compositions.size(), 0);
}

/**
 * @brief Check that the parser parses the "android_python_path" field.
 *
 */
TEST(RaceConfigWrap, parses_android_python_path) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    raceConfig.appConfig.nodeType = RaceEnums::NodeType::NT_CLIENT;
    raceConfig.appConfig.platform = "android";
    json raceJson = base;
    raceJson["android_python_path"] = "some/android/python/path";
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);

    ASSERT_EQ(raceConfig.androidPythonPath, "some/android/python/path");
}

TEST(RaceConfigWrap, parseLogLevel_valid_log_level_value_in_config) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["level"] = "INFO";
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
    ASSERT_EQ(raceConfig.logLevel, RaceLog::LL_INFO);
}

TEST(RaceConfigWrap, parseLogLevel_invalid_log_level_value_in_config) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["level"] = "INVALID";
    std::string jsonString = raceJson.dump();
    EXPECT_THROW(raceConfig.wrapParseConfigString(jsonString),
                 RaceConfig::race_config_parsing_exception);
}

TEST(RaceConfigWrap, parseLogRaceConfig) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["log-race-config"] = "true";
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
    ASSERT_EQ(raceConfig.logRaceConfig, true);
}

TEST(RaceConfigWrap, parseLogRaceConfig_false) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["log-race-config"] = "false";
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
    ASSERT_EQ(raceConfig.logRaceConfig, false);
}

TEST(RaceConfigWrap, parseLogNMConfig) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["log-network-manager-config"] = "true";
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
    ASSERT_EQ(raceConfig.logNMConfig, true);
}

TEST(RaceConfigWrap, parseLogNMConfig_false) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["log-network-manager-config"] = "false";
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
    ASSERT_EQ(raceConfig.logNMConfig, false);
}

TEST(RaceConfigWrap, parseLogCommsConfig) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["log-comms-config"] = "true";
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
    ASSERT_EQ(raceConfig.logCommsConfig, true);
}

TEST(RaceConfigWrap, parseLogCommsConfig_false) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["log-comms-config"] = "false";
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
    ASSERT_EQ(raceConfig.logCommsConfig, false);
}

TEST(RaceConfigWrap, parseMsgLogLength_Invalid_Number) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["msg-log-length"] = "NOT_A_NUMBER";
    std::string jsonString = raceJson.dump();
    EXPECT_THROW(raceConfig.wrapParseConfigString(jsonString),
                 RaceConfig::race_config_parsing_exception);
}

TEST(RaceConfigWrap, parseMsgLogLength) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["msg-log-length"] = "10";
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
    unsigned long number = 10;
    EXPECT_EQ(raceConfig.msgLogLength, number);
}

TEST(RaceConfigWrap, parseEnvTags) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["environment_tags"] = {{"", std::vector<std::string>()}};
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
    std::unordered_map<std::string, std::vector<std::string>> expected = {{"", {}}};
    EXPECT_EQ(raceConfig.environmentTags, expected);
}

TEST(RaceConfigWrap, parseEnvTags2) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["environment_tags"] = {{"env1", std::vector<std::string>{"tag1", "tag2", "tag3"}},
                                    {"env2", std::vector<std::string>{"tag4", "tag5"}}};
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
    std::unordered_map<std::string, std::vector<std::string>> expected = {
        {"env1", {"tag1", "tag2", "tag3"}}, {"env2", {"tag4", "tag5"}}};
    EXPECT_EQ(raceConfig.environmentTags, expected);
}

TEST(RaceConfigWrap, parseStorageEncryption) {
    RaceConfigWrap raceConfig = RaceConfigWrap();
    json raceJson = base;
    raceJson["storage-encryption"] = "none";
    std::string jsonString = raceJson.dump();
    raceConfig.wrapParseConfigString(jsonString);
}
