
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

#include "TwosixWhiteboardLink.h"

#include <RaceLog.h>

#include <algorithm>  // std::find_if
#include <chrono>     // std::chrono::seconds
#include <nlohmann/json.hpp>
#include <sstream>  // std::ostringstream

#include "../base/Connection.h"
#include "../utils/PersistentStorageHelpers.h"
#include "../utils/base64.h"
#include "../utils/log.h"
#include "curlwrap.h"

using json = nlohmann::json;

const std::deque<std::size_t>::size_type maxNumHashes =
    1024;  // Upper bound for num of hashes stored in mOwnPostHashes

/**
 * @brief callback function required by libcurl-dev.
 * See documentation in link below:
 * https://curl.haxx.se/libcurl/c/libcurl-tutorial.html
 */
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    (static_cast<std::string *>(userp))->append(static_cast<char *>(contents), size * nmemb);
    return size * nmemb;
}

TwosixWhiteboardLink::MonitorState::MonitorState(TwosixWhiteboardLink *link, double timestamp) :
    mLink(std::dynamic_pointer_cast<TwosixWhiteboardLink>(link->shared_from_this())),
    mShouldStop(false),
    mTimestamp(timestamp) {}

TwosixWhiteboardLink::TwosixWhiteboardLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin,
                                           Channel *channel, const LinkID &linkId,
                                           const LinkProperties &linkProperties,
                                           const TwosixWhiteboardLinkProfileParser &parser) :
    Link(sdk, plugin, channel, linkId, linkProperties, parser),
    mHostname(parser.hostname),
    mPort(parser.port),
    mTag(parser.hashtag),
    mConfigPeriod(parser.checkFrequency),
    mCheckPeriod(0),
    mLinkTimestamp(parser.timestamp),
    maxTries(parser.maxTries) {
    mProperties.linkAddress = this->TwosixWhiteboardLink::getLinkAddress();
}

TwosixWhiteboardLink::TwosixWhiteboardLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin,
                                           Channel *channel, const LinkID &linkId,
                                           const LinkProperties &linkProperties,
                                           const std::string &linkAddress) :
    TwosixWhiteboardLink(sdk, plugin, channel, linkId, linkProperties,
                         TwosixWhiteboardLinkProfileParser(linkAddress)) {}

void TwosixWhiteboardLink::shutdownInternal() {
    auto properties = getProperties();
    auto connections = getConnections();
    for (auto &connection : connections) {
        closeConnection(connection->connectionId);
        mSdk->onConnectionStatusChanged(NULL_RACE_HANDLE, connection->connectionId,
                                        CONNECTION_CLOSED, properties, RACE_BLOCKING);
    }
}

TwosixWhiteboardLink::~TwosixWhiteboardLink() {
    shutdownLink();
}

std::shared_ptr<Connection> TwosixWhiteboardLink::openConnection(LinkType linkType,
                                                                 const ConnectionID &connectionId,
                                                                 const std::string &linkHints,
                                                                 int timeout) {
    const std::string loggingPrefix = "TwosixWhiteboardLink::openConnection (" + mId + "): ";
    logInfo(loggingPrefix + " called");

    if (mShutdown) {
        logInfo(loggingPrefix + " Cannot open connection because link is shutting down");
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mLinkLock);

    auto connection = std::make_shared<Connection>(connectionId, linkType, shared_from_this(),
                                                   linkHints, timeout);

    mConnections.push_back(connection);

    return connection;
}

void TwosixWhiteboardLink::closeConnection(const ConnectionID &connectionId) {
    std::lock_guard<std::mutex> lock(mLinkLock);

    auto connectionIter =
        std::find_if(mConnections.begin(), mConnections.end(),
                     [&connectionId](const std::shared_ptr<Connection> &connection) {
                         return connectionId == connection->connectionId;
                     });

    if (connectionIter == mConnections.end()) {
        return;
    }

    LinkType lt = (*connectionIter)->linkType;
    mConnections.erase(connectionIter);

    if (lt == LT_RECV || lt == LT_BIDI) {
        bool hasReceiveConnection = std::any_of(
            mConnections.begin(), mConnections.end(), [](const std::shared_ptr<Connection> &conn) {
                return conn->linkType == LT_RECV || conn->linkType == LT_BIDI;
            });

        if (!hasReceiveConnection && mMonitorState != nullptr) {
            mMonitorState->mShouldStop = true;
            mMonitorState->mStopCV.notify_one();
            mMonitorState = nullptr;
        }
    }
}

void TwosixWhiteboardLink::removeHashFromDeque(std::size_t hash) {
    std::lock_guard<std::mutex> lock(hashMutex);
    std::deque<std::size_t>::iterator hashPos =
        std::find(mOwnPostHashes.begin(), mOwnPostHashes.end(), hash);
    if (hashPos != mOwnPostHashes.end()) {
        mOwnPostHashes.erase(hashPos);
    }
}

void TwosixWhiteboardLink::startConnection(Connection *connection) {
    const std::string loggingPrefix =
        "TwosixWhiteboardLink::startConnection (" + connection->connectionId + "): ";

    json hintsJson;
    if (connection->linkHints.size() > 0) {
        try {
            hintsJson = json::parse(connection->linkHints);
        } catch (json::exception &e) {
            logWarning("Error parsing LinkHints JSON, ignoring for this connection");
            hintsJson = json::object();
        }
    } else {
        hintsJson = json::object();
    }

    std::lock_guard<std::mutex> lock(hashMutex);

    if (connection->linkType == LT_BIDI || connection->linkType == LT_RECV) {
        // get the polling interval hint, defaulting to the value from the config if it doesn't
        // exist
        int newPollingInterval;
        try {
            newPollingInterval = hintsJson.at("polling_interval_ms").get<int>();
        } catch (...) {
            newPollingInterval = mConfigPeriod;
        }

        double timestamp = mLinkTimestamp;
        if (mLinkTimestamp == -1.0) {
            try {
                timestamp = hintsJson.at("after").get<double>();
            } catch (...) {
            }
        }

        // if the receiving thread doesn't yet exist, we have to create the monitor thread
        if (!mMonitorState) {
            // we can raise the polling interval if no other connections exist
            mCheckPeriod.store(newPollingInterval);

            logDebug(loggingPrefix + "creating thread for receiving link ID: " + mId);
            logInfo(loggingPrefix + "polling interval: " + std::to_string(newPollingInterval) +
                    " ms");
            mMonitorState = std::make_shared<MonitorState>(this, timestamp);
            std::thread(&TwosixWhiteboardLink::runMonitor, mMonitorState).detach();
        } else {
            // if another connections exists, we are only allowed to lower the interval
            if (newPollingInterval < mCheckPeriod) {
                mCheckPeriod.store(newPollingInterval);
                logInfo("Overwriting old period with new hint value: \"polling_interval_ms\": " +
                        std::to_string(newPollingInterval));
            }

            logDebug(loggingPrefix + "Link " + mId + " already open. Reusing link for connection " +
                     connection->connectionId + ".");
        }
    }
}

bool TwosixWhiteboardLink::sendPackageInternal(RaceHandle handle, const EncPkg &pkg) {
    const std::string loggingPrefix = "TwosixWhiteboardLink::sendPackage (" + mId + "): ";
    logInfo(loggingPrefix + " called");

    std::string postUrl = "http://" + mHostname + ":" + std::to_string(mPort) + "/post/" + mTag;

    RawData rawData = pkg.getRawData();

    uint64_t base64Length = 4 * ((rawData.size() + 1) / 3);  // pessimistic approximation
    std::string pkgData = base64::encode(pkg.getRawData());
    std::size_t pkgHash = std::hash<std::string>()(pkgData);
    std::string postData;
    postData.reserve(base64Length + 14);
    postData.append("{ \"data\":\"");
    postData.append(pkgData);
    postData.append("\" }");

    // TODO: RAII this thing
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    int tries = 0;
    for (; tries < maxTries; tries++) {
        try {
            CurlWrap curl;
            std::string response;

            logDebug(loggingPrefix + "Attempting to post to: " + postUrl);

            curl.setopt(CURLOPT_URL, postUrl.c_str());
            curl.setopt(CURLOPT_HTTPPOST, 1L);

            // connecton timeout. override the default and set to 10 seconds.
            curl.setopt(CURLOPT_CONNECTTIMEOUT, 10L);

            curl.setopt(CURLOPT_WRITEFUNCTION, WriteCallback);
            curl.setopt(CURLOPT_WRITEDATA, &response);
            curl.setopt(CURLOPT_HTTPHEADER, headers);
            curl.setopt(CURLOPT_POSTFIELDS, postData.c_str());
            curl.setopt(CURLOPT_POSTFIELDSIZE, postData.size());

            {
                std::lock_guard<std::mutex> lock(hashMutex);
                if (mOwnPostHashes.size() > maxNumHashes) {
                    logWarning(loggingPrefix + "Max size reached for hash deque, dropping oldest");
                    mOwnPostHashes.pop_front();
                }
                mOwnPostHashes.push_back(
                    pkgHash);  // assuming post success to avoid a race condition
            }

            curl.perform();

            if (response.find("index") != std::string::npos) {
                logDebug(loggingPrefix + "Post successful: " + response);
                break;
            } else {
                logWarning(loggingPrefix + "Unknown reponse: " + response);
                removeHashFromDeque(pkgHash);  // need to correct assumption if post fails
            }
        } catch (curl_exception &error) {
            // Log once ever 30 seconds (assuming the retries are done every second)
            if (tries % 30 == 0) {
                logWarning(loggingPrefix + "curl exception: " + std::string(error.what()));
            }
            removeHashFromDeque(pkgHash);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    curl_slist_free_all(headers);
    if (tries == maxTries) {
        logError(loggingPrefix + "Retry limit exceeded: post failed");
        mSdk->onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING);

        // fail and tell the send thread to close the link
        return false;
    } else {
        mSdk->onPackageStatusChanged(handle, PACKAGE_SENT, RACE_BLOCKING);
    }
    logInfo(loggingPrefix + " returned");
    return true;
}

void TwosixWhiteboardLink::runMonitor(std::shared_ptr<MonitorState> monitorState) {
    if (!monitorState->mLink->runMonitorInternal(*monitorState)) {
        logError("TwosixWhiteboardLink::runMonitor: Monitor failed, destroying link");
        monitorState->mLink->mPlugin->destroyLink(0, monitorState->mLink->mId);
    }
}

bool TwosixWhiteboardLink::runMonitorInternal(MonitorState &monitorState) {
    const std::string loggingPrefix = "TwosixWhiteboardLink::runMonitorInternal (" + mId + "): ";
    logInfo(loggingPrefix + ": called. hostname: " + mHostname + ", tag: " + mTag +
            ", checkPeriod: " + std::to_string(mCheckPeriod));

    // Read last recorded timestamp - if there is none, use the value of the LinkAddress or Hint
    // If there is nothing, then just start from <now>
    double lastTimestamp = psh::readValue(mSdk, prependIdentifier("lastTimestamp"), -1.0);
    if (lastTimestamp > 0) {
        monitorState.mTimestamp = lastTimestamp;
    } else if (monitorState.mTimestamp < 0) {
        std::chrono::duration<double> sinceEpoch =
            std::chrono::high_resolution_clock::now().time_since_epoch();
        double now = sinceEpoch.count();
        monitorState.mTimestamp = now;
    } else {
        logDebug(loggingPrefix +
                 "Using timestamp hint/address value: " + std::to_string(monitorState.mTimestamp));
    }
    int latest = getIndexFromTimestamp(monitorState.mTimestamp);

    auto period = std::chrono::milliseconds(mCheckPeriod);
    int tries = 0;
    while (!monitorState.mShouldStop) {
        auto checkTime = std::chrono::system_clock::now();

        try {
            auto [posts, newLatest, serverTimestamp] = getNewPosts(latest);

            int numPosts = static_cast<int>(posts.size());
            if (numPosts < newLatest - latest) {
                logError("Expected " + std::to_string(newLatest - latest) +
                         " posts, but only got " + std::to_string(numPosts) + ". " +
                         std::to_string(newLatest - latest - numPosts) +
                         " posts may have been lost.");
            }

            latest = newLatest;

            std::vector<ConnectionID> connIds;

            for (const auto &conn : getConnections()) {
                // cppcheck-suppress useStlAlgorithm
                connIds.push_back(conn->connectionId);
            }

            for (const auto &post : posts) {
                try {
                    std::lock_guard<std::mutex> lock(hashMutex);
                    std::deque<std::size_t>::iterator hashPos =
                        std::find(mOwnPostHashes.begin(), mOwnPostHashes.end(),
                                  std::hash<std::string>()(post));
                    if (hashPos == mOwnPostHashes.end()) {
                        EncPkg package(base64::decode(post));
                        logDebug(loggingPrefix + ": Received encrypted package");
                        receivePackageWithCorruption(package, connIds, RACE_BLOCKING);
                    } else {
                        logDebug(loggingPrefix + ": Received post from self, ignoring");
                        mOwnPostHashes.erase(mOwnPostHashes.begin(), hashPos + 1);
                    }
                } catch (std::invalid_argument &error) {
                    logError("Package had invalid base64 encoding, skipping");
                }
            }

            if (numPosts > 0) {
                psh::saveValue(mSdk, prependIdentifier("lastTimestamp"), serverTimestamp);
            }

            tries = 0;
        } catch (curl_exception &error) {
            if (tries % 30 == 0) {
                logWarning(loggingPrefix + "curl exception: " + std::string(error.what()));
            }
            tries++;
        } catch (json::exception &error) {
            logError(loggingPrefix + "json exception: " + std::string(error.what()));
            tries++;
        } catch (std::exception &error) {
            logError(loggingPrefix + "std exception: " + std::string(error.what()));
            tries++;
        }

        if (tries >= maxTries) {
            logError(loggingPrefix + "Retry limit reached. Giving up.");
            break;
        }

        std::unique_lock<std::mutex> lock(monitorState.mStopMutex);
        if (monitorState.mStopCV.wait_until(lock, checkTime + period, [&monitorState] {
                return monitorState.mShouldStop.load();
            })) {
            break;
        }
    }

    logInfo(loggingPrefix + " returned");
    return monitorState.mShouldStop;
}

int TwosixWhiteboardLink::getIndexFromTimestamp(double secondsSinceEpoch) {
    const std::string loggingPrefix = "TwosixWhiteboardLink::getIndexFromTimestamp (" + mId + "): ";

    std::string postUrl = "http://" + mHostname + ":" + std::to_string(mPort) + "/after/" + mTag +
                          "/" + std::to_string(secondsSinceEpoch);

    // return 0 if error
    int index = 0;
    try {
        CurlWrap curl;
        std::string response;

        logDebug(loggingPrefix + "Attempting to get post by timestamp from: " + postUrl);

        curl.setopt(CURLOPT_URL, postUrl.c_str());
        curl.setopt(CURLOPT_WRITEFUNCTION, WriteCallback);
        curl.setopt(CURLOPT_WRITEDATA, &response);
        curl.perform();

        index = json::parse(response).at("index").get<int>();
        logDebug(loggingPrefix + "Got index: " + std::to_string(index));

    } catch (curl_exception &error) {
        logError(loggingPrefix + "curl exception: " + std::string(error.what()));
    } catch (json::exception &error) {
        logError(loggingPrefix + "json exception: " + std::string(error.what()));
    } catch (std::exception &error) {
        logError(loggingPrefix + "std exception: " + std::string(error.what()));
    }

    return index;
}

std::tuple<std::vector<std::string>, int, double> TwosixWhiteboardLink::getNewPosts(int oldest) {
    // get all posts after (and including) oldest
    std::string postUrl = "http://" + mHostname + ":" + std::to_string(mPort) + "/get/" + mTag +
                          "/" + std::to_string(oldest) + "/-1";

    CurlWrap curl;
    std::string response;

    // logDebug("TwosixWhiteboardLink::getNewPosts (" + mId + "): Attempting to get posts from: " +
    // postUrl);

    curl.setopt(CURLOPT_URL, postUrl.c_str());
    curl.setopt(CURLOPT_WRITEFUNCTION, WriteCallback);
    curl.setopt(CURLOPT_WRITEDATA, &response);
    curl.perform();

    auto responseJson = json::parse(response);

    return {responseJson.at("data"), responseJson.at("length"),
            std::stod(responseJson.at("timestamp").get<std::string>())};
}

std::string TwosixWhiteboardLink::prependIdentifier(const std::string &key) {
    return key + ":" + mHostname + ":" + std::to_string(mPort) + ":" + mTag;
}

std::string TwosixWhiteboardLink::getLinkAddress() {
    nlohmann::json address;
    address["hostname"] = mHostname;
    address["port"] = mPort;
    address["checkFrequency"] = mConfigPeriod;
    address["hashtag"] = mTag;
    address["timestamp"] = mLinkTimestamp;
    address["maxTries"] = maxTries;
    return address.dump();
}
