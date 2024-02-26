
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

#ifndef __SOURCE_BOOTSTRAP_MANAGER_H__
#define __SOURCE_BOOTSTRAP_MANAGER_H__

#include <StorageEncryption.h>

#include <atomic>
#include <memory>  // std::unique_ptr
#include <mutex>   // std::mutex, std::lock_guard

#include "AppConfig.h"
#include "DeviceInfo.h"
#include "FileSystemHelper.h"
#include "IRaceSdkComms.h"
#include "IRaceSdkNM.h"
#include "OpenTracingForwardDeclarations.h"
#include "PersonaForwardDeclarations.h"
#include "WrapperForwardDeclarations.h"

class RaceSdk;

/*
The bootstrap manager handles all bootstrap logic on the introducer node.

Expected states:

INITIALIZED -> WAITING_FOR_LINK
    triggered by: prepareToBootstrap call
    triggers    : Create bootstrap link

WAITING_FOR_LINK -> WAITING_FOR_NM
    triggered by: onLinkStatusChanged call (link created)
    triggers    : network manager prepare to bootstrap

WAITING_FOR_NM -> WAITING_FOR_BOOTSTRAP_PKG
    triggered by: bootstrapDevice call
    triggers    : serveFiles

WAITING_FOR_BOOTSTRAP_PKG -> WAITING_FOR_CONNECTION_CLOSED
    triggered by: receiveEncPkg call
    triggers    : add persona, close connection

WAITING_FOR_CONNECTION_CLOSED -> SUCCESS
    triggered by: onConnectionStatusChanged call (closed)
    triggers    : add persona, delete pending bootstrap

Error cases

INITIALIZED -> FAILED
    triggered by: prepareToBootstrap call (filesystem error, bootstrap channel error)
    triggers    : Bootstrap failed

WAITING_FOR_LINK -> FAILED
    triggered by: onLinkStatusChanged call (link destroyed)
    triggers    : Bootstrap failed

WAITING_FOR_NM -> FAILED
    triggered by: Bootstrap failed call
    triggers    : Bootstrap failed

WAITING_FOR_NM -> FAILED
    triggered by: onLinkStatusChanged call (link destroyed)
    triggers    : Bootstrap failed

WAITING_FOR_BOOTSTRAP_PKG -> FAILED
    triggered by: corrupted pkg received
    triggers    : Bootstrap failed

WAITING_FOR_BOOTSTRAP_PKG -> FAILED
    triggered by: Bootstrap failed call
    triggers    : Bootstrap failed

WAITING_FOR_BOOTSTRAP_PKG -> FAILED
    triggered by: onLinkStatusChanged call (link destroyed)
    triggers    : Bootstrap failed

* -> CANCELLED
    triggered by: race app
    triggers: network manager -> onBootstrapFinished

*/

struct BootstrapInfo {
    enum State {
        INITIALIZED,
        WAITING_FOR_LINK,
        WAITING_FOR_NM,
        WAITING_FOR_BOOTSTRAP_PKG,
        WAITING_FOR_CONNECTION_CLOSED,
        SUCCESS,
        FAILED,
        CANCELLED
    };

    BootstrapInfo(const DeviceInfo &deviceInfo, const std::string &passphrase,
                  const std::string &bootstrapChannelId);

    DeviceInfo deviceInfo;
    std::atomic<State> state;

    RaceHandle prepareBootstrapHandle = NULL_RACE_HANDLE;
    RaceHandle createdLinkHandle = NULL_RACE_HANDLE;
    RaceHandle connectionHandle = NULL_RACE_HANDLE;

    std::string passphrase;
    std::string bootstrapChannelId;
    std::string bootstrapPath;
    std::string timeSinceEpoch;
    std::string bootstrapBundlePath;
    std::vector<std::string> commsPlugins;
    LinkID bootstrapLink;
    ConnectionID bootstrapConnection;
};

class BootstrapManager;
class BootstrapThread;
/**
 *  This class contains the interface for managing calls related to a specific ongoing bootstrap.
 */
class BootstrapInstanceManager {
public:
    explicit BootstrapInstanceManager(BootstrapManager &manager,
                                      std::shared_ptr<FileSystemHelper> fileSystemHelper);
    virtual ~BootstrapInstanceManager() = default;

    virtual RaceHandle handleBootstrapStart(BootstrapInfo &bootstrap);
    virtual void handleLinkCreated(BootstrapInfo &bootstrap, const LinkID &linkId);
    virtual void handleConnectionOpened(BootstrapInfo &bootstrap, const ConnectionID &connId);
    virtual void handleConnectionClosed(BootstrapInfo &bootstrap);
    virtual bool handleBootstrapPkgReceived(BootstrapInfo &bootstrap, const EncPkg &pkg,
                                            int32_t timeout);
    virtual void handleNMReady(const std::shared_ptr<BootstrapInfo> &bootstrap,
                               std::vector<std::string> commsChannels);
    virtual void handleLinkFailed(BootstrapInfo &bootstrap, const LinkID &linkId);
    virtual void handleNMFailed(BootstrapInfo &bootstrap);
    virtual void handleCancelled(BootstrapInfo &bootstrap);
    virtual void handleServeFilesFailed(BootstrapInfo &bootstrap);
    virtual void cleanupBootstrap(const std::shared_ptr<BootstrapInfo> &bootstrapInfo);

    BootstrapThread *getBootstrapThread() {
        return bootstrapThread.get();
    }

protected:
    RaceHandle prepareToBootstrap(BootstrapInfo &bootstrap);

    /**
     * @brief Create the directory structure for the bootstrap bundle. Note that this does not
     * populate any files, but simply creates the directories.
     *
     * @param bootstrap A boostrap info object. Not used as input to this function. Instead this
     * function will populate the bootstrapPath member of that object.
     * @return False if there is an error creating any of the directories, otherwise true.
     */
    bool createBootstrapDirectories(BootstrapInfo &bootstrap);
    bool populateGlobalDirectories(const BootstrapInfo &bootstrap);

    bool createBootstrapLink(BootstrapInfo &bootstrap);
    bool closeBootstrapConnection(BootstrapInfo &bootstrap);

    std::tuple<bool, std::string, RawData> parseBootstrapPkg(const EncPkg &bootstrapPkg);

    BootstrapManager &manager;
    std::shared_ptr<FileSystemHelper> fileSystemHelper;
    std::unique_ptr<BootstrapThread> bootstrapThread;
};

/**
 *  This class contains the interface for managing all bootstrap related calls. For calls relating
 *  to an ongoing bootstrap, it will identify that bootstrap and pass to BootstrapInstanceManager
 * for handling calls related to the specific ongoing bootstrap.
 */
class BootstrapManager {
public:
    explicit BootstrapManager(RaceSdk &sdk);
    // Used for unit tests only
    explicit BootstrapManager(RaceSdk &sdk, std::shared_ptr<FileSystemHelper> fileSystemHelper);

    virtual RaceHandle prepareToBootstrap(DeviceInfo deviceInfo, std::string passphrase,
                                          std::string bootstrapChannelId);
    virtual bool onLinkStatusChanged(RaceHandle handle, LinkID linkId, LinkStatus status,
                                     LinkProperties /*properties*/);
    virtual bool onConnectionStatusChanged(RaceHandle handle, ConnectionID connId,
                                           ConnectionStatus status, LinkProperties /*properties*/);
    virtual bool onReceiveEncPkg(const EncPkg &pkg, const LinkID &linkId, int32_t timeout);
    virtual bool bootstrapDevice(RaceHandle handle, std::vector<std::string> commsChannels);
    virtual bool bootstrapFailed(RaceHandle handle);
    virtual bool cancelBootstrap(RaceHandle handle);
    virtual bool onServeFilesFailed(const BootstrapInfo &failedBootstrap);

    virtual void removePendingBootstrap(const BootstrapInfo &failedBootstrap);

    BootstrapThread *getBootstrapThread();

protected:
    bool validateDeviceInfo(const DeviceInfo &deviceInfo);

    std::shared_ptr<BootstrapInstanceManager> bsInstanceManager;
    std::mutex bootstrapLock;
    std::vector<std::shared_ptr<BootstrapInfo>> bootstraps;
    std::shared_ptr<FileSystemHelper> fileSystemHelper;

public:
    RaceSdk &sdk;
};

#endif
