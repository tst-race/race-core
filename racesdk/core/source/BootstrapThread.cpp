
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

#include "BootstrapThread.h"

#include "ArtifactManager.h"
#include "ArtifactManagerWrapper.h"
#include "CommsWrapper.h"
#include "FileSystemHelper.h"
#include "NMWrapper.h"
#include "filesystem.h"
#include "helper.h"

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

BootstrapThread::BootstrapThread(BootstrapManager &manager,
                                 std::shared_ptr<FileSystemHelper> fileSystemHelper) :
    manager(manager), fileSystemHelper(fileSystemHelper), mThreadHandler("bootstrap-thread", 0, 0) {
    mThreadHandler.create_queue("wait queue", std::numeric_limits<int>::min());
    mThreadHandler.start();
}

std::string BootstrapThread::getBootstrapCachePath(const std::string &platform,
                                                   const std::string &architecture,
                                                   const std::string &nodeType) {
    return manager.sdk.getAppConfig().bootstrapCacheDirectory + "/" + platform + "-" +
           architecture + "-" + nodeType;
}

bool BootstrapThread::fetchArtifacts(std::vector<std::string> artifacts,
                                     const std::shared_ptr<BootstrapInfo> &bootstrapInfo) {
    TRACE_METHOD();

    if (bootstrapInfo->state == BootstrapInfo::CANCELLED) {
        helper::logDebug(logPrefix + " bootstrap cancelled");
        return false;
    }

    if (manager.sdk.getArtifactManager() == nullptr) {
        helper::logError("fetchArtifacts called when no ArtifactManager is available");
        return false;
    }

    try {
        std::string postId = std::to_string(nextPostId++);
        helper::logInfo("Posting fetchArtifacts, postId: " + postId);
        auto [success, queueSize, future] = mThreadHandler.post("", 0, -1, [=] {
            const std::string &platform = bootstrapInfo->deviceInfo.platform;
            const std::string &architecture = bootstrapInfo->deviceInfo.architecture;
            const std::string &nodeType = bootstrapInfo->deviceInfo.nodeType;

            helper::logInfo("In fetchArtifacts, postId: " + postId);
            manager.sdk.displayBootstrapInfoToUser("sdk", "Acquiring bootstrap artifacts...",
                                                   RaceEnums::UD_NOTIFICATION,
                                                   RaceEnums::BS_ACQUIRING_ARTIFACT);
            auto artifactManager = manager.sdk.getArtifactManager();
            std::string destPath = getBootstrapCachePath(platform, architecture, nodeType);
            helper::logDebug("creating dir " + destPath);
            fs::create_directories(destPath);
            helper::logDebug("created dir " + destPath);
            for (const std::string &artifact : artifacts) {
                if (bootstrapInfo->state == BootstrapInfo::CANCELLED) {
                    helper::logDebug(logPrefix + " bootstrap cancelled");
                    return std::make_optional(false);
                }
                helper::logInfo("Fetching " + artifact + " to path: " + destPath +
                                ", platform: " + platform + ", architecture: " + architecture +
                                ", nodeType: " + nodeType);
                artifactManager->acquirePlugin(destPath, artifact, platform, nodeType,
                                               architecture);
            }

            return std::make_optional(true);
        });
        (void)queueSize;
        (void)future;

        return success == Handler::PostStatus::OK;
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    } catch (std::exception &error) {
        helper::logError("Unknown exception. This should never happen. what:" +
                         std::string(error.what()));
    }

    // got an exception, fetching artifacts failed
    return false;
}

bool BootstrapThread::createSymlink(const std::string &cachePath, const std::string &destPath,
                                    const std::string &artifactName) {
    std::string cacheArtifactDir = cachePath + "/" + artifactName;
    std::string destArtifactDir = destPath + "/" + artifactName;

    if (!fs::exists(cacheArtifactDir)) {
        helper::logError("Could not find artifact: " + artifactName + " at: " + cachePath);
        return false;
    }

    // Android must copy instead of symlink because the OS does not allow us to create symlinks
    // in the external storage directory
#ifdef __ANDROID__
    try {
        helper::copyDir(cacheArtifactDir, destArtifactDir);
    } catch (std::exception &e) {
        helper::logError("Failed to copy artifact " + cacheArtifactDir + ". what: " + e.what());
        return false;
    }
#else
    try {
        fs::create_directory_symlink(cacheArtifactDir, destArtifactDir);
    } catch (std::exception &e) {
        helper::logError("Failed to symlink artifact " + cacheArtifactDir + ". what: " + e.what());
        return false;
    }
#endif

    return true;
}

bool BootstrapThread::serveFiles(const LinkID &linkId,
                                 const std::shared_ptr<BootstrapInfo> &bootstrapInfo) {
    try {
        if (bootstrapInfo->state == BootstrapInfo::CANCELLED) {
            helper::logDebug("serveFiles bootstrap cancelled");
            return false;
        }

        std::string postId = std::to_string(nextPostId++);
        helper::logInfo("Posting serveFiles, postId: " + postId);
        auto [success, queueSize, future] = mThreadHandler.post("", 0, -1, [=] {
            manager.sdk.displayBootstrapInfoToUser("sdk", "Creating bootstrap bundle...",
                                                   RaceEnums::UD_NOTIFICATION,
                                                   RaceEnums::BS_CREATING_BUNDLE);

            // symlink the downloaded artifacts to the bootstrap dir
            const DeviceInfo &deviceInfo = bootstrapInfo->deviceInfo;
            std::string cachePath = getBootstrapCachePath(
                deviceInfo.platform, deviceInfo.architecture, deviceInfo.nodeType);

            if (bootstrapInfo->state == BootstrapInfo::CANCELLED) {
                helper::logDebug("serveFiles bootstrap cancelled");
                return std::make_optional(false);
            }

            // RACE app
            if (!createSymlink(cachePath, bootstrapInfo->bootstrapPath, "race")) {
                manager.onServeFilesFailed(*bootstrapInfo);
                return std::make_optional(false);
            }

            if (bootstrapInfo->state == BootstrapInfo::CANCELLED) {
                helper::logDebug("serveFiles bootstrap cancelled");
                return std::make_optional(false);
            }

            // network manager plugin
            auto networkManagerPluginDir =
                bootstrapInfo->bootstrapPath + "/artifacts/network-manager";
            if (!createSymlink(cachePath, networkManagerPluginDir, manager.sdk.getNM()->getId())) {
                manager.onServeFilesFailed(*bootstrapInfo);
                return std::make_optional(false);
            }

            if (bootstrapInfo->state == BootstrapInfo::CANCELLED) {
                helper::logDebug("serveFiles bootstrap cancelled");
                return std::make_optional(false);
            }

            // comms channels
            auto commsPluginDir = bootstrapInfo->bootstrapPath + "/artifacts/comms";
            for (auto &pluginName : bootstrapInfo->commsPlugins) {
                if (!createSymlink(cachePath, commsPluginDir, pluginName)) {
                    manager.onServeFilesFailed(*bootstrapInfo);
                    return std::make_optional(false);
                }

                if (bootstrapInfo->state == BootstrapInfo::CANCELLED) {
                    helper::logDebug("serveFiles bootstrap cancelled");
                    return std::make_optional(false);
                }
            }

            // ArtifactManager plugins
            auto artifactManagerPluginDir =
                bootstrapInfo->bootstrapPath + "/artifacts/artifact-manager";
            for (const auto &pluginName : manager.sdk.getArtifactManager()->getIds()) {
                if (!createSymlink(cachePath, artifactManagerPluginDir, pluginName)) {
                    manager.onServeFilesFailed(*bootstrapInfo);
                    return std::make_optional(false);
                }

                if (bootstrapInfo->state == BootstrapInfo::CANCELLED) {
                    helper::logDebug("serveFiles bootstrap cancelled");
                    return std::make_optional(false);
                }
            }

            // Wrap the configs into a gzipped tar file to be placed within the bootstrap bundle
            // tar. This formats them in a way that the node expects, the same way that the node
            // daemon will pass configs to a genesis node.
            // TODO: do this more... programmatically. exec bad.
            std::string configsTarName = bootstrapInfo->bootstrapPath + "/configs.tar.gz";
            std::string tarConfigsCmd =
                "tar -czvf " + configsTarName + " -C " + bootstrapInfo->bootstrapPath + " ./data";
            helper::logDebug("taring " + bootstrapInfo->bootstrapPath +
                             " to output archive: " + configsTarName + " cmd: " + tarConfigsCmd);
            exec(tarConfigsCmd);
            // remove configs that are now in the tar.gz file
            const std::string bootstrapDataPath = bootstrapInfo->bootstrapPath + "/data";
            helper::logDebug("removing: " + bootstrapDataPath + " ...");
            for (auto &childPath : fs::directory_iterator(bootstrapDataPath)) {
                fs::remove_all(childPath);
            }

            if (bootstrapInfo->state == BootstrapInfo::CANCELLED) {
                helper::logDebug("serveFiles bootstrap cancelled");
                return std::make_optional(false);
            }

            // tar bootstrap bundle
            bootstrapInfo->bootstrapBundlePath =
                fs::path(bootstrapInfo->bootstrapPath).filename().native();
            if (bootstrapInfo->bootstrapBundlePath.empty()) {
                std::chrono::duration<double> sinceEpoch =
                    std::chrono::system_clock::now().time_since_epoch();
                bootstrapInfo->bootstrapBundlePath = std::to_string(sinceEpoch.count());
            }
            bootstrapInfo->bootstrapBundlePath =
                manager.sdk.getAppConfig().bootstrapFilesDirectory + "/" +
                bootstrapInfo->bootstrapBundlePath + ".zip";
            if (not fileSystemHelper->createZip(bootstrapInfo->bootstrapBundlePath,
                                                bootstrapInfo->bootstrapPath)) {
                manager.onServeFilesFailed(*bootstrapInfo);
                return std::make_optional(false);
            }

            if (bootstrapInfo->state == BootstrapInfo::CANCELLED) {
                helper::logDebug("serveFiles bootstrap cancelled");
                return std::make_optional(false);
            }

            // serve files and wait for response
            helper::logInfo("In serveFiles");
            manager.sdk.displayBootstrapInfoToUser(
                "sdk", "Preparing to transfer bootstrap bundle...", RaceEnums::UD_NOTIFICATION,
                RaceEnums::BS_PREPARING_TRANSFER);

            manager.sdk.serveFiles(linkId, bootstrapInfo->bootstrapBundlePath, RACE_UNLIMITED);

            if (bootstrapInfo->state == BootstrapInfo::CANCELLED) {
                helper::logDebug("serveFiles bootstrap cancelled");
                return std::make_optional(false);
            }

            helper::logInfo("serveFiles returned, calling openConnection");
            manager.sdk.openConnectionInternal(bootstrapInfo->connectionHandle, LT_RECV, linkId, "",
                                               0, RACE_UNLIMITED, RACE_UNLIMITED);
            helper::logInfo("openConnection returned");
            return std::make_optional(true);
        });
        (void)queueSize;
        (void)future;

        return success == Handler::PostStatus::OK;
    } catch (std::out_of_range &error) {
        helper::logError("Default queue does not exist. This should never happen. what:" +
                         std::string(error.what()));
    } catch (std::exception &error) {
        helper::logError("Unknown exception. This should never happen. what:" +
                         std::string(error.what()));
    }

    return false;
}

bool BootstrapThread::onBootstrapFinished(const std::shared_ptr<BootstrapInfo> &bootstrapInfo) {
    TRACE_METHOD();

    // remove bootstrap directory on same thread as not to interfere with current/pending file IO
    auto [success, queueSize, future] = mThreadHandler.post("", 0, -1, [=] {
        try {
            helper::logDebug(logPrefix + " removing bootstrap dir " + bootstrapInfo->bootstrapPath);
            if (fs::exists(bootstrapInfo->bootstrapPath)) {
                fs::remove_all(bootstrapInfo->bootstrapPath);
            }
            if (fs::exists(bootstrapInfo->bootstrapBundlePath)) {
                fs::remove(bootstrapInfo->bootstrapBundlePath);
            }
        } catch (const std::exception &e) {
            helper::logError(logPrefix + " Failed to remove subdirectories within " +
                             bootstrapInfo->bootstrapPath + ". what: " + e.what());
            return std::make_optional(false);
        }
        return std::make_optional(true);
    });
    (void)queueSize;
    (void)future;
    return success == Handler::PostStatus::OK;
}

void BootstrapThread::waitForCallbacks() {
    auto [success, queueSize, future] =
        mThreadHandler.post("wait queue", 0, -1, [=] { return std::make_optional(true); });
    (void)success;
    (void)queueSize;
    future.wait();
}
