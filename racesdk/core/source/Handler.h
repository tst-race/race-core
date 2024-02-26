
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

#ifndef __HANDLER_H__
#define __HANDLER_H__

#include <IRaceSdkCommon.h>

#include <condition_variable>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <thread>
#include <unordered_map>

class Handler {
public:
    enum class State { INVALID, PRESTART, STARTED, STOPPING, STOPPED };
    enum PostStatus { OK, INVALID_STATE, QUEUE_FULL, HANDLER_FULL };

    const std::string name;
    const size_t max_queue_size;
    const size_t max_total_size;

private:
    struct PriorityLevel;
    struct WorkQueue;
    struct Work;

    using QueueIter = std::list<WorkQueue>::iterator;
    using PriorityIter = std::map<int, PriorityLevel, std::greater<int>>::iterator;
    using TimeoutIter = std::multiset<std::shared_ptr<Work>,
                                      std::function<bool(const std::shared_ptr<Work> &,
                                                         const std::shared_ptr<Work> &)>>::iterator;
    using TimedOutIter = std::list<std::shared_ptr<Work>>::iterator;
    using WorkQueueIter = std::list<std::shared_ptr<Work>>::iterator;

    struct Work {
        std::function<bool()> callback;
        std::function<void()> timeoutCallback;
        size_t size;
        double timeoutTimestamp;
        QueueIter queueIter;     // the queue that contains this work
        WorkQueueIter workIter;  // the iterator to this work in the work queue
        TimeoutIter timeoutIter;
        TimedOutIter timedOutIter;
        bool runningCallback;

        template <typename T, typename T2>
        Work(T &&_callback, T2 _timeoutCallback, size_t _size, double _timeoutTimestamp) :
            callback(std::forward<T>(_callback)),
            timeoutCallback(_timeoutCallback),
            size(_size),
            timeoutTimestamp(_timeoutTimestamp),
            runningCallback(false) {}
        Work() : size(0), timeoutTimestamp(0), runningCallback(false) {}
    };

    struct WorkQueue {
        // The work in this queue
        std::list<std::shared_ptr<Work>> queue;

        // The priority level this queue is part of
        PriorityLevel &priority_level;

        // The name of the queue
        std::string name;

        // the sum of the sizes of all the work in this queue
        size_t size;

        // whether this queue is marked for deletion
        bool marked;

        // whether this queue is blocked and can't make progress
        bool blocked;

        WorkQueue(const std::string &_name, PriorityLevel &_priority_level) :
            priority_level(_priority_level), name(_name), size(0), marked(false), blocked(false) {}
    };

    struct PriorityLevel {
        // The queues in this priority level
        std::list<WorkQueue> work_queues;

        // The integer priority
        int priority;

        // The total number of work in this priority level
        size_t unblocked_work_count;

        // The total number of marked queue in this priority level
        size_t marked_count;

        explicit PriorityLevel(int _priority) :
            priority(_priority), unblocked_work_count(0), marked_count(0) {}
    };

    // Locking this prevents any other thread from simultaneously modifying any of the member
    // variables Locking this does not mean anything can be done to the variables however. Notably,
    // the work thread maintains iterators to queues and priorities, so even if the mutex is locked
    // no operation that invalidates iterators should be called on those containers.
    std::mutex data_mutex;

    // informs the timeout thread that there's work to be done.
    std::condition_variable timeout_thread_signaler;

    // informs the work thread that there's work to be done.
    std::condition_variable work_thread_signaler;
    std::atomic<State> state;
    std::thread work_thread;
    std::thread timeout_thread;

    // used to inform waiting posts that there may be more space available. This was changed from
    // being per queue because a post waiting on total work to decrease may be unblocked by a
    // different queue being processed.
    std::condition_variable post_signaler;

    // priorities, in descending order. Each priority has a list of queues. Those quese are
    // processed fairly. no operations that invalidate iterators of either this or the queus inside
    // should be done by any thread other than the work thread
    std::map<int, PriorityLevel, std::greater<int>> priority_levels;

    // The current priority the work thread is processesing work from. This may be higher than the
    // priority of the highest priority work currently, but will not be lower.
    PriorityIter current_priority;

    // Iterators to the next queue to get work from for each priority level. This is used to
    // maintain fairness. e.g. if we do work on the first queue on priority 0, then get a new piece
    // of work on priority 1, when we got back to doing work on priority 0 we want to start with the
    // second queue
    std::unordered_map<int, QueueIter> queueIters;

    // queues to be unblocked by the work thread.
    std::vector<std::string> unblock_list;

    // The work, in timeout order
    std::multiset<std::shared_ptr<Work>,
                  std::function<bool(const std::shared_ptr<Work> &, const std::shared_ptr<Work> &)>>
        timeoutQueue;

    // work that has timed out, but not yet been removed from its work queue
    std::list<std::shared_ptr<Work>> timedOutQueue;

    // various counts, prevents having to iterate over all the queues to tell if there's work to do
    // the amount of work in all queues, including blocked queues
    int total_work;
    // the number of queues marked for removal, NOT including blocked queues that are marked for
    // removal
    int total_marked;
    // the sum of the size of all work in all queues, including blocked queues
    size_t total_size;
    // the amount of work in all queues, not including blocked queues
    size_t unblocked_work;

    // Map queue name to queue
    std::unordered_map<std::string, QueueIter> queue_map;

public:
    /* Handler: Construct a Handler that manages a thread of execution
     *
     * No thread is created on construction. To start a the Handler thread, start() must be called.
     * A newly created handler may have callbacks posted to it. These callbacks will not be run
     * until start() is called.
     *
     * param max_queue_size: the maximum amount of work a single queue can have, in bytes
     * param max_total_size: the maximum amount of work all queues combined can have, in bytes
     */
    Handler(const std::string &name, size_t _max_queue_size, size_t _max_total_size);
    ~Handler();

    /* post: Post a callback to be run on the handler thread
     *
     * If post is called before the handler has been started, post will return as normal but the
     * callback will not be called until after start() has been called. If multiple posts are
     * performed in series, the order the callbacks are executed in matches the order they were
     * submitted to the handler. If stop_immediate() is before the callback starts executing, the
     * future will be set to ready with error condition std::future_error. If stop() or
     * stop_immediate() was called before calling post, the future returned will immediately be
     * ready with its error condition set to std::future_error.
     *
     * exceptions: throws std::out_of_range if a queue with the specified name does not exist.
     *
     * param queue_name: The name of the queue to post to
     * param postedWorkSize: size the size in bytes of the posted work
     * param timeout: how long to wait (in milliseconds) for space to become available
     * param callback: callback to be executed on the internal handler thread.
     * return: a tuple containing whether the work was successfully implaced, the size of the queue
     * that is was implaced on, and a future that may be used to wait for completion of the callback
     * and retrieving the return value.
     */
    template <typename T>
    auto post(std::string queue_name, size_t postedWorkSize, int timeout, T &&callback,
              double timeoutTimestamp = std::numeric_limits<double>::infinity(),
              std::function<void()> timeoutCallback = {})
        -> std::tuple<Handler::PostStatus, size_t,
                      std::future<typename std::remove_reference<decltype(*callback())>::type>>;

    /* start: Start the internal Handler thread
     *
     * start() may only be called once on a given handler. If start() is called after
     * start(), stop(), or stop_immediate() was called on the same handler previously,
     * std::logic_error is thrown.
     */
    void start();

    /* stop_immediate: stop the internal Handler thread as soon as possible
     *
     * stop() will prevent any new callbacks from being posted and wait for all previously posted
     * callbacks to complete. Once it is complete, the internal thread will gracefully exit. Once
     * stop() has been called, start() may not be called (even if it had not been called before).
     * Attempts to post before while stop() is running will return immediately and have their
     * futures set to ready with error condition std::future_error.
     *
     * Calling stop_immediate() additional times has no effect.
     */
    void stop();

    /* stop_immediate: stop the internal Handler thread as soon as possible
     *
     * stop_immediate() will wait for the callback currently executing on the internal handler
     * thread to complete. Once it is complete, the internal thread will gracefully exit. Any
     * callbacks not called will have their associated future set to ready with an exception of
     * std::future_error. Once stop_immediate() has been called, start() may not be called (even if
     * it had not been called before). Any callbacks not completed before stop_immediate() returns
     * will have their futures set to ready with error condition std::future_error.
     *
     * Calling stop_immediate() additional times has no effect.
     */
    void stop_immediate();

    /* create_queue: create a new queue on the handler thread.
     *
     * create_queue() will create a new queue with the specified name that allows work to be posted
     * to it. A handler may have multiple queues associated with the handler thread. A piece of work
     * may be posted to a specific queue. All queues are processed in FIFO order. All queues are
     * processed fairly, e.g. work backing up on one queue will not cause other queues to be
     * blocked.
     *
     * exceptions: throws std::invalid_argument if a queue with the specified name already exists.
     *
     * param queue_name: The name of the queue to be created
     */
    void create_queue(const std::string &queue_name, int priority);

    /* remove_queue: mark a queue for removal.
     *
     * remove_queue() marks a queue for removal. Queues marked for removal cannot be posted to.
     * Existing callbacks on the queue will be completed. Once the queue has been emptied, it is
     * deleted by the handler thread.
     *
     * exceptions: throws std::invalid_argument if a queue with the specified does not exist
     *
     * param queue_name: The name of the queue to be removed
     */
    void remove_queue(const std::string &queue_name);

    /* unblock_queue: Unblock a queue and start processing the work on the queue again
     *
     * This method doesn't do the work of unblocking the queue. It merely puts the queue in a list
     * to be unblocked later. This is done so that if a call to unblock and a callback happen at the
     * same time, the unblock happens after the callback is completed.
     *
     * param queue_name: The name of the queue to be unblocked
     */
    void unblock_queue(const std::string &queue_name);

    /* get_state: Get the current state of the Handler
     *
     * Return the internal state of the handler. This should be one of Handler::State::PRESTART,
     * Handler::State::STARTED, or Handler::State::STOPPED. This value should not be relied on to
     * prevent futures from throwing std::future_error as any such attempt will be prone to race
     * conditions. This method therefore should be used for diagnostics and tests only.
     *
     * return: Handler::State enum.
     */
    State get_state();

    /* get_num_queues: Get the current number of queues on the handler thread
     *
     * Returns the number of queues that exist on the handler thread. This value includes the number
     * of queues marked for deletion.
     *
     * return: int number of queues.
     */
    int get_num_queues();

protected:
    /**
     * @brief The main work thread function. Waits for work and performs it on the work thread.
     * Stops if the state is stopping and there's no more work, or stops immediately if the state is
     * set to stopped.
     *
     */
    void runWorkThread();

    /**
     * @brief The timeout thread. Waits for queued work to timeout, and notifies the work thread if
     * it has.
     *
     */
    void runTimeoutThread();

    /**
     * @brief Join the work thread, or timeout if it takes too long.
     *
     */
    void join_work_thread();

    /**
     * @brief Join the timeout thread
     *
     */
    void join_timeout_thread();

    /**
     * @brief Clear the handler. remove all work and all queues except for the default queue. This
     * is an internal function and both the data and map mutexes must be obtained before calling
     *
     */
    void clear();

    /**
     * @brief Does the actual work of unblocking the queue. This will set flags and adjust the
     * counters appropriately. This must not be called by any thread other than the work thread.
     *
     * @param: queue_name The name of the queue to unblock
     */
    void unblock_queue_internal(const std::string &queue_name);

    /**
     * @brief unblocks all queues in the unblock list. This must not be called by any thread other
     * than the work thread.
     *
     */
    void unblock_queues_internal();

    /**
     * @brief gets the next queue that has work and updates current_priority as appropriate.
     *
     * @return: QueueIter& A reference to an iterator. A reference is returned so that the iterator
     * can be updated to point to the next queue that should be returned from this priority
     */
    QueueIter &get_next_queue_with_work();

    /**
     * @brief Remove a queue. Unlike the remove_queue method, this method actually removes the
     * queue, instead of just marking it to be removed. This invalidates iterators and should only
     * be called by the work thread.
     *
     * @param: queue The queue to remove
     */
    void remove_queue_internal(QueueIter &queue);

    /**
     * @brief remove the first work from a queue
     *
     * @param: queue The queue to remove work from
     */
    void pop_queue_internal(QueueIter &queue);

    /**
     * @brief remove the specified work from the queue
     *
     * @param: work The work to remove
     */
    void remove_work_internal(std::shared_ptr<Work> work);

    /**
     * @brief block a queue
     *
     * @param: queue The queue to block
     */
    void block_queue_internal(QueueIter &queue);
};

template <typename T>
auto Handler::post(std::string queue_name, size_t postedWorkSize, int timeout, T &&callback,
                   double timeoutTimestamp, std::function<void()> timeoutCallback)
    -> std::tuple<Handler::PostStatus, size_t,
                  std::future<typename std::remove_reference<decltype(*callback())>::type>> {
    using Ret = typename std::remove_reference<decltype(*callback())>::type;
    static_assert(std::is_same<decltype(callback()), std::optional<Ret>>::value,
                  "return type of callback must be std::optional");

    // we need shared_ptr because std::function requires everything to be copyable, and promise is
    // not
    auto promise_ptr = std::make_shared<std::promise<Ret>>();
    bool success = false;
    size_t queue_size = 0;

    std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
    if (timeoutTimestamp < now.count() && timeoutCallback) {
        timeoutCallback();
        return {PostStatus::OK, 0, promise_ptr->get_future()};
    }

    // work is too large to fit in even an empty queue
    if (postedWorkSize > max_queue_size) {
        return {PostStatus::QUEUE_FULL, 0, std::future<Ret>()};
    } else if (postedWorkSize > max_total_size) {
        return {PostStatus::HANDLER_FULL, 0, std::future<Ret>()};
    }

    if (state.load() == State::PRESTART || state.load() == State::STARTED) {
        std::unique_lock<std::mutex> lock(data_mutex);
        // throws std::out_of_range if queue does not exist
        QueueIter queue = queue_map.at(queue_name);
        PostStatus wait_status = PostStatus::OK;

        auto cond = [&]() {
            // Look up the queue again. wait() will release the lock, so it's possible the map has
            // changed. This will throw std::out_of_range if the queue was deleted while we were
            // waiting
            queue = queue_map.at(queue_name);

            if (queue->marked) {
                // just throw out of range to map the queue not existing case. The queue is
                // being removed, so it 'does not exist' for the purposes of post
                throw std::out_of_range("Queue is being removed");
            }

            // check the capacity to make sure the work will fit
            if ((queue->size + postedWorkSize) > max_queue_size) {
                wait_status = PostStatus::QUEUE_FULL;
                return false;
            } else if ((total_size + postedWorkSize) > max_total_size) {
                wait_status = PostStatus::HANDLER_FULL;
                return false;
            }

            wait_status = PostStatus::OK;
            return true;
        };

        if (timeout == RACE_BLOCKING) {
            post_signaler.wait(lock, cond);
        } else {
            if (!post_signaler.wait_for(lock, std::chrono::milliseconds(timeout), cond)) {
                // queue is still empty after timeout
                return {wait_status, queue->size, std::future<Ret>()};
            }
        }

        auto work = std::make_shared<Work>(
            [callback = std::forward<T>(callback), promise_ptr]() mutable {
                auto ret = callback();

                // if the callback returned a value, return true
                // if it returned an null optional, return false.
                if (ret) {
                    promise_ptr->set_value(*ret);
                    return true;
                } else {
                    return false;
                }
            },
            timeoutCallback, postedWorkSize, timeoutTimestamp);

        // notify the work thread if there's a shorter timeout
        if (timeoutTimestamp != std::numeric_limits<double>::infinity() &&
            (timeoutQueue.empty() ||
             timeoutTimestamp < (*timeoutQueue.begin())->timeoutTimestamp)) {
            timeout_thread_signaler.notify_one();
        }

        // add the work and get the iterator
        auto workIt = queue->queue.insert(queue->queue.end(), work);
        auto timeoutIt = timeoutQueue.insert(work);

        work->queueIter = queue;
        work->workIter = workIt;
        work->timeoutIter = timeoutIt;
        work->timedOutIter = timedOutQueue.end();

        // update counters
        queue->size += postedWorkSize;
        total_size += postedWorkSize;
        total_work++;

        // only update the unblocked counters if the queue we're putting the work in is not
        // blocked
        if (!queue->blocked) {
            queue->priority_level.unblocked_work_count++;
            unblocked_work++;
        }

        // update max priority if we are higher that the current max work priority
        if (queue->priority_level.priority > current_priority->first) {
            current_priority = priority_levels.find(queue->priority_level.priority);
            if (current_priority == priority_levels.end()) {
                // we have an entry in the queue map, but the entry in the priority map doesn't
                // exist. This shouldn't be possible
                throw std::logic_error(
                    "Failed to find entry in priority map. This should never happen");
            }
        }

        // we did it!
        success = true;

        queue_size = queue->size;
    }

    work_thread_signaler.notify_one();
    return {success ? PostStatus::OK : PostStatus::INVALID_STATE, queue_size,
            promise_ptr->get_future()};
}

std::string handlerPostStatusToString(Handler::PostStatus status);
std::ostream &operator<<(std::ostream &out, Handler::PostStatus status);

#endif  // __HANDLER_H__
