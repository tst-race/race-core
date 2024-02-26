
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

#include "Handler.h"

#include "helper.h"

// increment the iterator to the next value. If it has reached the end of the container, reset it to
// the beginning
template <typename I, typename C>
I next_cycle(I iter, C &container) {
    iter++;

    if (iter == container.end()) {
        iter = container.begin();
    }

    return iter;
}

Handler::Handler(const std::string &name, size_t _max_queue_size, size_t _max_total_size) :
    name(name),
    max_queue_size(_max_queue_size),
    max_total_size(_max_total_size),
    state(State::PRESTART),
    timeoutQueue([](const std::shared_ptr<Work> &w1, const std::shared_ptr<Work> &w2) {
        return w1->timeoutTimestamp < w2->timeoutTimestamp;
    }),
    total_work(0),
    total_marked(0),
    total_size(0),
    unblocked_work(0) {
    create_queue("", 0);
    current_priority = priority_levels.begin();
}

Handler::~Handler() {
    helper::logDebug("~Handler called");
    stop_immediate();
    helper::logDebug("~Handler returned");
}

void Handler::start() {
    std::lock_guard<std::mutex> lock(data_mutex);

    // can only start if we haven't been started
    if (state.load() != State::PRESTART) {
        throw std::logic_error(
            "Failed to start handler thread. Handler was not in a valid state for start.");
    }

    state.store(State::STARTED);

    work_thread = std::thread(&Handler::runWorkThread, this);
    timeout_thread = std::thread(&Handler::runTimeoutThread, this);
}

void Handler::runTimeoutThread() {
    helper::set_thread_name(name + "-timeout-thread");
    while (true) {
        using TimePoint =
            std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>>;
        using Duration = TimePoint::duration;

        std::unique_lock<std::mutex> lock(data_mutex);

        // check state here. It may have changed while we were waiting for the lock
        if (state.load() != State::STARTED) {
            break;
        }

        // wait until the earliest timestamp (or we're woken up before that)
        double timeoutTimestamp = std::numeric_limits<double>::infinity();
        if (!timeoutQueue.empty()) {
            // helper::logInfo("Handler::runTimeoutThread: waiting until: " +
            // std::to_string((*timeoutQueue.begin())->timeoutTimestamp));
            timeoutTimestamp = (*timeoutQueue.begin())->timeoutTimestamp;
        }

        helper::logInfo("Handler::runTimeoutThread: waiting until: " +
                        std::to_string(timeoutTimestamp));
        if (timeoutTimestamp < std::numeric_limits<double>::infinity()) {
            TimePoint nextTimestamp = TimePoint{Duration{timeoutTimestamp}};
            timeout_thread_signaler.wait_until(lock, nextTimestamp);
        } else {
            // special case infinity. Waiting until infinity returns immediately, when we want it to
            // wait until it gets a signal
            timeout_thread_signaler.wait(lock);
        }

        // check state again here. it may have changed while we were waiting
        if (state.load() != State::STARTED) {
            break;
        }

        // find work that has timed out
        std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();

        while (timeoutQueue.size() > 0) {
            auto workIt = timeoutQueue.begin();
            auto work = *workIt;
            if (work->timeoutTimestamp > now.count()) {
                break;
            }

            if (!work->runningCallback && work->timeoutCallback) {
                helper::logDebug("Handler::runTimeoutThread: Calling timeout callback");
                work->timeoutCallback();
            }

            // move work from timeoutQueue to timedOutQueue
            work->timedOutIter = timedOutQueue.insert(timedOutQueue.end(), work);
            timeoutQueue.erase(workIt);
            work->timeoutIter = timeoutQueue.end();
            helper::logDebug("Handler::runTimeoutThread: Moving work to timed out queue");
        }

        // notify the work thread if there's something for it to do
        if (!timedOutQueue.empty()) {
            helper::logDebug("Handler::runTimeoutThread: Notifying work queue");
            work_thread_signaler.notify_one();
        }
    }
}

void Handler::runWorkThread() {
    helper::set_thread_name(name);
    while (true) {
        std::unique_lock<std::mutex> lock(data_mutex);

        // wait until we have something to do
        // possible things to do:
        // 1. unblock queues
        // 2. stop the thread
        // 3. remove timed out work
        // 4. perform work
        // 5. remove a queue
        //
        // Each iteration of the while loop will perform one of these, and then check again for
        // any other work
        work_thread_signaler.wait(lock, [this] {
            return unblock_list.size() > 0 || state.load() != State::STARTED || total_marked > 0 ||
                   unblocked_work > 0 || timedOutQueue.size() > 0;
        });

        // unblock any queues. This must happen before the check to stop, since this may cause
        // new work to become available.
        if (unblock_list.size()) {
            unblock_queues_internal();
            continue;
        }

        // check if we should stop
        if (state.load() == State::STOPPED ||
            (state.load() == State::STOPPING && unblocked_work == 0)) {
            // if we are stopping, we don't care about any blocked or marked queues
            break;
        }

        // remove timed out work
        if (timedOutQueue.size() > 0) {
            while (timedOutQueue.size() > 0) {
                helper::logDebug("Handler::runWorkThread: removing timed out work");
                remove_work_internal(timedOutQueue.front());
            }
            continue;
        }

        QueueIter &queue = get_next_queue_with_work();

        // got the queue to update, either get work from it, or remove it as necessary
        if (!queue->queue.empty()) {
            // get the work, and decrement the work counts and sizes
            Work &work = *queue->queue.front();
            work.runningCallback = true;

            // don't hold lock during work callback()
            lock.unlock();
            bool success = work.callback();
            lock.lock();

            if (success) {
                pop_queue_internal(queue);
            } else {
                // item is blocked. Don't pop, since we want it to be there when we're unblocked
                block_queue_internal(queue);
            }

            // advance to the next queue for fairness
            queue = next_cycle(queue, queue->priority_level.work_queues);
        } else if (queue->marked) {
            remove_queue_internal(queue);
        } else {
            helper::logError("Handler::run invalid state");
        }
    }
}

void Handler::stop() {
    State prev_state;
    {
        // tell the thread to stop
        std::lock_guard<std::mutex> lock(data_mutex);
        prev_state = state.load();
        if (prev_state == State::STARTED || prev_state == State::PRESTART) {
            state.store(State::STOPPING);
        }
    }

    // don't block other threads from posting while we are waiting for callbacks
    if (prev_state == State::STARTED) {
        // wait for thread to stop
        join_work_thread();
        join_timeout_thread();

        // sanity check
        std::lock_guard<std::mutex> lock(data_mutex);
        if (unblocked_work != 0) {
            helper::logError("Handler::stop: handler has work remaining after stop()");
        }
    }

    // clean up
    std::lock_guard<std::mutex> lock(data_mutex);
    if (prev_state == State::STARTED || prev_state == State::PRESTART) {
        state.store(State::STOPPED);
    }

    clear();
}

void Handler::stop_immediate() {
    State prev_state;

    {
        std::lock_guard<std::mutex> lock(data_mutex);
        prev_state = state.exchange(State::STOPPED);
    }
    if (prev_state == State::STARTED) {
        join_work_thread();
        join_timeout_thread();
    }

    std::lock_guard<std::mutex> lock(data_mutex);
    clear();
}

void Handler::create_queue(const std::string &queue_name, int priority) {
    std::unique_lock<std::mutex> data_lock(data_mutex);

    // check if we already have a queue with this name
    if (queue_map.count(queue_name) > 0) {
        throw std::invalid_argument("Already have a queue named: " + queue_name);
    }

    // get or create the priority
    PriorityIter priorityIter = priority_levels.emplace(priority, PriorityLevel(priority)).first;

    // create the queue
    QueueIter queueIter = priorityIter->second.work_queues.emplace(
        priorityIter->second.work_queues.end(), queue_name, priorityIter->second);

    // create the name mapping
    queue_map.emplace(queue_name, queueIter);
}

void Handler::remove_queue(const std::string &queue_name) {
    std::unique_lock<std::mutex> data_lock(data_mutex);

    // sanity check. Run assumes there is at least on queue in existence. Keep this queue around to
    // ensure that.
    if (queue_name == "") {
        throw std::invalid_argument("Cannot remove default queue");
    }

    auto iter = queue_map.find(queue_name);
    if (iter == queue_map.end()) {
        throw std::out_of_range("No queue named: " + queue_name + " exists");
    }

    // set the flag to have the work thread remove the queue if when it's empty
    QueueIter queue = iter->second;
    queue->marked = true;

    // we can only do more if the queue isn't blocked
    if (queue->blocked == false) {
        // increment the marked counters
        queue->priority_level.marked_count++;
        total_marked++;

        // update max priority if necessary
        if (queue->priority_level.priority > current_priority->first) {
            current_priority = priority_levels.find(queue->priority_level.priority);
            if (current_priority == priority_levels.end()) {
                // we have an entry in the queue map, but the entry in the priority map doesn't
                // exist. This shouldn't be possible
                throw std::logic_error(
                    "Failed to find entry in priority map. This should never happen");
            }
        }

        // There's a new marked queue, so tell the work thread to wake if if it's not awake already
        work_thread_signaler.notify_one();
    }
}

void Handler::unblock_queue(const std::string &queue_name) {
    std::unique_lock<std::mutex> data_lock(data_mutex);

    // push on to list to be run by work thread. We don't want to unblock stuff while a callback is
    // being run. There's a potential race condition where callback determines it is blocked,
    // something signals the unblock, we get the unblock and block after the callback returned in
    // that order and end up blocked when we shouldn't be if the callback and unblocking isn't
    // synchronized.
    unblock_list.push_back(queue_name);

    // There's a new queue to unblock, so tell the work thread to wake if if it's not awake already
    work_thread_signaler.notify_one();
}

Handler::State Handler::get_state() {
    return state.load();
}

int Handler::get_num_queues() {
    std::lock_guard<std::mutex> lock(data_mutex);
    int num_queues = 0;
    for (auto &priority_level : priority_levels) {
        num_queues += static_cast<int>(priority_level.second.work_queues.size());
    }
    return num_queues;
}

void Handler::join_work_thread() {
    work_thread_signaler.notify_all();
    std::promise<void> timeoutPromise;
    std::thread([&]() {
        work_thread.join();
        timeoutPromise.set_value();
    }).detach();
    // Wait for the threads to join. If they don't join within the timeout then assume they are
    // hanging, log an error, and bail.
    const std::int32_t secondsToWaitForThreadsToJoin = 5;
    auto status =
        timeoutPromise.get_future().wait_for(std::chrono::seconds(secondsToWaitForThreadsToJoin));
    if (status != std::future_status::ready) {
        helper::logError(
            "FATAL: Handler::join_work_thread: timed out waiting for worker thread to join. "
            "Terminating.");
        std::terminate();
    }
}

void Handler::join_timeout_thread() {
    timeout_thread_signaler.notify_all();
    std::promise<void> timeoutPromise;
    std::thread([&]() {
        timeout_thread.join();
        timeoutPromise.set_value();
    }).detach();
    // Wait for the threads to join. If they don't join within the timeout then assume they are
    // hanging, log an error, and bail.
    const std::int32_t secondsToWaitForThreadsToJoin = 5;
    auto status =
        timeoutPromise.get_future().wait_for(std::chrono::seconds(secondsToWaitForThreadsToJoin));
    if (status != std::future_status::ready) {
        helper::logError(
            "FATAL: Handler::join_timeout_thread: timed out waiting for timeout thread to join. "
            "Terminating.");
        std::terminate();
    }
}

void Handler::clear() {
    priority_levels.clear();
    queue_map.clear();
    timeoutQueue.clear();
    timedOutQueue.clear();
    unblocked_work = 0;
    total_work = 0;
    total_marked = 0;
    total_size = 0;

    // create default work queue, as it's assumed to always exist
    // can't use create as that tries to lock
    current_priority = priority_levels.emplace(0, PriorityLevel(0)).first;
    QueueIter queue = current_priority->second.work_queues.emplace(
        current_priority->second.work_queues.end(), "", current_priority->second);
    queue_map.emplace("", queue);
}

void Handler::unblock_queue_internal(const std::string &queue_name) {
    auto iter = queue_map.find(queue_name);
    if (iter == queue_map.end()) {
        return;
    }

    // clear the blocked flag
    QueueIter queue = iter->second;
    if (queue->blocked == true) {
        queue->blocked = false;

        // update the unblocked work counts
        queue->priority_level.unblocked_work_count += queue->queue.size();
        unblocked_work += queue->queue.size();

        // If this was marked for removal, add it to the marked counts. Queues that are blocked
        // don't count towards that. Now that it's unblocked, it should count towards the number of
        // marked queues
        if (queue->marked == true) {
            queue->priority_level.marked_count++;
            total_marked++;
        }

        // update max priority if necessary
        if (queue->priority_level.priority > current_priority->first) {
            current_priority = priority_levels.find(queue->priority_level.priority);
            if (current_priority == priority_levels.end()) {
                // we have an entry in the queue map, but the entry in the priority map doesn't
                // exist. This shouldn't be possible
                throw std::logic_error(
                    "Failed to find entry in priority map. This should never happen");
            }
        }
    }
}

void Handler::unblock_queues_internal() {
    for (const std::string &queue_name : unblock_list) {
        unblock_queue_internal(queue_name);
    }
    unblock_list.clear();
}

Handler::QueueIter &Handler::get_next_queue_with_work() {
    // find the first priority level with work or marked queues
    while (current_priority->second.unblocked_work_count == 0 &&
           current_priority->second.marked_count == 0) {
        current_priority = next_cycle(current_priority, priority_levels);
    }

    // create the queueIter if one doesn't exist, or get one if it does
    // it's a pointer, so updates will update the value in the map
    QueueIter &queue =
        queueIters.emplace(current_priority->first, current_priority->second.work_queues.begin())
            .first->second;

    // find next queue that is not blocked and has work or is marked and deal with it.
    while (queue->blocked || !(queue->marked || !queue->queue.empty())) {
        queue = next_cycle(queue, current_priority->second.work_queues);
    }

    return queue;
}

void Handler::pop_queue_internal(Handler::QueueIter &queue) {
    std::shared_ptr<Work> work = queue->queue.front();
    queue->queue.pop_front();

    if (work->timeoutIter != timeoutQueue.end()) {
        timeoutQueue.erase(work->timeoutIter);
    }

    if (work->timedOutIter != timedOutQueue.end()) {
        timedOutQueue.erase(work->timedOutIter);
    }

    // update counts
    queue->size -= work->size;
    queue->priority_level.unblocked_work_count--;
    total_size -= work->size;
    total_work--;
    unblocked_work--;

    post_signaler.notify_all();
}

void Handler::remove_work_internal(std::shared_ptr<Work> work) {
    Handler::QueueIter &queue = work->queueIter;
    queue->queue.erase(work->workIter);

    if (work->timeoutIter != timeoutQueue.end()) {
        timeoutQueue.erase(work->timeoutIter);
    }

    if (work->timedOutIter != timedOutQueue.end()) {
        timedOutQueue.erase(work->timedOutIter);
    }

    // update counts
    queue->size -= work->size;
    queue->priority_level.unblocked_work_count--;
    total_size -= work->size;
    total_work--;
    unblocked_work--;

    post_signaler.notify_all();
}

void Handler::block_queue_internal(Handler::QueueIter &queue) {
    // mark this queue as blocked, and adjust work counts approppriately
    queue->blocked = true;

    // update counts
    queue->priority_level.unblocked_work_count -= queue->queue.size();
    unblocked_work -= queue->queue.size();

    // if this was marked for removal, we can't remove it while work is blocked
    if (queue->marked == true) {
        queue->priority_level.marked_count--;
        total_marked--;
    }
}

void Handler::remove_queue_internal(Handler::QueueIter &queue) {
    // remove from the map to prevent anything new from being added to the queue
    queue_map.erase(queue->name);

    // remove this queue from the list of queues and advance the iterator to the next queue
    QueueIter next_queue = next_cycle(queue, current_priority->second.work_queues);
    current_priority->second.work_queues.erase(queue);
    queue = next_queue;

    // decrement the marked counters
    current_priority->second.marked_count--;
    total_marked--;

    // if there are no more queues int the priority level, it should be deleted
    if (current_priority->second.work_queues.empty()) {
        // remove this priority level from the priority - iterator map
        queueIters.erase(current_priority->first);

        // remove this iterator and advance max priority
        PriorityIter next_priority = next_cycle(current_priority, priority_levels);
        priority_levels.erase(current_priority);
        current_priority = next_priority;

        // sanity check
        if (priority_levels.empty()) {
            // The logic for finding work assumes the current priority is valid. If this
            // is reached, there are no valid priorities left. In remove queue, we
            // prevent removing the default queue to prevent this from happening.
            helper::logError("Handler::run No queues left. This is an invalid state.");
        }
    }
}

std::string handlerPostStatusToString(Handler::PostStatus status) {
    switch (status) {
        case Handler::PostStatus::OK:
            return "OK";
        case Handler::PostStatus::INVALID_STATE:
            return "INVALID_STATE";
        case Handler::PostStatus::QUEUE_FULL:
            return "QUEUE_FULL";
        case Handler::PostStatus::HANDLER_FULL:
            return "HANDLER_FULL";
        default:
            return "ERROR: INVALID COMPONENT STATUS" + std::to_string(status);
    }
}

std::ostream &operator<<(std::ostream &out, Handler::PostStatus status) {
    return out << handlerPostStatusToString(status);
}
