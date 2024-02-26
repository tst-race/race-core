
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

#ifndef __COMMS_TWOSIX_TRANSPORT_LINK_H__
#define __COMMS_TWOSIX_TRANSPORT_LINK_H__

#include <ComponentTypes.h>
#include <LinkProperties.h>
#include <PackageStatus.h>
#include <SdkResponse.h>  // RaceHandle

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "LinkAddress.h"
#include "MessageHashQueue.h"

class ITransportSdk;

/**
 * @brief A Instance of a link within the twoSixIndirectCpp transport
 */
class Link {
public:
    Link(LinkID linkId, LinkAddress address, LinkProperties properties, ITransportSdk *sdk);

    virtual ~Link();

    /**
     * @brief Get the ID of this link. This function is thread-safe.
     *
     * @return The ID of this link
     */
    virtual LinkID getId() const;

    /**
     * @brief Get the link properties of this link. This function is thread-safe.
     *
     * @return The properties of this link
     */
    virtual const LinkProperties &getProperties() const;

    /**
     * @brief Enqueue the given content to be posted to the whiteboard.
     *
     * @param actionId Unique ID of the post action
     * @param content Content to be posted
     * @return Component status enum
     */
    virtual ComponentStatus enqueueContent(uint64_t actionId, const std::vector<uint8_t> &content);

    /**
     * @brief Dequeue the content associated with the given action ID.
     *
     * @param actionId Unique ID of the post action
     * @return Component status enum
     */
    virtual ComponentStatus dequeueContent(uint64_t actionId);

    /**
     * @brief Polls the whiteboard for unread content.
     *
     * @param handles Action handles
     * @return Component status enum
     */
    virtual ComponentStatus fetch();

    /**
     * @brief Posts previously queued content to the whiteboard.
     *
     * @param handles Action handles
     * @param actionId Unique ID of the post action
     * @return Component status enum
     */
    virtual ComponentStatus post(std::vector<RaceHandle> handles, uint64_t actionId);

    /**
     * @brief Start the link.
     */
    virtual void start();

    /**
     * @brief Shutdown the link.
     */
    virtual void shutdown();

protected:
    virtual void postOnActionThread(const std::vector<RaceHandle> &handles, uint64_t actionId);
    virtual bool postToWhiteboard(const std::string &message);

    virtual int getInitialIndex();
    virtual int fetchOnActionThread(int latestIndex);
    virtual int getIndexFromTimestamp(double secondsSinceEpoch);
    virtual std::tuple<std::vector<std::string>, int, double> getNewPosts(int latestIndex);

    MessageHashQueue postedMessageHashes;

private:
    ITransportSdk *sdk;

    LinkID linkId;
    LinkAddress address;
    LinkProperties properties;

    struct QueuedAction {
        bool post;
        std::vector<RaceHandle> handles;
        uint64_t actionId;
    };

    std::thread thread;
    std::atomic<bool> isShutdown{false};
    std::mutex mutex;
    std::condition_variable conditionVariable;

    std::deque<QueuedAction> actionQueue;
    std::unordered_map<uint64_t, std::vector<uint8_t>> contentQueue;
    int fetchAttempts{0};

    void runActionThread();
    void updatePackageStatus(const std::vector<RaceHandle> &handles, PackageStatus status);
    std::string prependIdentifier(const std::string &key);
};

#endif  //  __COMMS_TWOSIX_TRANSPORT_LINK_H__