
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

#include <RaceLog.h>
#include <assert.h>
#include <race/mocks/MockRaceSdkComms.h>
#include <stdio.h>

#include <cstring>
#include <iostream>
#include <memory>

#include "ConnectionStatus.h"
#include "EncPkg.h"
#include "IRacePluginComms.h"
#include "PackageStatus.h"
#include "PluginResponse.h"

const std::string OUTPUT_PREFIX = "COMMSGO CPP TEST APP";
#define APP_COUT std::cout << OUTPUT_PREFIX << ": "

static const std::string logLabel = "GolangCppTestApp";

void logDebug(const std::string &message) {
    RaceLog::logDebug(logLabel, message, "");
}

int main(int /*argc*/, char ** /*argv*/) {
    RaceLog::setLogLevelStdout(RaceLog::LL_DEBUG);

    logDebug("creating sdk");
    MockRaceSdkComms sdk;

    logDebug("creating plugin");
    auto plugin = createPluginComms(&sdk);
    assert(plugin != nullptr);
    printf("plugin %p\n", static_cast<void *>(plugin));

    RaceHandle handle = 1;

    logDebug("init plugin");
    PluginConfig pluginConfig;
    plugin->init(pluginConfig);

    logDebug("activate channels");
    plugin->activateChannel(handle, "", "role");

    logDebug("open connection");
    plugin->openConnection(handle, LT_BIDI, "1", "", RACE_UNLIMITED);

    logDebug("send package");
    const std::string cipherText = "pkg from cpp";
    EncPkg pkg({cipherText.begin(), cipherText.end()});
    plugin->sendPackage(handle, "", pkg, 0, 0);

    logDebug("close connection");
    plugin->closeConnection(handle, "1");

    logDebug("shutdown plugin");
    plugin->shutdown();

    logDebug("destroy plugin");
    destroyPluginComms(plugin);
    return 0;
}
