
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

#include "IndirectBootstrapLink.h"

#include <RaceLog.h>

#include <fstream>
#include <iterator>
#include <sstream>

#include "../../source/filesystem.h"
#include "../whiteboard/curlwrap.h"
#include "IndirectBootstrapChannel.h"

const std::string file_server_url = "http://twosix-file-server:8080";

IndirectBootstrapLink::IndirectBootstrapLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin,
                                             Channel *channel, const LinkID &linkId,
                                             const LinkProperties &linkProperties,
                                             const TwosixWhiteboardLinkProfileParser &parser,
                                             const std::string &passphrase) :
    TwosixWhiteboardLink(sdk, plugin, channel, linkId, linkProperties, parser),
    bootstrapDir(plugin->getPluginConfig().tmpDirectory + "/indirect-bootstrap"),
    mPassphrase(passphrase) {
    logDebug("IndirectBootstrapLink: created ");
    logDebug("Creating bootstrap dir '" + bootstrapDir + "'");
    fs::create_directories(bootstrapDir);
}

IndirectBootstrapLink::~IndirectBootstrapLink() {
    logDebug("IndirectBootstrapLink: destroyed ");
}

static std::string exec(const std::string &cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

bool IndirectBootstrapLink::uploadFile(std::string filename) {
    std::string postUrl = file_server_url + "/upload";

    int tries = 0;
    for (; tries < maxTries; tries++) {
        try {
            CurlWrap curl;

            logDebug("Attempting to upload bootstrap file to: " + postUrl);

            curl.createUploadForm(filename);

            curl.setopt(CURLOPT_URL, postUrl.c_str());

            // connecton timeout. override the default and set to 10 seconds.
            curl.setopt(CURLOPT_CONNECTTIMEOUT, 10L);

            curl.perform();

            // break out of loop if we were sucessful
            logDebug("Successfully uploaded bootstrap file to: " + postUrl);
            break;

        } catch (curl_exception &error) {
            logWarning("curl exception: " + std::string(error.what()));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return true;
}

PluginResponse IndirectBootstrapLink::serveFiles(std::string path) {
    logDebug("IndirectBootstrapLink::serveFiles called");

    if (!mPassphrase.empty()) {
        std::string filename = mPassphrase;
        std::string fullpath = bootstrapDir + "/" + filename;
        if (fs::is_directory(path)) {
            // path is a directory
            filename += ".tar";
            fullpath += ".tar";
            std::string cmd = "tar -chf " + fullpath + " -C " + path + " .";
            logDebug("serveFiles: taring " + path + " to output archive: " + filename +
                     " cmd: " + cmd);
            std::string output = exec(cmd);
            logDebug("serveFiles: tar output: " + output);
        } else {
            // path is a file
            // keep original file extension
            filename += fs::path(path).extension().string();
            fullpath += fs::path(path).extension().string();
            std::string cmd = "cp " + path + " " + fullpath;
            logDebug("serveFiles: copying " + path + " to : " + filename + " cmd: " + cmd);
            std::string output = exec(cmd);
            logDebug("serveFiles: cp output: " + output);
        }
        logDebug("serveFiles: deleting " + path);
        fs::remove_all(path);
        uploadFile(fullpath);
        std::string downloadUrl = "http://twosix-file-server:8080/" + filename;
        mSdk->displayBootstrapInfoToUser(downloadUrl, RaceEnums::UD_QR_CODE,
                                         RaceEnums::BS_DOWNLOAD_BUNDLE);
    }

    return PLUGIN_OK;
}
