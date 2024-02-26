
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

#include "VoaThread.h"

#include "../include/RaceSdk.h"
#include "OpenTracingHelpers.h"
#include "VoaConfig.h"
#include "helper.h"

VoaThread::VoaThread(RaceSdk &sdk, const std::string &configPath) :
    raceSdk(sdk),
    voaConfig(configPath),
    voaActiveState(true),
    mRnd(std::random_device{}()),
    voaQueue([](const std::shared_ptr<VoaWorkItem> &v1, const std::shared_ptr<VoaWorkItem> &v2) {
        return v1->holdTimestamp > v2->holdTimestamp;
    }) {
    helper::logDebug("VoaThread::VoaThread constructor called with config:" + configPath);
    voa_thread_state.store(State::STOPPED);
}

void VoaThread::startThread() {
    helper::logDebug("VoaThread::startThread  called");
    std::lock_guard<std::mutex> lock(voa_queue_mutex);
    voa_thread = std::thread(&VoaThread::runVoaThread, this);
    voa_thread_state.store(State::STARTED);
}

void VoaThread::runVoaThread() {
    helper::logDebug("VoaThread::runVoaThread called");
    while (true) {
        using TimePoint =
            std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>>;
        using Duration = TimePoint::duration;

        std::unique_lock<std::mutex> lock(voa_queue_mutex);

        // check state prior to waiting
        if (voa_thread_state.load() == State::STOPPED) {
            helper::logInfo("VoaThread::runVoaThread stopping thread.");
            break;
        }

        if (voaQueue.empty()) {
            helper::logDebug("VoaThread::runVoaThread: Waiting until lock notify");
            voa_thread_signaler.wait(lock);
        } else {
            double holdTimestamp = (*voaQueue.top()).holdTimestamp;
            std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
            if (holdTimestamp > now.count()) {
                helper::logDebug("VoaThread::runVoaThread: Waiting until:" +
                                 std::to_string(holdTimestamp));
                TimePoint nextTimestamp = TimePoint{Duration{holdTimestamp}};
                voa_thread_signaler.wait_until(lock, nextTimestamp);
            }
        }

        // check if we should stop
        if (voa_thread_state.load() == State::STOPPED) {
            helper::logInfo("VoaThread::runVoaThread stopping thread.");
            break;
        }

        // Hold time reached or notifiy received
        // Process entries in the queue
        std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
        helper::logDebug("VoaThread::runVoaThread: woke up at:" + std::to_string(now.count()));
        if (voaQueue.size() == 0) {
            helper::logDebug(
                "VoaThread::runVoaThread: empty queue. "
                "Going back to sleep");
            continue;
        }

        auto voaIt = voaQueue.top();
        auto currItem = *voaIt;
        helper::logDebug("VoaThread::runVoaThread: checking holdTimeStamp=" +
                         std::to_string(currItem.holdTimestamp) +
                         " at time=" + std::to_string(now.count()));

        if (currItem.holdTimestamp > now.count()) {
            helper::logDebug(
                "VoaThread::runVoaThread: spurious wakeup. "
                "Going back to sleep");
            continue;
        }

        helper::logDebug("VoaThread::runVoaThread: invoking callback.");
        // Don't hold the lock while we perform the callback
        lock.unlock();
        SdkResponse response = currItem.callback();
        lock.lock();
        if (response.status != SDK_OK) {
            helper::logInfo("VoaThread::runVoaThread: failed callback for handle:" +
                            std::to_string(response.handle) +
                            " with status:" + sdkStatusToString(response.status));
        }
        voaQueue.pop();
        helper::logDebug("VoaThread::process: voaQueue size after work removed:" +
                         std::to_string(voaQueue.size()));
    }
    helper::logInfo("VoaThread::runVoaThread returning from thread.");
}

// This routine was copied from plugin-comms-twosix-cpp/source/base/Link.cpp
// Ideally, comms plugins should be able to use the SDK implementation
EncPkg VoaThread::corruptPackage(const EncPkg &pkg, uint32_t corruptAmout) {
    RawData cipherText = pkg.getCipherText();
    std::uniform_int_distribution<size_t> corruptIndex(0, cipherText.size() - 1);
    std::uniform_int_distribution<uint8_t> corruptByte(0, 255);

    // Is there a better method of corruption?
    // for now, just mess with random bytes
    for (uint32_t i = 0; i < corruptAmout; ++i) {
        size_t index = corruptIndex(mRnd);
        cipherText[index] = corruptByte(mRnd);
    }

    EncPkg newPkg(pkg.getTraceId(), pkg.getSpanId(), cipherText);
    return newPkg;
}

std::list<std::pair<EncPkg, double>> VoaThread::getVoaPkgQueue(
    const EncPkg &ePkg, std::string activePersona, LinkID linkId, std::string channelGid,
    std::vector<std::string> &personaList) {
    // No lock should be needed here since we are simply constructing a list

    std::list<std::pair<EncPkg, double>> pkgQueue;

    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    std::vector<VoaConfig::VoaRule> ruleVector =
        voaConfig.findTargetedRules(activePersona, linkId, channelGid, personaList);

    if (ruleVector.empty()) {
        return {};
    }

    std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
    double currentTimestamp = now.count();

    // retrieve first active rule
    std::vector<VoaConfig::VoaRule>::iterator it = std::find_if(
        ruleVector.begin(), ruleVector.end(),
        [&](const VoaConfig::VoaRule &rule) { return voaConfig.isActive(rule, currentTimestamp); });

    if (it == std::end(ruleVector)) {
        helper::logDebug("VoaThread::getVoaPkgQueue no rules are active");
        return {};
    }

    VoaConfig::VoaRule rule = *it;
    helper::logDebug("VoaConfig::getVoaPkgQueue: found active rule:" + rule.ruleId);

    // Check if rule application window is triggered
    if (!voaConfig.isTriggered(rule)) {
        helper::logDebug("VoaThread::getVoaPkgQueue not triggered");
        return {};
    }

    // Add tag to opentracing log
    auto pkgSpanContext = spanContextFromEncryptedPackage(ePkg);
    std::shared_ptr<opentracing::Span> span = raceSdk.getTracer()->StartSpan(
        "voa_processing", {opentracing::ChildOf(pkgSpanContext.get())});

    span->SetTag("voa_ruleId", rule.ruleId);
    span->SetTag("voa_action", rule.action);
    span->SetTag("voa_tag", rule.tag);
    span->SetTag("voa_linkId", linkId);
    span->SetTag("voa_channelGid", channelGid);
    span->SetTag("voa_activePersona", activePersona);
    span->SetTag("voa_personaList", helper::personasToString(personaList));

    raceSdk.traceLinkStatus(span, linkId);
    span->Finish();

    // update the traceId and spanId for the package
    EncPkg newPkg = ePkg;
    newPkg.setTraceId(traceIdFromContext(span->context()));
    newPkg.setSpanId(spanIdFromContext(span->context()));

    if (rule.action == VoaConfig::VOA_ACTION_DROP) {
        pkgQueue.push_back(std::make_pair(newPkg, VOA_DROP_TIMESTAMP));
        helper::logInfo("VoaThread::getVoaPkgQueue: Dropping package on LinkId=" + linkId +
                        " and Gid=" + channelGid);
    } else if (rule.action == VoaConfig::VOA_ACTION_DELAY) {
        helper::logInfo("VoaThread::getVoaPkgQueue: Delaying package on LinkId=" + linkId +
                        " and Gid=" + channelGid);
        float randWeight = distribution(mRnd);
        double holdTime = rule.getHoldTimeParam(randWeight);
        double holdTimestamp = currentTimestamp + holdTime;
        pkgQueue.push_back(std::make_pair(newPkg, holdTimestamp));
        helper::logInfo("VoaThread::getVoaPkgQueue: holding package for delay=" +
                        std::to_string(holdTime) + " until " + std::to_string(holdTimestamp));
    } else if (rule.action == VoaConfig::VOA_ACTION_TAMPER) {
        helper::logInfo("VoaThread::getVoaPkgQueue: Mangling package on LinkId=" + linkId +
                        " and Gid=" + channelGid);
        unsigned long corruptTimes = rule.getIterationsParam();
        EncPkg ePkgMod = corruptPackage(newPkg, corruptTimes);
        pkgQueue.push_back(std::make_pair(ePkgMod, currentTimestamp));
    } else if (rule.action == VoaConfig::VOA_ACTION_REPLAY) {
        helper::logInfo("VoaThread::getVoaPkgQueue: Replaying package on LinkId=" + linkId +
                        " and Gid=" + channelGid);
        // Replay of one actually implies two packages
        unsigned long times = rule.getReplayTimesParam() + 1;
        double holdTimestamp = currentTimestamp;
        for (unsigned long i = 0; i < times; i++) {
            float randWeight = distribution(mRnd);
            double holdTime = rule.getHoldTimeParam(randWeight);
            pkgQueue.push_back(std::make_pair(EncPkg(newPkg), holdTimestamp));
            holdTimestamp += holdTime;
        }
    } else {
        helper::logInfo("VoaThread::getVoaPkgQueue: No rule matched");
        throw std::invalid_argument("Voa rule action has to be one of drop/delay/tamper/replay");
    }
    return pkgQueue;
}

void VoaThread::process(std::list<std::shared_ptr<VoaThread::VoaWorkItem>> voaItems) {
    std::lock_guard<std::mutex> lock(voa_queue_mutex);

    for (auto voa : voaItems) {
        voaQueue.push(voa);  // inserts in sorted holdtime order
    }

    helper::logDebug("VoaThread::process: voaQueue size after work added:" +
                     std::to_string(voaQueue.size()));
    // tell Voa thread to look at its queue
    voa_thread_signaler.notify_one();
}

bool VoaThread::addVoaRules(const nlohmann::json &payload) {
    helper::logDebug("VoaThread::addVoaRules() called");
    std::lock_guard<std::mutex> lock(voa_queue_mutex);
    return voaConfig.addRules(payload);
}

bool VoaThread::deleteVoaRules(const nlohmann::json &payload) {
    helper::logDebug("VoaThread::deleteVoaRules() called");
    std::lock_guard<std::mutex> lock(voa_queue_mutex);
    return voaConfig.deleteRules(payload);
}

void VoaThread::setVoaActiveState(bool state) {
    helper::logDebug("VoaThread::setVoaActiveState() called");
    std::lock_guard<std::mutex> lock(voa_queue_mutex);
    voaActiveState = state;
}

bool VoaThread::isVoaActive() {
    helper::logDebug("VoaThread::isVoaActive() called");
    std::lock_guard<std::mutex> lock(voa_queue_mutex);
    return voaActiveState;
}

void VoaThread::stopThread() {
    helper::logInfo("VoaThread::stopThread called");

    State prev_state;

    {
        std::lock_guard<std::mutex> lock(voa_queue_mutex);
        prev_state = voa_thread_state.exchange(State::STOPPED);
    }

    if (prev_state == State::STARTED) {
        // Terminate threads
        voa_thread_signaler.notify_all();

        std::promise<void> timeoutPromise;
        std::thread([&]() {
            voa_thread.join();
            timeoutPromise.set_value();
        }).detach();

        // Wait for the threads to join. If they don't join within the timeout then assume they are
        // hanging, log an error, and bail.
        const std::int32_t secondsToWaitForThreadsToJoin = 5;
        auto status = timeoutPromise.get_future().wait_for(
            std::chrono::seconds(secondsToWaitForThreadsToJoin));
        if (status != std::future_status::ready) {
            helper::logError(
                "FATAL: Handler:: timed out waiting for voa thread to join. "
                "Terminating.");
            std::terminate();
        }
    }
}
