
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

#include "BootstrapFileWatcher.h"

#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>

#include "../PluginCommsTwoSixCpp.h"
#include "../filesystem.h"
#include "../utils/log.h"
#include "BootstrapFileChannel.h"

static const std::string STOP_FILENAME = "stop";

BootstrapFileWatcher::BootstrapFileWatcher(PluginCommsTwoSixCpp &plugin) :
    plugin(plugin), inotifyFd(0), inotifyWd(0) {}

bool BootstrapFileWatcher::start(const std::string &dir) {
    const std::string logPrefix = "BootstrapFileWatcher::start: ";
    directory = dir;

    try {
        fs::create_directories(directory);
    } catch (std::exception &e) {
        logError(logPrefix + "Failed to create directory to watch: " + e.what());
        return false;
    }

    inotifyFd = inotify_init();
    if (inotifyFd < 0) {
        logError(logPrefix + " Failed to create inotify instance");
        return false;
    }

    inotifyWd = inotify_add_watch(inotifyFd, directory.c_str(), IN_CREATE);
    if (inotifyWd < 0) {
        logError(logPrefix + " Failed add inotify watch for directory " + directory);
        return false;
    }

    monitorThread = std::thread(&BootstrapFileWatcher::runMonitorThread, this);

    return true;
}

bool BootstrapFileWatcher::stop() {
    // create "stop" file. This notifies the thread which is watching the directory to stop.
    std::string stopFilename = directory + "/" + STOP_FILENAME;
    auto stopFile = fopen(stopFilename.c_str(), "w");
    fclose(stopFile);

    monitorThread.join();

    // clean up
    remove(stopFilename.c_str());
    return false;
}

static std::vector<uint8_t> read_file(const std::string &filename) {
    const std::string logPrefix = "read_file: ";
    auto fp = std::fopen(filename.c_str(), "rb");
    if (fp == NULL) {
        logError(logPrefix + "fopen error: " + strerror(errno));
        return {};
    }
    std::vector<uint8_t> contents;
    if (std::fseek(fp, 0, SEEK_END) != 0) {
        logError(logPrefix + "fseek error");
        return {};
    }
    long size = std::ftell(fp);
    if (size < 0) {
        logError(logPrefix + "ftell error");
        return {};
    }
    contents.resize(static_cast<size_t>(size));
    std::rewind(fp);
    std::fread(&contents[0], 1, contents.size(), fp);
    std::fclose(fp);
    return contents;
}

void BootstrapFileWatcher::runMonitorThread() {
    const std::string logPrefix = "BootstrapFileWatcher::runMonitorThread: ";
    bool shouldStop = false;
    size_t bufsiz = sizeof(inotify_event) + PATH_MAX + 1;
    std::vector<uint8_t> buffer(bufsiz);
    // uint8_t *buffer = (uint8_t *)malloc(bufsiz);

    while (!shouldStop) {
        long length = ::read(inotifyFd, buffer.data(), bufsiz);
        if (length < 0) {
            logError(logPrefix + "read error");
            break;
        }

        size_t i = 0;
        while (i < static_cast<size_t>(length)) {
            inotify_event *event = reinterpret_cast<inotify_event *>(&buffer[i]);
            i += sizeof(inotify_event) + event->len;

            if (event->len > 0) {
                // if event->len > 0 then name is a null terminated c string
                // it'st not possible to do name(event->name, event->len) because there may be extra
                // null characters for padding contained within len, but not part of the file name
                std::string name(event->name);

                // This thread gets notified to stop by the other thread creating a file named
                // "stop"
                if (name == STOP_FILENAME) {
                    logError(logPrefix + "received stop command");
                    shouldStop = true;
                    break;
                }

                std::vector<uint8_t> contents = read_file(directory + "/" + name);
                remove(name.c_str());
                if (contents.empty()) {
                    logError(logPrefix + "Failed to read file " + name);
                    continue;
                }

                // We don't know what connection this was received on, or even what link. It would
                // be possible to add structure and files added to a specific directory are received
                // on a specifc link, but as this requires interaction from a user, it's more
                // difficult. This may need to be revisited later if it turns out to be an issue.
                // For now, it's okay to receive it on all connections for this channel.
                std::vector<std::string> connIDs;
                auto links = plugin.linksForChannel(BootstrapFileChannel::bootstrapFileChannelGid);
                for (auto &link : links) {
                    auto connections = link->getConnections();
                    for (auto &connection : connections) {
                        // cppcheck-suppress useStlAlgorithm
                        connIDs.push_back(connection->connectionId);
                    }
                }

                EncPkg pkg(contents);
                plugin.raceSdk->receiveEncPkg(pkg, connIDs, RACE_UNLIMITED);
            }
        }
    }
}
