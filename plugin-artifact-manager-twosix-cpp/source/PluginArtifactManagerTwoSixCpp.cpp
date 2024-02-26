
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

#include "PluginArtifactManagerTwoSixCpp.h"

#include <stdio.h>

#include <cerrno>
#include <cstring>

#include "curlwrap.h"
#include "log.h"

PluginArtifactManagerTwoSixCpp::PluginArtifactManagerTwoSixCpp(IRaceSdkArtifactManager *sdk) :
    raceSdk(sdk), hostname("twosix-file-server"), port(8080) {}

PluginResponse PluginArtifactManagerTwoSixCpp::init(const PluginConfig &pluginConfig) {
    logInfo("init: called");
    logInfo("init: etcDirectory: " + pluginConfig.etcDirectory);
    logInfo("init: loggingDirectory: " + pluginConfig.loggingDirectory);
    logInfo("init: tmpDirectory: " + pluginConfig.tmpDirectory);
    logInfo("init: pluginDirectory: " + pluginConfig.pluginDirectory);
    logInfo("init: returned");
    return PLUGIN_OK;
}

PluginResponse PluginArtifactManagerTwoSixCpp::acquireArtifact(const std::string &destPath,
                                                               const std::string &fileName) {
    logInfo("acquireArtifact: called");

    std::string getUrl = "http://" + hostname + ":" + std::to_string(port) + "/" + fileName;
    logDebug("acquireArtifact: attempting to GET " + getUrl);

    FILE *outFile;

    try {
        outFile = fopen(destPath.c_str(), "wb");
        if (outFile == NULL) {
            logError("Unable to open destination file: " + destPath + " errno: " + strerror(errno));
            return PLUGIN_ERROR;
        }

        CurlWrap curl;
        curl.setopt(CURLOPT_URL, getUrl.c_str());
        curl.setopt(CURLOPT_WRITEDATA, outFile);
        curl.setopt(CURLOPT_WRITEFUNCTION, NULL);
        curl.perform();

        fclose(outFile);

        long httpCode = curl.getinfo<long>(CURLINFO_RESPONSE_CODE);
        if (httpCode == 200) {
            logInfo("acquireArtifact: success");
            return PLUGIN_OK;
        }
    } catch (std::exception &error) {
        logWarning("acquireArtifact: exception: " + std::string(error.what()));
        fclose(outFile);
    }

    logInfo("acquireArtifact: returned");
    return PLUGIN_TEMP_ERROR;
}

PluginResponse PluginArtifactManagerTwoSixCpp::onUserInputReceived(RaceHandle, bool,
                                                                   const std::string &) {
    logInfo("onUserAcknowledgementReceived: called");
    return PLUGIN_OK;
}

PluginResponse PluginArtifactManagerTwoSixCpp::onUserAcknowledgementReceived(RaceHandle) {
    logInfo("onUserAcknowledgementReceived: called");
    return PLUGIN_OK;
}

PluginResponse PluginArtifactManagerTwoSixCpp::receiveAmpMessage(
    const std::string & /* message */) {
    logInfo("receiveAmpMessage: called");
    return PLUGIN_OK;
}

#ifndef TESTBUILD
IRacePluginArtifactManager *createPluginArtifactManager(IRaceSdkArtifactManager *sdk) {
    return new PluginArtifactManagerTwoSixCpp(sdk);
}

void destroyPluginArtifactManager(IRacePluginArtifactManager *plugin) {
    delete static_cast<PluginArtifactManagerTwoSixCpp *>(plugin);
}

const RaceVersionInfo raceVersion = RACE_VERSION;
const char *const racePluginId = "PluginArtifactManagerTwoSixCpp";
const char *const racePluginDescription =
    "ArtifactManager Plugin Exemplar (Two Six Tech) " BUILD_VERSION;
#endif
