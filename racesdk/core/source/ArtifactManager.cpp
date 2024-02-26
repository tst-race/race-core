
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

#include "ArtifactManager.h"

#include <zip.h>

#include <algorithm>
#include <fstream>
#include <iterator>

#include "AppConfig.h"
#include "ArtifactManagerWrapper.h"
#include "ClrMsg.h"
#include "PluginConfig.h"
#include "helper.h"

std::string getPluginArtifactName(const std::string &pluginName, const std::string &platform,
                                  const std::string &nodeType, const std::string &architecture) {
    return platform + "-" + architecture + "-" + nodeType + "-" + pluginName + ".zip";
}

ArtifactManager::ArtifactManager(std::vector<std::unique_ptr<ArtifactManagerWrapper>> &&plugins) :
    plugins(std::move(plugins)) {}

bool ArtifactManager::init(const AppConfig &appConfig) {
    helper::logDebug("Initializing ArtifactManager plugins");

    PluginConfig pluginConfig;
    pluginConfig.etcDirectory = appConfig.etcDirectory;
    pluginConfig.loggingDirectory = appConfig.logDirectory;
    pluginConfig.tmpDirectory = appConfig.tmpDirectory;

    auto iter = plugins.begin();
    while (iter != plugins.end()) {
        auto &plugin = *iter;
        // Aux data directory is intentionally left blank, artifact manager plugins shouldn't use
        // any aux data
        pluginConfig.pluginDirectory =
            appConfig.pluginArtifactsBaseDir + "/artifact-manager/" + plugin->getId();
        auto response = plugin->init(pluginConfig);

        // If a plugin didn't initialize correctly, remove it from the list
        if (response != PLUGIN_OK) {
            helper::logError("ArtifactManager plugin initialization failed for plugin with ID: " +
                             plugin->getId() + ", response: " + pluginResponseToString(response));
            iter = plugins.erase(iter);
        } else {
            ++iter;
        }
    }

    if (plugins.empty()) {
        helper::logError("No ArtifactManager plugins successfully initialized");
        return false;
    }

    helper::logDebug("ArtifactManager plugins initialized");
    return true;
}

bool ArtifactManager::acquirePlugin(const std::string &destPath, const std::string &pluginName,
                                    const std::string &platform, const std::string &nodeType,
                                    const std::string &architecture) {
    std::string artifactFileName =
        getPluginArtifactName(pluginName, platform, nodeType, architecture);
    helper::logDebug("Acquiring plugin artifact: " + artifactFileName);

    std::string localFilePath = destPath + "/" + artifactFileName;

    for (auto &plugin : plugins) {
        helper::logDebug("Attempting to acquire plugin artifact with " + plugin->getId());
        PluginResponse response = plugin->acquireArtifact(localFilePath, artifactFileName);
        if (response == PLUGIN_OK) {
            helper::logDebug("Extracting plugin zip artifact: " + localFilePath);
            if (!extractZip(localFilePath, destPath)) {
                helper::logError("Failed to extract plugin zip artifact: " + localFilePath);
                return false;
            }
            return true;
        } else {
            helper::logWarning("Failed to acquire plugin artifact with " + plugin->getId() + ": " +
                               pluginResponseToString(response));
        }
    }

    helper::logError("Failed to locate plugin artifact: " + artifactFileName);
    return false;
}

std::vector<std::string> ArtifactManager::getIds() const {
    std::vector<std::string> ids;
    std::transform(plugins.begin(), plugins.end(), std::back_inserter(ids),
                   [](const auto &plugin) { return plugin->getId(); });
    return ids;
}

void ArtifactManager::receiveAmpMessage(const ClrMsg &msg) {
    auto plugin = plugins.at(static_cast<size_t>(msg.getAmpIndex()) - 1).get();  // SDK is index 0
    plugin->receiveAmpMessage(msg.getMsg());
}

bool ArtifactManager::extractZip(const std::string &zipFile, const std::string &baseDir) const {
    try {
        struct zip *za;
        struct zip_file *zf;
        struct zip_stat sb;
        char buf[100];
        int err;
        int i, len;
        long long sum;

        if ((za = zip_open(zipFile.c_str(), 0, &err)) == NULL) {
            zip_error_to_str(buf, sizeof(buf), err, errno);
            helper::logError("fail to open zip file: " + std::string(buf));
            return false;
        }
        for (i = 0; i < zip_get_num_entries(za, 0); i++) {
            if (zip_stat_index(za, static_cast<zip_uint64_t>(i), 0, &sb) == 0) {
                len = strlen(sb.name);
                std::string outputFile = baseDir + "/" + std::string(sb.name);
                if (sb.name[len - 1] == '/') {
                    fs::create_directory(outputFile.c_str());
                } else {
                    zf = zip_fopen_index(za, static_cast<zip_uint64_t>(i), 0);
                    if (!zf) {
                        helper::logError("failed to get zip index");
                        return false;
                    }
                    helper::logDebug(outputFile);
                    fs::create_directories(fs::path(outputFile).parent_path());

                    std::ofstream file(outputFile.c_str(), std::ofstream::trunc);
                    if (file.fail()) {
                        helper::logWarning("failed to open file to extract zip to: " + outputFile);
                        return false;
                    }

                    sum = 0;
                    while (static_cast<zip_uint64_t>(sum) != sb.size) {
                        len = zip_fread(zf, buf, 100);
                        if (len <= 0) {
                            helper::logError("failed to read bytes from zip file");
                            return false;
                        }
                        file.write(buf, static_cast<std::int64_t>(len));
                        if (file.fail()) {
                            helper::logWarning("writeFile error writing to file: " + outputFile);
                            return false;
                        }
                        sum += len;
                    }
                    file.close();
                    zip_fclose(zf);
                }
            }
        }
        if (zip_close(za) == -1) {
            helper::logError("failed to close zip file");
        }

        fs::remove(zipFile);

    } catch (const std::exception &e) {
        helper::logError("failed to extract zip file");
    }
    return true;
}
