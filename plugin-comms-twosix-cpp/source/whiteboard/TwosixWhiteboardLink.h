
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

#ifndef _TWOSIX_WHITEBOARD_LINK_
#define _TWOSIX_WHITEBOARD_LINK_

#include <atomic>  // std::atomic
#include <deque>   // std::deque
#include <mutex>   // std::mutex, std::lock_guard
#include <thread>  // std::thread
#include <tuple>   // std::vector
#include <vector>  // std::vector

#include "../PluginCommsTwoSixCpp.h"
#include "../base/Link.h"
#include "TwosixWhiteboardLinkProfileParser.h"

class TwosixWhiteboardLink : public Link {
protected:
    struct MonitorState {
        std::shared_ptr<TwosixWhiteboardLink> mLink;

        // condition variable to allow notification that we want to stop the monitor thread
        std::condition_variable mStopCV;
        std::mutex mStopMutex;

        // whether the monitorThread should stop
        std::atomic<bool> mShouldStop;

        // The timestamp to start reading from
        double mTimestamp;

        MonitorState(TwosixWhiteboardLink *link, double timestamp);
    };

    /**
     * @brief The function that runs on the monitor thread to receive posts. This is a static
     * function because we shut down and destroy the link if the whiteboard goes down, so if `this`
     * pointed to the link it would be undefined behavior.
     *
     * @param double link The link this monitor thread is for.
     * @param double timestamp The timestamp to start reading posts from
     */
    static void runMonitor(std::shared_ptr<MonitorState> monitorState);

    /**
     * @brief The function that runs on the monitor thread to receive posts.
     *
     * @param double timestamp The timestamp to start reading posts from
     * @return true if shutdown was expected, false if it was due to errors
     */
    bool runMonitorInternal(MonitorState &monitorState);

    /**
     * @brief get the index of the first post after timestamp, or the next post if there are no
     * posts after timestamp
     *
     * @param double secondsSinceEpoch The timestamp in seconds. May be fractional
     * @return int The index of the first post to the whiteboard after timestamp
     *
     */
    int getIndexFromTimestamp(double secondsSinceEpoch);

    /**
     * @brief Get any posts after index oldest
     *
     * @param oldest The index of the oldest post to fetch
     * @return tuple containing the list of posts, the total number of posts since oldest, and the
     * server timestamp. The length of the list may not equal the number of posts since oldest in
     * the case that the whiteboard has dropped posts in order to prevent itself from filling up
     */
    std::tuple<std::vector<std::string>, int, double> getNewPosts(int oldest);

    /**
     * @brief Prepend a string that uniquely identifies this link. This identifier is not based on
     * LinkId and is instead based on information in the link profile
     *
     * @param key The key to prepend to
     * @return std::string key, with a string that uniquely identifies this link prepended to it
     */
    std::string prependIdentifier(const std::string &key);

    // The hostname of the whiteboard
    const std::string mHostname;

    // The port to connect to the whiteboard on
    const int mPort;

    // The category this link uses on the whiteboard
    const std::string mTag;

    // The default period, in milliseconds, between polls to the whiteboard to check for new posts
    const int mConfigPeriod;

    // The currently set period in milliseconds between polls to the whiteboard to check for new
    // posts
    std::atomic<int> mCheckPeriod;

    // The start timestamp for which to check for messages (can be overridden for specific
    // connections with link hints)
    const double mLinkTimestamp;

    // Maximum number of retries before issuing a failure report
    const int maxTries;

    // The state of the monitor thread
    std::shared_ptr<MonitorState> mMonitorState;

    // The hashes of posted message contents to identify posts sent by self. oldest -> newest
    std::deque<std::size_t> mOwnPostHashes;
    std::mutex hashMutex;

    /**
     * @brief Send a package using this link.
     *
     * @param EncPkg the package to send.
     * @return false if the link should shutdown, otherwise true.
     */
    virtual bool sendPackageInternal(RaceHandle handle, const EncPkg &pkg) override;

    /**
     * @brief Helper function to remove only the first hash from the mOwnPostHashes deque
     *
     * @param hash The hash to find and remove
     */
    void removeHashFromDeque(std::size_t hash);

    /**
     * @brief Overridable method to deal with subclass specific shutdown logic.
     */
    virtual void shutdownInternal() override;

public:
    /**
     * @brief Construct a link representing communication using the twosix whiteboard
     *
     * @param sdk Pointer to the RACE sdk instance
     * @param linkId The id of this link
     * @param linkProperties The properties of this link
     * @param hostname The hostname of the whiteboard this link uses to communicate
     * @param port The port of the whiteboard this link uses to communicate
     * @param tag The tag of the whiteboard this link uses to communicate
     * @param period The period in milliseconds between polls to the whiteboard to check for new
     * posts
     */
    TwosixWhiteboardLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin, Channel *channel,
                         const LinkID &linkId, const LinkProperties &linkProperties,
                         const TwosixWhiteboardLinkProfileParser &parser);

    /**
     * @brief Construct a link representing communication using the twosix whiteboard
     *
     * @param sdk Pointer to the RACE sdk instance
     * @param linkId The id of this link
     * @param linkProperties The properties of this link
     * @param linkAddress The link address used to create the link
     */
    TwosixWhiteboardLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin, Channel *channel,
                         const LinkID &linkId, const LinkProperties &linkProperties,
                         const std::string &linkAddress);

    virtual ~TwosixWhiteboardLink();

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
                                                       int timeout) override;

    /**
     * @brief Close and remove a connection that uses this link. If there are no remaining open
     * receive connections, close the monitor thread
     *
     * @param connectionId The ID of the connection to close.
     */
    virtual void closeConnection(const ConnectionID &connectionId) override;

    /**
     * @brief Start the connection. No packages should be received on the connection before this is
     * called.
     *
     * @param connection The connection to start.
     */
    virtual void startConnection(Connection *connection) override;

    /**
     * @brief Get the link address.
     *
     * @return std::string The link address.
     */
    virtual std::string getLinkAddress() override;
};

#endif
