
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

#include "PluginResponse.h"
#include "gtest/gtest.h"

TEST(PluginResponse, toString) {
    EXPECT_EQ(pluginResponseToString(PLUGIN_INVALID), "PLUGIN_INVALID");
    EXPECT_EQ(pluginResponseToString(PLUGIN_OK), "PLUGIN_OK");
    EXPECT_EQ(pluginResponseToString(PLUGIN_TEMP_ERROR), "PLUGIN_TEMP_ERROR");
    EXPECT_EQ(pluginResponseToString(PLUGIN_ERROR), "PLUGIN_ERROR");
    EXPECT_EQ(pluginResponseToString(PLUGIN_FATAL), "PLUGIN_FATAL");
    EXPECT_EQ(pluginResponseToString(static_cast<PluginResponse>(99)),
              "ERROR: INVALID PLUGIN RESPONSE: 99");
}
