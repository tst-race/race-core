
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

#include <algorithm>
#include <iostream>

#include "../PluginCommsTwoSixCpp.h"
#include "../utils/base64.h"
#include "Channel.h"
#include "Connection.h"

Link::Link(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin, Channel *channel, const LinkID &linkId,
           const LinkProperties &linkProperties, const LinkProfileParser &parser) :
    mSdk(sdk),
    mPlugin(plugin),
    mChannel(channel),
    mRnd(std::random_device{}()),
    mId(linkId),
    mProperties(linkProperties),
    mShutdown(false),
    mSendThreadShutdown(false),
    mSendPeriodLength(parser.send_period_length),
    mSendPeriodAmount(parser.send_period_amount),
    mSleepPeriodLength(parser.sleep_period_length),
    mNextChange(std::numeric_limits<double>::infinity()),
    mNextSleepAmount(mSendPeriodAmount),
    mSleeping(false),
    mSendDropRate(parser.send_drop_rate),
    mReceiveDropRate(parser.receive_drop_rate),
    mSendCorruptRate(parser.send_corrupt_rate),
    mReceiveCorruptRate(parser.receive_corrupt_rate),
    mSendCorruptAmount(parser.send_corrupt_amount),
    mReceiveCorruptAmount(parser.receive_corrupt_amount),
    mTraceCorruptSizeLimit(parser.trace_corrupt_size_limit) {
    std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
    logDebug("Link(" + linkId + "): start time: " + std::to_string(now.count()));
    if (mSendPeriodLength > 0) {
        mNextChange = now.count() + mSendPeriodLength;
    }
    logDebug("Link(" + linkId + "): initial mNextChange: " + std::to_string(mNextChange));

    if (linkProperties.linkType == LT_SEND || linkProperties.linkType == LT_BIDI) {
        std::thread(&Link::runSendThread, this).detach();
    } else if (linkProperties.linkType == LT_RECV) {
        mSendThreadShutdown = true;
    }
}

Link::~Link() {}

bool Link::shouldSleep() {
    std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
    return !mSleeping &&
           ((now.count() > mNextChange) || (mSendPeriodAmount != 0 && mNextSleepAmount <= 0));
}

bool Link::shouldWake() {
    std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
    return mSleeping && now.count() > mNextChange;
}

bool Link::shouldSend() {
    return !mSleeping && mSendQueue.size() > 0;
}

void Link::goSleep() {
    logDebug("Link(" + getId() + "): Going to sleep");
    mSleeping = true;

    std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
    mNextChange = now.count() + mSleepPeriodLength;

    logDebug("Link(" + getId() + "): sending CONNECTION_UNAVAILABLE to connections");
    for (auto &conn : getConnections()) {
        if (conn->timeout < mSleepPeriodLength && conn->timeout != RACE_UNLIMITED) {
            conn->available = false;
            mSdk->onConnectionStatusChanged(NULL_RACE_HANDLE, conn->connectionId,
                                            CONNECTION_UNAVAILABLE, mProperties, 0);
        }
    }
    logDebug("Link(" + getId() + "): finished sending CONNECTION_UNAVAILABLE to connections");

    // timeout packages that will timeout while we're sleeping
    for (SendInfo &toSend : mSendQueue) {
        if (toSend.timeoutTimestamp < mNextChange) {
            mSdk->onPackageStatusChanged(toSend.handle, PACKAGE_FAILED_TIMEOUT, 0);
        }
    }
    mSendQueue.erase(
        std::remove_if(mSendQueue.begin(), mSendQueue.end(),
                       [this](const SendInfo &a) { return a.timeoutTimestamp < mNextChange; }),
        mSendQueue.end());
}

void Link::wakeUp() {
    logDebug("Link(" + getId() + "): Waking up");

    logDebug("Link(" + getId() + "): sending CONNECTION_AVAILABLE to connections");
    for (auto &conn : getConnections()) {
        if (conn->available == false) {
            conn->available = true;
            mSdk->onConnectionStatusChanged(NULL_RACE_HANDLE, conn->connectionId,
                                            CONNECTION_AVAILABLE, mProperties, 0);
        }
    }
    logDebug("Link(" + getId() + "): finished sending CONNECTION_AVAILABLE to connections");

    mSleeping = false;
    if (mSendPeriodLength > 0) {
        std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
        mNextChange = now.count() + mSendPeriodLength;
    } else {
        mNextChange = std::numeric_limits<double>::infinity();
    }
    mNextSleepAmount = mSendPeriodAmount;

    logDebug("Link(" + getId() + "): awake until " + std::to_string(mNextChange));
}

void Link::runSendThread(Link *link) {
    if (!link->runSendThreadInternal()) {
        logError("Link::runSendThread: Send thread failed, destroying link");
        link->mPlugin->destroyLink(0, link->mId);
        // link is now invalid. don't use it
    }
}

bool Link::runSendThreadInternal() {
    using TimePoint =
        std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>>;
    using Duration = TimePoint::duration;

    logDebug("Link(" + getId() + "): starting send thread");

    while (!mShutdown) {
        SendInfo sendInfo;
        {
            std::unique_lock<std::mutex> lock(mSendLock);

            // wait until the next change. If waiting forever because a sendPeriod has
            // not been specified (e.g. it's 0) then wait forever
            auto pred = [this] {
                return mShutdown || shouldSleep() || shouldWake() || shouldSend();
            };

            if (mNextChange < std::numeric_limits<double>::infinity()) {
                logDebug("Link(" + getId() + "): Waiting until: " + std::to_string(mNextChange));
                TimePoint nextChange = TimePoint{Duration{mNextChange}};
                mSendThreadSignaler.wait_until(lock, nextChange, pred);
            } else {
                logDebug("Link(" + getId() + "): Waiting forever");
                mSendThreadSignaler.wait(lock, pred);
            }

            logDebug("Link(" + getId() + "): Woke up. shutdown: " + std::to_string(mShutdown) +
                     ", shouldWake: " + std::to_string(shouldWake()) +
                     ", shouldSleep: " + std::to_string(shouldSleep()) +
                     ", shouldSend: " + std::to_string(shouldSend()));

            if (mShutdown) {
                logDebug("Link(" + getId() + "): Shutting down");
                break;
            }

            if (shouldSleep()) {
                goSleep();
                continue;
            }

            if (shouldWake()) {
                wakeUp();
                continue;
            }

            // predicate should prevent up from getting here
            if (!shouldSend()) {
                logError("Link(" + getId() + "): Woke up, but there's nothing to do.");
                continue;
            }

            logDebug("Link(" + getId() + "): Sending package");
            sendInfo = mSendQueue.front();
            mSendQueue.pop_front();
            mNextSleepAmount--;
            for (auto &connection : getConnections()) {
                mSdk->unblockQueue(connection->connectionId);
            }
        }

        if (!sendPackageWithCorruption(sendInfo.handle, *sendInfo.pkg)) {
            logError("Link(" + getId() + "): Send package failed, stopping send thread");
            break;
        }
    }

    std::unique_lock<std::mutex> lock(mSendLock);
    for (SendInfo &sendInfo : mSendQueue) {
        mSdk->onPackageStatusChanged(sendInfo.handle, PACKAGE_FAILED_GENERIC, 0);
    }

    mSendThreadShutdown = true;
    mSendThreadShutdownSignaler.notify_all();
    return mShutdown;
}

std::string Link::getCipherTextForDisplay(const EncPkg &pkg) {
    auto traceCipherText = pkg.getCipherText();
    if (traceCipherText.size() > mTraceCorruptSizeLimit) {
        traceCipherText.resize(mTraceCorruptSizeLimit);
        return base64::encode(traceCipherText) + "...";
    } else {
        return base64::encode(traceCipherText);
    }
}

bool Link::sendPackageWithCorruption(RaceHandle handle, const EncPkg &pkg) {
    std::bernoulli_distribution drop(mSendDropRate);
    if (drop(mRnd)) {
        logWarning("Dropping package due to send_drop_rate probability");
        logDebug("Dropped Package : " + getCipherTextForDisplay(pkg));
        mSdk->onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING);
        return true;
    }

    EncPkg newPkg({});
    std::bernoulli_distribution corrupt(mSendCorruptRate);
    if (corrupt(mRnd)) {
        newPkg = corruptPackage(pkg, mSendCorruptAmount);
    } else {
        newPkg = pkg;
    }

    return sendPackageInternal(handle, newPkg);
}

void Link::receivePackageWithCorruption(const EncPkg &pkg, const std::vector<ConnectionID> &connIds,
                                        int32_t timeout) {
    std::bernoulli_distribution drop(mReceiveDropRate);
    if (drop(mRnd)) {
        logWarning("Dropping package due to receive_drop_rate probability");
        logDebug("Dropped Package : " + getCipherTextForDisplay(pkg));
        return;
    }

    EncPkg newPkg({});
    std::bernoulli_distribution corrupt(mReceiveCorruptRate);
    if (corrupt(mRnd)) {
        newPkg = corruptPackage(pkg, mReceiveCorruptAmount);
    } else {
        newPkg = pkg;
    }

    SdkResponse response = mSdk->receiveEncPkg(newPkg, connIds, timeout);
    if (response.status != SDK_OK) {
        logWarning("SDK failed with status: " +
                   std::to_string(static_cast<int64_t>(response.status)));
    }
}

EncPkg Link::corruptPackage(const EncPkg &pkg, uint32_t corruptAmout) {
    RawData cipherText = pkg.getCipherText();
    std::uniform_int_distribution<size_t> corruptIndex(0, cipherText.size() - 1);
    std::uniform_int_distribution<uint8_t> corruptByte(0, 255);

    logWarning("Corrupting package");
    logDebug("Package before corruption: " + getCipherTextForDisplay(pkg));

    // Is there a better method of corruption?
    // for now, just mess with random bytes
    for (uint32_t i = 0; i < corruptAmout; ++i) {
        size_t index = corruptIndex(mRnd);
        cipherText[index] = corruptByte(mRnd);
    }

    logDebug("Package after corruption: " + getCipherTextForDisplay(pkg));

    EncPkg newPkg(pkg.getTraceId(), pkg.getSpanId(), cipherText);
    return newPkg;
}

PluginResponse Link::sendPackage(RaceHandle handle, const EncPkg &pkg, double timeoutTimestamp) {
    {
        std::lock_guard lock(mSendLock);

        if (mSendQueue.size() >= SEND_QUEUE_MAX_CAPACITY) {
            logDebug("sendPackage: send queue full for link: " + getId());
            return PLUGIN_TEMP_ERROR;
        }

        if (mSleeping && mNextChange > timeoutTimestamp) {
            mSdk->onPackageStatusChanged(handle, PACKAGE_FAILED_TIMEOUT, RACE_BLOCKING);
            return PLUGIN_OK;  // TODO: is this correct?
        }

        mSendQueue.emplace_back(handle, pkg, timeoutTimestamp);
    }

    mSendThreadSignaler.notify_one();

    return PLUGIN_OK;
}

PluginResponse Link::serveFiles(std::string /*path*/) {
    logError("serveFiles unsupported for link: " + mId);
    return PLUGIN_ERROR;
}

void Link::shutdownLink() {
    mShutdown = true;
    std::unique_lock<std::mutex> lock(mSendLock);
    mSendThreadSignaler.notify_one();
    mSendThreadShutdownSignaler.wait(lock, [this] { return mSendThreadShutdown.load(); });
    mChannel->onLinkDestroyed(this);
}

void Link::shutdown() {
    mShutdown = true;
    mSdk->onLinkStatusChanged(NULL_RACE_HANDLE, mId, LINK_DESTROYED, mProperties, RACE_BLOCKING);
    shutdownInternal();
    shutdownLink();

    for (const auto &connection : mConnections) {
        mSdk->onConnectionStatusChanged(NULL_RACE_HANDLE, connection->connectionId,
                                        CONNECTION_CLOSED, mProperties, 0);
    }
    mConnections.clear();
}

LinkID Link::getId() {
    // id is const, no need for lock
    return mId;
}

LinkProperties Link::getProperties() {
    std::lock_guard<std::mutex> lock(mLinkLock);
    return mProperties;
}

std::vector<std::shared_ptr<Connection>> Link::getConnections() {
    std::lock_guard<std::mutex> lock(mLinkLock);
    return mConnections;
}

bool Link::isAvailable() {
    return !mSleeping;
}
