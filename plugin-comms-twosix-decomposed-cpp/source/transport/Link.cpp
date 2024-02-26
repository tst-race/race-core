
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

#include "Link.h"

#include <ITransportSdk.h>

#include <chrono>
#include <nlohmann/json.hpp>

#include "PersistentStorageHelpers.h"
#include "base64.h"
#include "curlwrap.h"
#include "log.h"

static const size_t ACTION_QUEUE_MAX_CAPACITY = 10;

namespace std {
static std::ostream &operator<<(std::ostream &out, const std::vector<RaceHandle> &handles) {
    return out << nlohmann::json(handles).dump();
}
}  // namespace std

Link::Link(LinkID linkId, LinkAddress address, LinkProperties properties, ITransportSdk *sdk) :
    sdk(sdk),
    linkId(std::move(linkId)),
    address(std::move(address)),
    properties(std::move(properties)) {
    this->properties.linkAddress = nlohmann::json(this->address).dump();
}

Link::~Link() {
    TRACE_METHOD(linkId);
    shutdown();
}

LinkID Link::getId() const {
    return linkId;
}

const LinkProperties &Link::getProperties() const {
    return properties;
}

ComponentStatus Link::enqueueContent(uint64_t actionId, const std::vector<uint8_t> &content) {
    TRACE_METHOD(linkId, actionId);
    {
        std::lock_guard<std::mutex> lock(mutex);
        contentQueue[actionId] = content;
    }
    return COMPONENT_OK;
}

ComponentStatus Link::dequeueContent(uint64_t actionId) {
    TRACE_METHOD(linkId, actionId);
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto iter = contentQueue.find(actionId);
        if (iter != contentQueue.end()) {
            contentQueue.erase(iter);
        }
    }
    return COMPONENT_OK;
}

ComponentStatus Link::fetch() {
    TRACE_METHOD(linkId);

    if (isShutdown) {
        logError(logPrefix + "link has been shutdown: " + linkId);
        return COMPONENT_ERROR;
    }

    std::lock_guard<std::mutex> lock(mutex);

    if (actionQueue.size() >= ACTION_QUEUE_MAX_CAPACITY) {
        logError(logPrefix + "action queue full for link: " + linkId);
        return COMPONENT_ERROR;
    }

    actionQueue.push_back({false, {}, 0});
    conditionVariable.notify_one();
    return COMPONENT_OK;
}

ComponentStatus Link::post(std::vector<RaceHandle> handles, uint64_t actionId) {
    TRACE_METHOD(linkId, handles, actionId);

    if (isShutdown) {
        logError(logPrefix + "link has been shutdown: " + linkId);
        updatePackageStatus(handles, PACKAGE_FAILED_GENERIC);
        return COMPONENT_ERROR;
    }

    std::lock_guard<std::mutex> lock(mutex);

    if (actionQueue.size() >= ACTION_QUEUE_MAX_CAPACITY) {
        logError(logPrefix + "action queue full for link: " + linkId);
        updatePackageStatus(handles, PACKAGE_FAILED_GENERIC);
        return COMPONENT_ERROR;
    }

    if (contentQueue.find(actionId) == contentQueue.end()) {
        // TODO: what's the correct log level. We want it to be an error(?) for performer encodings,
        // but this is expected for our own comms plugin.
        logInfo(logPrefix + "no enqueued content for given action ID: " + std::to_string(actionId));
        updatePackageStatus(handles, PACKAGE_FAILED_GENERIC);
        return COMPONENT_OK;
    }

    actionQueue.push_back({true, std::move(handles), actionId});
    conditionVariable.notify_one();
    return COMPONENT_OK;
}

void Link::start() {
    TRACE_METHOD(linkId);
    thread = std::thread(&Link::runActionThread, this);
}

void Link::shutdown() {
    TRACE_METHOD(linkId);
    isShutdown = true;
    conditionVariable.notify_one();
    if (thread.joinable()) {
        thread.join();
    }
}

void Link::runActionThread() {
    TRACE_METHOD(linkId);
    logPrefix += linkId + ": ";

    int latest = getInitialIndex();

    while (not isShutdown) {
        std::unique_lock<std::mutex> lock(mutex);
        conditionVariable.wait(lock, [this] { return isShutdown or not actionQueue.empty(); });

        if (isShutdown) {
            logDebug(logPrefix + "shutting down");
            break;
        }

        auto action = actionQueue.front();
        actionQueue.pop_front();

        if (action.post) {
            postOnActionThread(action.handles, action.actionId);
        } else {
            latest = fetchOnActionThread(latest);
        }
    }
}

int Link::getInitialIndex() {
    TRACE_METHOD(linkId);
    logPrefix += linkId + ": ";

    // TODO check link hints for timestamp
    double timestamp = psh::readValue(sdk, prependIdentifier("lastTimestamp"), -1.0);
    if (timestamp > 0) {
        logDebug(logPrefix + "using last recorded timestamp: " + std::to_string(timestamp));
    } else if (address.timestamp <= 0) {
        std::chrono::duration<double> sinceEpoch =
            std::chrono::high_resolution_clock::now().time_since_epoch();
        timestamp = sinceEpoch.count();
        logDebug(logPrefix + "using now for timestamp: " + std::to_string(timestamp));
    } else {
        timestamp = address.timestamp;
        logDebug(logPrefix + "using address timestamp: " + std::to_string(timestamp));
    }

    return getIndexFromTimestamp(timestamp);
}

int Link::fetchOnActionThread(int latestIndex) {
    TRACE_METHOD(linkId, latestIndex);
    logPrefix += linkId + ": ";

    try {
        auto [posts, newLatestIndex, serverTimestamp] = getNewPosts(latestIndex);

        int numPosts = static_cast<int>(posts.size());
        if (numPosts < newLatestIndex - latestIndex) {
            logError(logPrefix + "expected " + std::to_string(newLatestIndex - latestIndex) +
                     " posts, but only got " + std::to_string(numPosts) + ". " +
                     std::to_string(newLatestIndex - latestIndex - numPosts) +
                     " posts may have been lost.");
        }

        for (const auto &post : posts) {
            if (postedMessageHashes.findAndRemoveMessage(post)) {
                logDebug(logPrefix + "received post from self, ignoring");
            } else {
                logDebug(logPrefix + "received encrypted package");
                std::vector<uint8_t> message = base64::decode(post);
                sdk->onReceive(linkId, {linkId, "*/*", false, {}}, message);
            }
        }

        if (numPosts > 0) {
            psh::saveValue(sdk, prependIdentifier("lastTimestamp"), serverTimestamp);
        }

        fetchAttempts = 0;

        return newLatestIndex;
    } catch (curl_exception &error) {
        logError(logPrefix + "curl exception: " + std::string(error.what()));
    } catch (nlohmann::json::exception &error) {
        logError(logPrefix + "json exception: " + std::string(error.what()));
    } catch (std::exception &error) {
        logError(logPrefix + "std exception: " + std::string(error.what()));
    }

    ++fetchAttempts;
    if (fetchAttempts >= address.maxTries) {
        logError(logPrefix + "Retry limit reached. Giving up.");
        sdk->updateState(COMPONENT_STATE_FAILED);
    }

    return latestIndex;
}

/**
 * @brief callback function required by libcurl-dev.
 * See documentation in link below:
 * https://curl.haxx.se/libcurl/c/libcurl-tutorial.html
 */
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    (static_cast<std::string *>(userp))->append(static_cast<char *>(contents), size * nmemb);
    return size * nmemb;
}

int Link::getIndexFromTimestamp(double secondsSinceEpoch) {
    TRACE_METHOD(linkId, secondsSinceEpoch);
    logPrefix += linkId + ": ";

    std::string url = "http://" + address.hostname + ":" + std::to_string(address.port) +
                      "/after/" + address.hashtag + "/" + std::to_string(secondsSinceEpoch);

    // return 0 if error
    int index = 0;
    try {
        CurlWrap curl;
        std::string response;

        logDebug(logPrefix + "Attempting to get post by timestamp from: " + url);

        curl.setopt(CURLOPT_URL, url.c_str());
        curl.setopt(CURLOPT_WRITEFUNCTION, WriteCallback);
        curl.setopt(CURLOPT_WRITEDATA, &response);
        curl.perform();

        index = nlohmann::json::parse(response).at("index").get<int>();
        logDebug(logPrefix + "Got index: " + std::to_string(index));

    } catch (curl_exception &error) {
        logError(logPrefix + "curl exception: " + std::string(error.what()));
    } catch (nlohmann::json::exception &error) {
        logError(logPrefix + "json exception: " + std::string(error.what()));
    } catch (std::exception &error) {
        logError(logPrefix + "std exception: " + std::string(error.what()));
    }

    return index;
}

std::tuple<std::vector<std::string>, int, double> Link::getNewPosts(int latestIndex) {
    TRACE_METHOD(linkId, latestIndex);
    logPrefix += linkId + ": ";

    // Get all posts after (and including) oldest
    std::string url = "http://" + address.hostname + ":" + std::to_string(address.port) + "/get/" +
                      address.hashtag + "/" + std::to_string(latestIndex) + "/-1";

    CurlWrap curl;
    std::string response;

    curl.setopt(CURLOPT_URL, url.c_str());
    curl.setopt(CURLOPT_WRITEFUNCTION, WriteCallback);
    curl.setopt(CURLOPT_WRITEDATA, &response);
    curl.perform();

    auto responseJson = nlohmann::json::parse(response);

    return {responseJson.at("data"), responseJson.at("length"),
            std::stod(responseJson.at("timestamp").get<std::string>())};
}

void Link::postOnActionThread(const std::vector<RaceHandle> &handles, uint64_t actionId) {
    TRACE_METHOD(linkId, handles, actionId);
    logPrefix += linkId + ": ";

    auto iter = contentQueue.find(actionId);
    if (iter == contentQueue.end()) {
        // We really shouldn't get here, since we already check for this before queueing the action,
        // but just in case...
        logError(logPrefix +
                 "no enqueued content for given action ID: " + std::to_string(actionId));
        updatePackageStatus(handles, PACKAGE_FAILED_GENERIC);
        return;
    }

    std::string message = base64::encode(iter->second);
    auto msgHash = postedMessageHashes.addMessage(message);

    int tries = 0;
    for (; tries < address.maxTries; ++tries) {
        if (postToWhiteboard(message)) {
            break;
        }
    }

    if (tries == address.maxTries) {
        logError(logPrefix + "retry limit exceeded: post failed");
        postedMessageHashes.removeHash(msgHash);
        updatePackageStatus(handles, PACKAGE_FAILED_GENERIC);
    } else {
        updatePackageStatus(handles, PACKAGE_SENT);
    }
}

void Link::updatePackageStatus(const std::vector<RaceHandle> &handles, PackageStatus status) {
    for (auto &handle : handles) {
        sdk->onPackageStatusChanged(handle, status);
    }
}

bool Link::postToWhiteboard(const std::string &message) {
    TRACE_METHOD(linkId);
    logPrefix += linkId + ": ";
    bool success = false;

    std::string url = "http://" + address.hostname + ":" + std::to_string(address.port) + "/post/" +
                      address.hashtag;

    std::string postData;
    postData.reserve(message.size() + 14);
    postData.append("{ \"data\":\"");
    postData.append(message);
    postData.append("\" }");

    // TODO: RAII this thing
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    try {
        CurlWrap curl;
        std::string response;

        logDebug(logPrefix + "Attempting to post to: " + url);

        curl.setopt(CURLOPT_URL, url.c_str());
        curl.setopt(CURLOPT_HTTPPOST, 1L);

        // connecton timeout. override the default and set to 10 seconds.
        curl.setopt(CURLOPT_CONNECTTIMEOUT, 10L);

        curl.setopt(CURLOPT_WRITEFUNCTION, WriteCallback);
        curl.setopt(CURLOPT_WRITEDATA, &response);
        curl.setopt(CURLOPT_HTTPHEADER, headers);
        curl.setopt(CURLOPT_POSTFIELDS, postData.c_str());
        curl.setopt(CURLOPT_POSTFIELDSIZE, postData.size());

        curl.perform();

        if (response.find("index") != std::string::npos) {
            logDebug(logPrefix + "Post successful: " + response);
            success = true;
        } else {
            logWarning(logPrefix + "Unknown reponse: " + response);
        }
    } catch (curl_exception &error) {
        logWarning(logPrefix + "curl exception: " + std::string(error.what()));
    }

    curl_slist_free_all(headers);

    return success;
}

std::string Link::prependIdentifier(const std::string &key) {
    return key + ":" + address.hostname + ":" + std::to_string(address.port) + ":" +
           address.hashtag;
}
