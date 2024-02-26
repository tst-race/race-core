
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

#ifndef __BOOTSTRAP_THREAD_H__
#define __BOOTSTRAP_THREAD_H__

#include <atomic>
#include <memory>

#include "../include/BootstrapManager.h"
#include "../include/RaceSdk.h"
#include "Handler.h"

class FileSystemHelper;

/*
 * BootstrapThread: A class to manage the thread associated with longer running bootstrap calls
 */
class BootstrapThread {
private:
    BootstrapManager &manager;
    std::shared_ptr<FileSystemHelper> fileSystemHelper;
    Handler mThreadHandler;

    // nextPostId is used to identify which post matches with which call/return log.
    std::atomic<uint64_t> nextPostId = 0;

protected:
    std::string getBootstrapCachePath(const std::string &platform, const std::string &architecture,
                                      const std::string &nodeType);

    bool createSymlink(const std::string &cachePath, const std::string &destPath,
                       const std::string &artifactName);

public:
    BootstrapThread(BootstrapManager &manager, std::shared_ptr<FileSystemHelper> fileSystemHelper);
    virtual ~BootstrapThread() {}

    /**
     * @brief Use artifact manager to download the specified artifacts, or retrieve them from cache
     */
    virtual bool fetchArtifacts(std::vector<std::string> artifacts,
                                const std::shared_ptr<BootstrapInfo> &bootstrapInfo);

    /**
     * @brief Schedule serve files to be called on the bootstrap thread
     */
    virtual bool serveFiles(const LinkID &linkId,
                            const std::shared_ptr<BootstrapInfo> &bootstrapInfo);

    virtual bool onBootstrapFinished(const std::shared_ptr<BootstrapInfo> &bootstrapInfo);

    /* waitForCallbacks: Wait for all callbacks to finish. Used for testing.
     */
    virtual void waitForCallbacks();

protected:
};

#endif
