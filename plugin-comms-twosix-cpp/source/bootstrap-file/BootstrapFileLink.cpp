
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

#include "BootstrapFileLink.h"

#include <RaceLog.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

#include "../PluginCommsTwoSixCpp.h"
#include "../base/Connection.h"
#include "../filesystem.h"
#include "../utils/log.h"

BootstrapFileLink::BootstrapFileLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin,
                                     Channel *channel, const LinkID &linkId,
                                     const LinkProperties &linkProperties,
                                     const BootstrapFileLinkProfileParser &parser) :
    Link(sdk, plugin, channel, linkId, linkProperties, parser),
    directory(parser.directory + "/send") {
    mProperties.linkAddress = this->BootstrapFileLink::getLinkAddress();

    try {
        fs::create_directories(directory);
    } catch (std::exception &e) {
        logError(
            "BootstrapFileLink::BootstrapFileLink: Failed to create directory to use for "
            "sending: " +
            std::string(e.what()));
        throw std::runtime_error("Failed to create directory for sending");
    }
}

BootstrapFileLink::~BootstrapFileLink() {
    shutdownLink();
}

std::shared_ptr<Connection> BootstrapFileLink::openConnection(LinkType linkType,
                                                              const ConnectionID &connectionId,
                                                              const std::string &linkHints,
                                                              int timeout) {
    const std::string loggingPrefix = "BootstrapFileLink::openConnection (" + mId + "): ";
    logInfo(loggingPrefix + " called");

    (void)linkHints;

    std::lock_guard<std::mutex> lock(mLinkLock);

    auto connection = std::make_shared<Connection>(connectionId, linkType, shared_from_this(),
                                                   linkHints, timeout);

    mConnections.push_back(connection);

    return connection;
}

void BootstrapFileLink::closeConnection(const ConnectionID &connectionId) {
    logDebug("BootstrapFileLink::closeConnection called");
    std::unique_lock<std::mutex> lock(mLinkLock);

    auto connectionIter =
        std::find_if(mConnections.begin(), mConnections.end(),
                     [&connectionId](const std::shared_ptr<Connection> &connection) {
                         return connectionId == connection->connectionId;
                     });

    if (connectionIter == mConnections.end()) {
        logWarning("BootstrapFileLink::closeConnection no connection found with ID " +
                   connectionId);
        return;
    }

    mConnections.erase(connectionIter);
    logDebug("BootstrapFileLink::closeConnection returned");
}

void BootstrapFileLink::startConnection(Connection * /* connection */) {}

void BootstrapFileLink::shutdownInternal() {}

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

PluginResponse BootstrapFileLink::serveFiles(std::string path) {
    std::chrono::duration<double> sinceEpoch =
        std::chrono::high_resolution_clock::now().time_since_epoch();
    double timestamp = sinceEpoch.count();
    std::string name = std::to_string(timestamp);

    std::string fullpath = directory + "/" + name;
    try {
        if (fs::is_directory(path)) {
            // path is a directory
            fullpath += ".tar";
            std::string cmd = "tar -chf " + fullpath + " -C " + path + " .";
            logDebug("serveFiles: taring " + path + " to output archive: " + fullpath +
                     " cmd: " + cmd);
            std::string output = exec(cmd);
            logDebug("serveFiles: tar output: " + output);
        } else {
            // path is a file
            // keep original file extension
            fullpath += fs::path(path).extension().string();
            std::string cmd = "cp " + path + " " + fullpath;
            logDebug("serveFiles: copying " + path + " to : " + fullpath + " cmd: " + cmd);
            std::string output = exec(cmd);
            logDebug("serveFiles: cp output: " + output);
        }
        mSdk->displayBootstrapInfoToUser(path, RaceEnums::UD_DIALOG, RaceEnums::BS_DOWNLOAD_BUNDLE);
    } catch (std::exception &e) {
        logError("serveFiles: Serving files failed: " + std::string(e.what()));
        return PLUGIN_ERROR;
    }

    return PLUGIN_OK;
}

bool BootstrapFileLink::sendPackageInternal(RaceHandle handle, const EncPkg &pkg) {
    std::chrono::duration<double> sinceEpoch =
        std::chrono::high_resolution_clock::now().time_since_epoch();
    double timestamp = sinceEpoch.count();
    std::string name = std::to_string(timestamp);

    auto bytes = pkg.getRawData();
    bool success = sendBytes(name, bytes);
    if (success) {
        mSdk->onPackageStatusChanged(handle, PACKAGE_SENT, RACE_BLOCKING);
    } else {
        mSdk->onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING);
    }
    return success;
}

bool BootstrapFileLink::sendBytes(const std::string &name, const std::vector<uint8_t> &bytes) {
    std::ofstream outputFile;
    outputFile.open(directory + "/" + name);
    if (!outputFile.good()) {
        return false;
    }

    outputFile.write(reinterpret_cast<const char *>(bytes.data()), static_cast<long>(bytes.size()));
    if (!outputFile.good()) {
        return false;
    }

    return true;
}

std::string BootstrapFileLink::getLinkAddress() {
    // nlohmann::json address;
    // return address.dump();
    return "{}";
}
