
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

#include "PluginArtifactManagerTwoSixCppLocal.h"

#include <stdio.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <nlohmann/json.hpp>

#include "log.h"
#include "zip.h"

#ifdef __ANDROID__
// Boost Filesystem is the original implementation. It may have subtle
// incompatibilities, but it is a widely available implementation.
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#else
// Filesystem TS is more broadly available, and is largely the same.
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif  //__ANDROID__

const char *hostArch() {
#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || \
    defined(__i386) || defined(_M_IX86)
    static const char *arch = "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    static const char *arch = "arm64-v8a";
#else
#error "Unsupported architecture"
#endif
    return arch;
}

const char *hostOs() {
#if defined(__ANDROID__)
    static const char *os = "android";
#elif defined(__unix__)
    static const char *os = "linux";
#else
#error "Unsupported platform"
#endif
    return os;
}

static std::string getPluginArtifactName(const std::string &pluginName, const std::string &platform,
                                         const std::string &nodeType,
                                         const std::string &architecture) {
    return platform + "-" + architecture + "-" + nodeType + "-" + pluginName + ".zip";
}

PluginArtifactManagerTwoSixCppLocal::PluginArtifactManagerTwoSixCppLocal(
    IRaceSdkArtifactManager *sdk) :
    raceSdk(sdk) {}

PluginResponse PluginArtifactManagerTwoSixCppLocal::init(const PluginConfig &pluginConfig) {
    TRACE_METHOD();
    logInfo(logPrefix + "etcDirectory: " + pluginConfig.etcDirectory);
    logInfo(logPrefix + "loggingDirectory: " + pluginConfig.loggingDirectory);
    logInfo(logPrefix + "tmpDirectory: " + pluginConfig.tmpDirectory);
    logInfo(logPrefix + "pluginDirectory: " + pluginConfig.pluginDirectory);

    using nlohmann::json;

    try {
        std::string arch = hostArch();
        std::string platform = hostOs();

        fs::path pluginPath = pluginConfig.pluginDirectory;
        pluginPath = pluginPath.parent_path().parent_path();
        std::vector<std::string> pluginTypes = {"network-manager", "comms", "artifact-manager"};
        for (std::string &pluginType : pluginTypes) {
            for (auto const &dir_entry : fs::directory_iterator{pluginPath / pluginType}) {
                logDebug(logPrefix + dir_entry.path().string());
                fs::path dirPath = dir_entry.path();
                fs::path pluginName = dirPath.filename();
                fs::path manifestPath = dirPath / "manifest.json";

                bool hasClient = false;
                bool hasServer = false;
                try {
                    std::ifstream manifestFile(manifestPath.string());
                    json manifest = json::parse(manifestFile);
                    for (json &plugin : manifest.at("plugins")) {
                        logDebug(logPrefix + plugin.dump());
                        logDebug(logPrefix + dir_entry.path().string());
                        std::string nodeType = plugin.at("node_type");
                        if (nodeType == "any") {
                            hasClient = true;
                            hasServer = true;
                        } else if (nodeType == "client") {
                            hasClient = true;
                        } else if (nodeType == "server") {
                            hasServer = true;
                        }
                    }
                } catch (std::exception &e) {
                    logError(logPrefix + "malformed manifest.json: " + e.what());
                    hasClient = false;
                    hasServer = false;
                }

                if (hasClient) {
                    std::string artifactName =
                        getPluginArtifactName(pluginName.string(), platform, "client", arch);
                    logDebug(logPrefix + "Creating local artifact entry for " + artifactName);
                    artifactMap[artifactName] = dirPath.string();
                }

                if (hasServer) {
                    std::string artifactName =
                        getPluginArtifactName(pluginName.string(), platform, "server", arch);
                    logDebug(logPrefix + "Creating local artifact entry for " + artifactName);
                    artifactMap[artifactName] = dirPath.string();
                }
            }
        }

        appArtifactName = getPluginArtifactName("race", platform, "client", arch);
        artifactMap[appArtifactName] = (pluginPath / "core" / "race").string();
    } catch (std::exception &e) {
        logError(logPrefix + "init failed: " + e.what());
        return PLUGIN_ERROR;
    }
    return PLUGIN_OK;
}

PluginResponse PluginArtifactManagerTwoSixCppLocal::acquireArtifact(const std::string &destPath,
                                                                    const std::string &fileName) {
    TRACE_METHOD(destPath, fileName);

#ifdef __ANDROID__
    if (fileName == appArtifactName) {
        std::string appPath = raceSdk->getAppPath();
        if (createApkZip(destPath, appPath)) {
            logDebug(logPrefix + "fetch android apk success");
            return PLUGIN_OK;
        } else {
            logError(logPrefix + "fetch android apk error");
            return PLUGIN_ERROR;
        }
    }
#endif

    auto it = artifactMap.find(fileName);
    if (it != artifactMap.end()) {
        if (createZip(destPath, it->second)) {
            logDebug(logPrefix + "fetch artifact success");
            return PLUGIN_OK;
        } else {
            logError(logPrefix + "fetch artifact error");
            return PLUGIN_ERROR;
        }
    } else {
        logDebug(logPrefix + "fetch artifact not found locally");
        return PLUGIN_ERROR;
    }
}

PluginResponse PluginArtifactManagerTwoSixCppLocal::onUserInputReceived(RaceHandle, bool,
                                                                        const std::string &) {
    TRACE_METHOD();
    return PLUGIN_OK;
}

PluginResponse PluginArtifactManagerTwoSixCppLocal::onUserAcknowledgementReceived(RaceHandle) {
    TRACE_METHOD();
    return PLUGIN_OK;
}

PluginResponse PluginArtifactManagerTwoSixCppLocal::receiveAmpMessage(
    const std::string & /* message */) {
    TRACE_METHOD();
    return PLUGIN_OK;
}

IRacePluginArtifactManager *createPluginArtifactManager(IRaceSdkArtifactManager *sdk) {
    return new PluginArtifactManagerTwoSixCppLocal(sdk);
}

void destroyPluginArtifactManager(IRacePluginArtifactManager *plugin) {
    delete static_cast<PluginArtifactManagerTwoSixCppLocal *>(plugin);
}

const RaceVersionInfo raceVersion = RACE_VERSION;
const char *const racePluginId = "PluginArtifactManagerTwoSixCppLocal";
const char *const racePluginDescription = "Local ArtifactManager Plugin (Two Six Tech) ";
