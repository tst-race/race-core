
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

#include "BootstrapServer.h"

#include <cpprest/asyncrt_utils.h>
#include <cpprest/containerstream.h>
#include <cpprest/filestream.h>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <cpprest/producerconsumerstream.h>
#include <cpprest/uri.h>

#include <fstream>

#include "../filesystem.h"
#include "../utils/log.h"

#ifdef __ANDROID__
#include <JavaShimUtils.h>

#include <mutex>
#endif

using namespace web::http;

const std::string listener_url = "http://0.0.0.0:2626";

// on android, cpprest_init needs to be called with a reference to the java vm
// this needs to be done before the listener member variable is inited, so it can't be done inside
// the constructor
void initialize_cpprest() {
#ifdef __ANDROID__
    static std::once_flag cpprest_init_flag;
    std::call_once(cpprest_init_flag, []() { cpprest_init(JavaShimUtils::getJvm()); });
#endif
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

BootstrapServer::BootstrapServer(const std::string &_bootstrapDir) :
    bootstrapDir(_bootstrapDir + "/bootstrap"),
    running(false),
    // comma operator, return value from initialize_cpprest is ignored
    listener((initialize_cpprest(), listener_url)) {
    logDebug("BootstrapServer::constructor called");
    logDebug("Creating bootstrap dir '" + bootstrapDir + "'");
    fs::create_directories(bootstrapDir);
    listener.support(methods::GET,
                     [this](const http_request &message) { this->handleGet(message); });  // NOLINT
}

void BootstrapServer::startServer() {
    logDebug("BootstrapServer::startServer called");
    if (!running) {
        listener.open().wait();
        running = true;
    }
}

void BootstrapServer::stopServer() {
    logDebug("BootstrapServer::stopServer called");
    if (running) {
        listener.close().wait();
        running = false;
    }
}

BootstrapServer::~BootstrapServer() {
    try {
        stopServer();
    } catch (std::exception &err) {
        logError("BootstrapServer::~BootstrapServer: error: " + std::string(err.what()));
    }
}

void BootstrapServer::serveFile(const std::string &passphrase, const std::string &path) {
    logDebug("BootstrapServer::serveFile called");

    if (requestMap.count(passphrase) != 0) {
        throw std::invalid_argument("passphrase already mapped");
    }

    requestMap[passphrase] = path;

    // Start the server if it hasn't been started yet
    startServer();
}

void BootstrapServer::serveFiles(const std::string &passphrase, const std::string &path) {
    // Create dest filename
    std::string filename = fs::path(path).filename().native();
    if (filename.empty()) {
        std::chrono::duration<double> sinceEpoch =
            std::chrono::system_clock::now().time_since_epoch();
        filename = std::to_string(sinceEpoch.count());
    }
    filename = bootstrapDir + "/" + filename;

    // tar path if path is a directory (sdk should tar it but handling dir just in case)
    if (fs::is_directory(path)) {
        // path is a directory
        filename = filename + ".tar";
        std::string cmd = "tar -chf " + filename + " -C " + path + " .";
        logDebug("serveFiles: taring " + path + " to output archive: " + filename + " cmd: " + cmd);
        std::string output = exec(cmd);
        logDebug("serveFiles: tar output: " + output);
    } else {
        // path is a file
        std::string cmd = "cp " + path + " " + filename;
        logDebug("serveFiles: copying " + path + " to : " + filename + " cmd: " + cmd);
        std::string output = exec(cmd);
        logDebug("serveFiles: cp output: " + output);
    }

    // serve file
    serveFile(passphrase, filename);
    logDebug("serveFiles: deleting " + path);
    fs::remove_all(path);
}

void BootstrapServer::stopServing(const std::string &passphrase) {
    logDebug("stopServing: " + passphrase);
    auto iter = requestMap.find(passphrase);
    if (iter != requestMap.end()) {
        auto path = iter->second;
        requestMap.erase(iter);
        logDebug("stopServing: deleting " + path);
        fs::remove(path);
    }
}

void BootstrapServer::handleGet(http_request message) {
    logDebug("BootstrapServer: got GET request: " + message.to_string());

    try {
        std::string passphrase = message.request_uri().path().substr(1);  // remove / from front
        logDebug("BootstrapServer: : got passphrase " + passphrase);
        std::string path = requestMap.at(passphrase);
        logDebug("BootstrapServer: : got path " + path);

        logDebug("BootstrapServer: : open file");
        size_t size = fs::file_size(path);
        auto file =
            concurrency::streams::fstream::open_istream(path, std::ios::in | std::ios::binary)
                .get();
        logDebug("BootstrapServer: : Responding OK with file " + path);
        message.reply(status_codes::OK, file, size);
        return;
    } catch (std::exception &e) {
        logDebug("BootstrapServer: got exception: " + std::string(e.what()));
    }
    logDebug("BootstrapServer: : Responding Not Found");
    message.reply(status_codes::NotFound);
}
