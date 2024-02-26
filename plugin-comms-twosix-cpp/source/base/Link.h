
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

#ifndef _LINK_H_
#define _LINK_H_

#include <IRacePluginComms.h>  // ConnectionID, LinkID, LinkProperties
#include <IRaceSdkComms.h>

#include <atomic>
#include <condition_variable>  // std::condition_variable
#include <deque>               // std::deque
#include <memory>              // std::shared_ptr
#include <mutex>               // std::mutex, std::lock_guard
#include <random>              // std::default_random_engine
#include <string>              // std::string
#include <thread>              // std::thread
#include <vector>              // std::vector

#include "../PluginCommsTwoSixCpp.h"
#include "Channel.h"
#include "LinkProfileParser.h"

class Connection;

class Link : public std::enable_shared_from_this<Link> {
protected:
    struct SendInfo {
        SendInfo() : handle(0), pkg(nullptr), timeoutTimestamp(0) {}
        SendInfo(RaceHandle _handle, const EncPkg &_pkg, double _timeoutTimestamp) :
            handle(_handle),
            pkg(std::make_shared<EncPkg>(_pkg)),
            timeoutTimestamp(_timeoutTimestamp) {}
        RaceHandle handle;
        std::shared_ptr<EncPkg> pkg;
        double timeoutTimestamp;
    };

    IRaceSdkComms *mSdk;
    PluginCommsTwoSixCpp *mPlugin;
    Channel *mChannel;

    std::default_random_engine mRnd;

    // must not access / modify any of the below members without grabbing linkLock. This could be
    // done in a more fine grained manner, but there's no need right now.
    std::mutex mLinkLock;

    const LinkID mId;
    LinkProperties mProperties;
    std::vector<std::shared_ptr<Connection>> mConnections;

    std::mutex mSendLock;
    std::condition_variable mSendThreadSignaler;

    std::deque<SendInfo> mSendQueue;

    std::atomic<bool> mShutdown;

    std::condition_variable mSendThreadShutdownSignaler;
    std::atomic<bool> mSendThreadShutdown;

    const double mSendPeriodLength;
    const uint32_t mSendPeriodAmount;
    const double mSleepPeriodLength;
    double mNextChange;
    double mNextSleepAmount;
    std::atomic<bool> mSleeping;

    const double mSendDropRate;
    const double mReceiveDropRate;
    const double mSendCorruptRate;
    const double mReceiveCorruptRate;
    const uint32_t mSendCorruptAmount;
    const uint32_t mReceiveCorruptAmount;
    const uint32_t mTraceCorruptSizeLimit;

    const size_t SEND_QUEUE_MAX_CAPACITY = 10;

    /**
     * @brief Send a package using this link. This method should get overridden by the indiviual
     * link implementations and will be called by the send thread when there's a package to process
     *
     * @param handle The handle to issue callbacks with
     * @param pkg The package to send.
     * @return false if the link should shutdown, otherwise true.
     */
    virtual bool sendPackageInternal(RaceHandle handle, const EncPkg &pkg) = 0;

    /**
     * @brief The entrance function for the send thread. This is static as the link it is
     * responsible for may be destroyed by this thread in case of error. Having the entrace function
     * be static prevents undefined behavior of running a method of a destroyed object.
     */
    static void runSendThread(Link *link);

    /**
     * @brief The entrance function for the send thread. This will call sendPackageInternal whenever
     * there's a package to send. It will exit and issue error callbacks for remaining packages if
     * the mShutdown flag is set
     *
     * @return true if the shutdown was intentional, false if it happened due to error.
     */
    virtual bool runSendThreadInternal();

    /**
     * @brief Overridable method to deal with subclass specific shutdown logic.
     */
    virtual void shutdownInternal() {}

    /**
     * @brief shutdown logic specific to this base class
     */
    void shutdownLink();

    /**
     * @brief potentially either corrupt or drop a package before sending it to the sdk
     */
    void receivePackageWithCorruption(const EncPkg &pkg, const std::vector<ConnectionID> &connIds,
                                      int32_t timeout);

public:
    /**
     * @brief Create a link representing a channel
     *
     * @return LinkID The id of this link
     */
    Link(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin, Channel *channel, const LinkID &linkId,
         const LinkProperties &linkProperties, const LinkProfileParser &parser);
    virtual ~Link() = 0;

    /**
     * @brief Create a new connection on this link. If this is a new receive link and there were no
     * previous receive links, start the monitor thread.
     *
     * @param linkType The type of link to open: send, receive, or bi-directional.
     * @param connectionId The ID to give the new connection.
     * @return std::shared_ptr<Connection> The new connection
     */
    virtual std::shared_ptr<Connection> openConnection(LinkType linkType,
                                                       const ConnectionID &connectionId,
                                                       const std::string &linkHints,
                                                       int timeout) = 0;

    /**
     * @brief Close and remove a connection that uses this link. If there are no remaining open
     * receive connections, close the monitor thread
     *
     * @param connectionId The ID of the connection to close.
     */
    virtual void closeConnection(const ConnectionID &connectionId) = 0;

    /**
     * @brief Start the connection. No packages should be received on the connection before this is
     * called.
     *
     * @param connection The connection to start.
     */
    virtual void startConnection(Connection *connection) = 0;

    /**
     * @brief Send a package using this link. This method merely puts the package in an internal
     * queue to be sent on the send thread by sendPackageInternal
     *
     * @param handle The handle to issue callbacks with
     * @param pkg The package to send.
     */
    virtual PluginResponse sendPackage(RaceHandle handle, const EncPkg &pkg,
                                       double timeoutTimestamp);

    /**
     * @brief Serve the files from the directory given in path.
     *
     * @param path A path to the directory containing the files to serve
     * @return PluginResponse status in response to the call
     */
    virtual PluginResponse serveFiles(std::string path);

    /**
     * @brief Shutdown the link. This will call shutdown internal to deal with subclass specific
     * shutdown logic
     */
    virtual void shutdown();

    // Getters
    /**
     * @brief Get the id of the link. This function is thread-safe.
     *
     * @return LinkID The id of this link
     */
    virtual LinkID getId();

    /**
     * @brief Get the link properties of this link. This function is thread-safe.
     *
     * @return LinkProperties The properties of this link
     */
    virtual LinkProperties getProperties();

    /**
     * @brief Get the list of connections for the link. This function is thread-safe.
     *
     * @return std::vector<std::shared_ptr<Connection>> The connections that use this link
     */
    virtual std::vector<std::shared_ptr<Connection>> getConnections();

    /**
     * @brief Get the link address.
     *
     * @return std::string The link address.
     */
    virtual std::string getLinkAddress() = 0;

    /**
     * @brief Is this link currently available to send packages on
     *
     * @return bool true if this link can currently send packages
     */
    virtual bool isAvailable();

private:
    // simple predicates that indicate if there's work to do
    bool shouldSleep();
    bool shouldWake();
    bool shouldSend();

    /**
     * @brief go to sleep and stop sending packages until sleep period has passed. Times out any
     * packages that would timeout before waking up. Caller should hold send lock.
     */
    void goSleep();

    /**
     * @brief schedule next sleep and start sending packages again.
     */
    void wakeUp();

    /**
     * @brief potentially either corrupt or drop a package before sending it
     */
    bool sendPackageWithCorruption(RaceHandle handle, const EncPkg &pkg);

    /**
     * @brief Take a input package and return a corrupted version
     */
    EncPkg corruptPackage(const EncPkg &pkg, uint32_t corruptAmount);

    /**
     * @brief return a (trucated) base64 encoded ciphertext for logging / display
     */
    std::string getCipherTextForDisplay(const EncPkg &pkg);
};

#endif
