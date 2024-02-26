
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

#ifndef __VOA_THREAD_H__
#define __VOA_THREAD_H__

#include <atomic>
#include <future>
#include <list>
#include <memory>
#include <queue>
#include <thread>

#include "VoaConfig.h"

/**
 * @brief This constant defines a special-value timestamp that indicates that a package
 * needs to be dropped as part of VoA processing.
 */
const int32_t VOA_DROP_TIMESTAMP = std::numeric_limits<int32_t>::min();

/**
 * VoaThread: A class to manage the thread associated with Voice of Adversary
 * (VoA) actions
 */
class VoaThread {
public:
    enum class State { STARTED, STOPPED };

    /**
     * @brief Representation of a single VoA deferred action
     */
    struct VoaWorkItem {
        // The deferred action
        std::function<SdkResponse()> callback;
        // Hold-time prior to invoking the deferred action
        double holdTimestamp;

        template <typename T>
        VoaWorkItem(T _callback, double _holdTimestamp) :
            callback(_callback), holdTimestamp(_holdTimestamp) {}
    };

private:
    // Reference to the RACE SDK context
    RaceSdk &raceSdk;

    // The thread object
    std::thread voa_thread;

    // Current VoA thread state
    std::atomic<State> voa_thread_state;

    // Reference to the VoA config
    VoaConfig voaConfig;

    // State to indicate if VoA processing is active
    bool voaActiveState;

    // Random engine for probablistic actions
    std::default_random_engine mRnd;

    // A queue of VoA actions. Ordering based on hold-timestamp values.
    std::priority_queue<std::shared_ptr<VoaWorkItem>, std::vector<std::shared_ptr<VoaWorkItem>>,
                        std::function<bool(const std::shared_ptr<VoaWorkItem> &,
                                           const std::shared_ptr<VoaWorkItem> &)>>
        voaQueue;

    // Mutex that mediates access to voaQueue
    std::mutex voa_queue_mutex;

    // Condition variable that manages concurrent access to voaQueue
    // between the work posting and work invocation functions.
    std::condition_variable voa_thread_signaler;

    // Internal routine to start-up VoA thread
    void runVoaThread();

    // Helper routine to corrupt package
    EncPkg corruptPackage(const EncPkg &pkg, uint32_t corruptAmout);

public:
    /**
     * @brief Constructor
     *
     * @param sdk a reference to the Race SDK context
     * @param configPath path to the VoA config file
     */
    explicit VoaThread(RaceSdk &sdk, const std::string &configPath);

    /**
     * @brief Startup VoA Thread
     */
    void startThread();

    /**
     * @brief Stop VoA Thread
     */
    void stopThread();

    /**
     * @brief Return a list of VoA packets to supplant existing package
     *
     * @param ePkg Package under VoA consideration
     * @param activePersona active persona selection parameter
     * @param linkId link selection parameter
     * @param personas persona selection parameter
     *
     * @returns A list of package (copies) and an associated hold-timestamp, if
     * the package was matched for VoA processing.
     */
    std::list<std::pair<EncPkg, double>> getVoaPkgQueue(const EncPkg &ePkg,
                                                        std::string activePersona, LinkID linkId,
                                                        std::string channelGid,
                                                        std::vector<std::string> &personaList);

    /**
     * @brief Process selected packages through VoA processing pipeline.
     *
     * @param voaItems list of <work-item, hold-timestamp> tuples
     */
    void process(std::list<std::shared_ptr<VoaWorkItem>> voaItems);

    /**
     * @brief Add a new rule received from RiB
     *
     * @param payload The add rule payload
     */
    bool addVoaRules(const nlohmann::json &payload);

    /**
     * @brief Delete rules as directed by RiB
     *
     * @param payload The delete rules payload
     */
    bool deleteVoaRules(const nlohmann::json &payload);

    /**
     * @brief Set the state of VoA processing
     *
     * @param state True (active) or False (inactive)
     */
    void setVoaActiveState(bool state);

    /**
     * @brief Return current state of Voa processing
     *
     */
    bool isVoaActive();
};

#endif
