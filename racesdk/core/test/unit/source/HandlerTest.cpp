
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

#include <gtest/gtest.h>
#include <valgrind/valgrind.h>  // RUNNING_ON_VALGRIND

#include <chrono>
#include <memory>

#include "../../../source/Handler.h"
#include "../../common/race_printers.h"

using namespace std::chrono_literals;

const size_t max_queue_size = 1000;
const size_t max_total_size = 10000;

// macro because RUNNING_ON_VALGRIND isn't allowed at file scope
#define TIME_MULTIPLIER static_cast<int>(1 + (RUNNING_ON_VALGRIND)*10)

template <typename T>
static bool run_with_timeout(T callback, int milliseconds = 10000) {
    auto promise_ptr = std::make_shared<std::promise<void>>();
    auto future = promise_ptr->get_future();

    std::thread([callback, promise_ptr]() {
        callback();
        promise_ptr->set_value();
    }).detach();

    auto status = future.wait_for(std::chrono::milliseconds(milliseconds));
    return status == std::future_status::ready;
}

TEST(HandlerTest, test_no_start_no_stop) {
    bool finished =
        run_with_timeout([=] { Handler handler("test-handler", max_queue_size, max_total_size); });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_start_stop) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_start_stop_immediate) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.stop_immediate();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_start_no_stop) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_stop_no_start) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_stop_immediate_no_start) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.stop_immediate();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_stop_stop) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.stop();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_stop_stop_immediate) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.stop();
        handler.stop_immediate();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_stop_immediate_stop) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.stop_immediate();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_stop_immediate_stop_immediate) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.stop_immediate();
        handler.stop_immediate();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_start_stop_stop) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.stop();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_start_stop_immediate_stop_immediate) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.stop_immediate();
        handler.stop_immediate();
    });

    EXPECT_EQ(finished, true);
}

// with the update to blocking queues, we can't return void anymore

// TEST(HandlerTest, test_callback_returning_void) {
//     bool finished = run_with_timeout([=] {
//         Handler handler("test-handler", max_queue_size, max_total_size);
//         handler.start();
//         auto [success1, queueSize1, future1] = handler.post("", 0, [] {return
//         std::make_optional(true);}); (void)success1; (void)queueSize1; future1.wait();
//         future1.get();
//     });

//     EXPECT_EQ(finished, true);
// }

TEST(HandlerTest, test_start_after_stop_errors) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.stop();

        try {
            handler.start();
        } catch (const std::logic_error &e) {
            *value1 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

// same as above, except start is never called before stop
TEST(HandlerTest, test_start_after_stop_errors2) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.stop();

        try {
            handler.start();
        } catch (const std::logic_error &e) {
            *value1 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_start_after_stop_immediate_errors) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.stop_immediate();

        try {
            handler.start();
        } catch (const std::logic_error &e) {
            *value1 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

// same as above, except start is never called before stop
TEST(HandlerTest, test_start_after_stop_immediate_errors2) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.stop_immediate();

        try {
            handler.start();
        } catch (const std::logic_error &e) {
            *value1 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_post_before_start) {
    // if the thread continues after the timeout happens, bad stuff could occur if we just used an
    // int (or called the tests inside the callback)
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        auto [success, queueSize, future] =
            handler.post("", 0, 0, [] { return std::make_optional(true); });
        (void)success;
        (void)queueSize;
        handler.start();

        future.wait();
        *value1 = future.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_post_after_start) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        auto [success, queueSize, future] =
            handler.post("", 0, 0, [] { return std::make_optional(true); });
        (void)success;
        (void)queueSize;

        future.wait();
        *value1 = future.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_post_after_stop_future_error) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.stop();
        try {
            auto [success, queueSize, future] =
                handler.post("", 0, 0, [] { return std::make_optional(true); });
            (void)success;
            (void)queueSize;
            future.wait();
            future.get();
        } catch (std::future_error) {
            *value1 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

// same as above, except start is never called
TEST(HandlerTest, test_post_after_stop_future_error2) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.stop();
        try {
            auto [success, queueSize, future] =
                handler.post("", 0, 0, [] { return std::make_optional(true); });
            (void)success;
            (void)queueSize;
            future.wait();
            future.get();
        } catch (std::future_error) {
            *value1 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_post_after_stop_immediate_future_error) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.stop_immediate();
        try {
            auto [success, queueSize, future] =
                handler.post("", 0, 0, [] { return std::make_optional(true); });
            (void)success;
            (void)queueSize;
            future.wait();
            future.get();
        } catch (std::future_error) {
            *value1 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

// same as above, except start is never called
TEST(HandlerTest, test_post_after_stop_immediate_future_error2) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.stop_immediate();
        try {
            auto [success, queueSize, future] =
                handler.post("", 0, 0, [] { return std::make_optional(true); });
            (void)success;
            (void)queueSize;
            future.wait();
            future.get();
        } catch (std::future_error) {
            *value1 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_post_no_start_stop_future_error) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        auto [success, queueSize, future] =
            handler.post("", 0, 0, [] { return std::make_optional(true); });
        (void)success;
        (void)queueSize;
        handler.stop();
        try {
            future.wait();
            future.get();
        } catch (std::future_error) {
            *value1 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_post_no_start_stop_immediate_future_error) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        auto [success, queueSize, future] =
            handler.post("", 0, 0, [] { return std::make_optional(true); });
        (void)success;
        (void)queueSize;
        handler.stop_immediate();
        try {
            future.wait();
            future.get();
        } catch (std::future_error) {
            *value1 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_post_multiple_before_start) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        auto [success1, queueSize1, future1] =
            handler.post("", 0, 0, [] { return std::make_optional(true); });
        (void)success1;
        (void)queueSize1;
        auto [success2, queueSize2, future2] =
            handler.post("", 0, 0, [] { return std::make_optional(true); });
        (void)success2;
        (void)queueSize2;
        handler.start();

        future1.wait();
        *value1 = future1.get();
        future2.wait();
        *value2 = future2.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_multiple_after_start) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        auto [success1, queueSize1, future1] =
            handler.post("", 0, 0, [] { return std::make_optional(true); });
        (void)success1;
        (void)queueSize1;
        auto [success2, queueSize2, future2] =
            handler.post("", 0, 0, [] { return std::make_optional(true); });
        (void)success2;
        (void)queueSize2;

        future1.wait();
        *value1 = future1.get();
        future2.wait();
        *value2 = future2.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_handler_execution_order) {
    auto value1 = std::make_shared<int>(0);
    auto value2 = std::make_shared<int>(0);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        std::atomic<int> count(0);
        auto [success1, queueSize1, future1] =
            handler.post("", 0, 0, [&] { return std::make_optional(++count); });
        (void)success1;
        (void)queueSize1;
        auto [success2, queueSize2, future2] =
            handler.post("", 0, 0, [&] { return std::make_optional(++count); });
        (void)success2;
        (void)queueSize2;
        handler.start();

        future2.wait();
        *value2 = future2.get();
        future1.wait();
        *value1 = future1.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, 1);
    EXPECT_EQ(*value2, 2);
}

TEST(HandlerTest, test_callback_does_not_block_post) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        std::mutex m1;
        std::mutex m2;
        std::condition_variable cv1;
        std::unique_lock<std::mutex> lock1(m1);
        std::unique_lock<std::mutex> lock2(m2);

        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        auto [success1, queueSize1, future1] = handler.post("", 0, 0, [&] {
            // wait for posting thread to be waiting on the cv
            std::unique_lock<std::mutex> lock1_2(m1);
            // signal posting thread to post
            lock1_2.unlock();
            cv1.notify_one();
            // wait for the posting thread to unlock the mutex, signaling that it has posted
            std::unique_lock<std::mutex> lock2_2(m2);
            lock2_2.unlock();
            return std::make_optional(true);
        });

        (void)success1;
        (void)queueSize1;

        // wait until the callback signals us that it is executing
        cv1.wait(lock1);
        lock1.unlock();

        auto [success2, queueSize2, future2] =
            handler.post("", 0, 0, [] { return std::make_optional(true); });
        (void)success2;
        (void)queueSize2;

        // unlock the second mutex so the first callback can finish
        lock2.unlock();

        future1.wait();
        *value1 = future1.get();
        future2.wait();
        *value2 = future2.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_stop_immediate_breaks_promises) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        auto handler_ptr =
            std::make_shared<Handler>("test-handler", max_queue_size, max_total_size);
        auto promise_ptr = std::make_shared<std::promise<void>>();
        auto wait_future1 = promise_ptr->get_future();

        auto [success1, queueSize1, future1] =
            handler_ptr->post("", 0, 0, [handler_ptr, promise_ptr] {
                promise_ptr->set_value();
                // this sleep is because stop_immediate must be called during the callback, and
                // won't finish until the callback ends so it can't signal us either
                while (handler_ptr->get_state() != Handler::State::STOPPED) {
                    std::this_thread::sleep_for(1ms);
                }

                return std::make_optional(true);
            });
        auto [success2, queueSize2, future2] =
            handler_ptr->post("", 0, 0, [] { return std::make_optional(true); });

        (void)success1;
        (void)queueSize1;
        (void)success2;
        (void)queueSize2;

        handler_ptr->start();

        // wait for callback to start
        wait_future1.wait();
        // stop_immediate in the middle of a callback
        handler_ptr->stop_immediate();

        // callback 1 should finish before we exit stop
        future1.wait();
        *value1 = future1.get();

        // callback 2 promise should get broken
        try {
            future2.wait();
            future2.get();
        } catch (std::future_error) {
            *value2 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_stop_completes_promises) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        auto handler_ptr =
            std::make_shared<Handler>("test-handler", max_queue_size, max_total_size);
        auto promise_ptr = std::make_shared<std::promise<void>>();
        auto wait_future1 = promise_ptr->get_future();

        auto [success1, queueSize1, future1] =
            handler_ptr->post("", 0, 0, [handler_ptr, promise_ptr] {
                promise_ptr->set_value();
                // this sleep is because stop must be called during the callback, and won't finish
                // until the callback ends so it can't signal us either
                while (handler_ptr->get_state() == Handler::State::STARTED) {
                    std::this_thread::sleep_for(1ms);
                }

                return std::make_optional(true);
            });
        auto [success2, queueSize2, future2] =
            handler_ptr->post("", 0, 0, [] { return std::make_optional(true); });

        (void)success1;
        (void)queueSize1;
        (void)success2;
        (void)queueSize2;

        handler_ptr->start();

        // wait for callback to start
        wait_future1.wait();
        // stop in the middle of a callback
        handler_ptr->stop();

        // callback 1 should finish before we exit stop
        future1.wait();
        *value1 = future1.get();

        // callback 2 promise should not get broken
        future2.wait();
        *value2 = future2.get();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_stop_does_not_block_post) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        auto handler_ptr =
            std::make_shared<Handler>("test-handler", max_queue_size, max_total_size);
        auto promise_ptr = std::make_shared<std::promise<void>>();
        auto wait_future1 = promise_ptr->get_future();

        auto [success1, queueSize1, future1] =
            handler_ptr->post("", 0, 0, [handler_ptr, promise_ptr] {
                promise_ptr->set_value();
                // this sleep is because stop must be called during the callback, and won't finish
                // until the callback ends so it can't signal us either
                while (handler_ptr->get_state() == Handler::State::STARTED) {
                    std::this_thread::sleep_for(1ms * TIME_MULTIPLIER);
                }

                // we are now STOPPING. trying to post should return immediately and have a future
                // with an error.
                auto [success2, queueSize2, future2] =
                    handler_ptr->post("", 0, 0, [] { return std::make_optional(true); });
                (void)success2;
                (void)queueSize2;

                try {
                    future2.wait();
                    future2.get();
                } catch (std::future_error) {
                    return std::make_optional(true);
                }
                return std::make_optional(false);
            });

        (void)success1;
        (void)queueSize1;

        handler_ptr->start();

        // wait for callback to start
        wait_future1.wait();
        // stop in the middle of a callback
        handler_ptr->stop();

        // callback 1 should finish before we exit stop
        future1.wait();
        *value1 = future1.get();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

//////////////////////////////////////////////////
// Multiple queues
//////////////////////////////////////////////////

TEST(HandlerTest, test_start_create_stop) {
    auto num_queues = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.create_queue("1", 0);

        *num_queues = handler.get_num_queues();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*num_queues, 2);
}

TEST(HandlerTest, test_start_create_create_stop) {
    auto num_queues = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.create_queue("1", 0);
        handler.create_queue("2", 0);

        *num_queues = handler.get_num_queues();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*num_queues, 3);
}

TEST(HandlerTest, test_start_create_same_queue_error_stop) {
    auto num_queues = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.create_queue("1", 0);
        ASSERT_THROW(handler.create_queue("1", 0), std::invalid_argument);

        *num_queues = handler.get_num_queues();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*num_queues, 2);
}

TEST(HandlerTest, test_start_create_remove_stop) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.create_queue("1", 0);
        handler.remove_queue("1");

        // should have 2 queues still. as the queue shouldn't be removed until it's been hit by the
        // handler thread. not required, so not checking it
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_start_remove_stop_fail) {
    auto num_queues = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        ASSERT_THROW(handler.remove_queue("1"), std::out_of_range);

        *num_queues = handler.get_num_queues();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*num_queues, 1);
}

TEST(HandlerTest, test_start_remove_default_stop_fail) {
    auto num_queues = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        ASSERT_THROW(handler.remove_queue(""), std::invalid_argument);

        *num_queues = handler.get_num_queues();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*num_queues, 1);
}

TEST(HandlerTest, test_create_start_stop) {
    auto num_queues = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 0);
        *num_queues = handler.get_num_queues();

        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*num_queues, 2);
}

TEST(HandlerTest, test_create_create_start_stop) {
    auto num_queues = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 0);
        handler.create_queue("2", 0);
        *num_queues = handler.get_num_queues();

        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*num_queues, 3);
}

TEST(HandlerTest, test_create_remove_start_stop) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 0);
        handler.remove_queue("1");
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_create_remove_create_start_stop) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 0);
        handler.remove_queue("1");
        handler.start();

        while (handler.get_num_queues() > 1) {
            std::this_thread::sleep_for(1ms * TIME_MULTIPLIER);
        }

        handler.create_queue("1", 0);
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_remove_start_stop_fail) {
    auto num_queues = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        ASSERT_THROW(handler.remove_queue("1"), std::out_of_range);
        *num_queues = handler.get_num_queues();

        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*num_queues, 1);
}

TEST(HandlerTest, test_post_invalid_queue_fail) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        ASSERT_THROW(handler.post("1", 0, 0, []() { return std::make_optional(true); }),
                     std::out_of_range);
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_create_remove_post_fail) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 0);
        handler.remove_queue("1");
        ASSERT_THROW(handler.post("1", 0, 0, []() { return std::make_optional(true); }),
                     std::out_of_range);
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_start_create_post) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.create_queue("1", 0);
        auto [success, queueSize, future] =
            handler.post("1", 0, 0, [] { return std::make_optional(true); });
        (void)success;
        (void)queueSize;

        future.wait();
        *value1 = future.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_start_create_post_post) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.create_queue("1", 0);
        auto [success, queueSize, future] =
            handler.post("1", 0, 0, [] { return std::make_optional(true); });
        (void)success;
        (void)queueSize;
        auto [success2, queueSize2, future2] =
            handler.post("", 0, 0, [] { return std::make_optional(true); });
        (void)success2;
        (void)queueSize2;

        future.wait();
        *value1 = future.get();
        future2.wait();
        *value2 = future2.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_start_create_create_post) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        handler.create_queue("1", 0);
        handler.create_queue("2", 0);
        auto [success, queueSize, future] =
            handler.post("1", 0, 0, [] { return std::make_optional(true); });
        (void)success;
        (void)queueSize;

        future.wait();
        *value1 = future.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_multiple_queue_fairness) {
    auto value1 = std::make_shared<int>(-1);
    auto value2 = std::make_shared<int>(-1);
    auto value3 = std::make_shared<int>(-1);
    auto value4 = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        std::atomic<int> count1 = 0;
        std::atomic<int> count2 = 0;
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 0);
        handler.create_queue("2", 0);
        auto [success1, queueSize1, future1] =
            handler.post("1", 0, 0, [&] { return std::make_optional(++count1); });
        auto [success2, queueSize2, future2] =
            handler.post("1", 0, 0, [&] { return std::make_optional(++count2); });
        auto [success3, queueSize3, future3] =
            handler.post("2", 0, 0, [&] { return std::make_optional(++count2); });
        auto [success4, queueSize4, future4] =
            handler.post("2", 0, 0, [&] { return std::make_optional(++count1); });
        (void)success1;
        (void)queueSize1;
        (void)success2;
        (void)queueSize2;
        (void)success3;
        (void)queueSize3;
        (void)success4;
        (void)queueSize4;
        handler.start();

        future1.wait();
        *value1 = future1.get();
        future2.wait();
        *value2 = future2.get();
        future3.wait();
        *value3 = future3.get();
        future4.wait();
        *value4 = future4.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);

    // expect fair handling of queues
    EXPECT_EQ(*value1, 1);
    EXPECT_EQ(*value2, 2);
    EXPECT_EQ(*value3, 1);
    EXPECT_EQ(*value4, 2);
}

TEST(HandlerTest, test_create_post_remove_start_stop) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 0);
        auto [success, queueSize, future] =
            handler.post("1", 0, 0, [] { return std::make_optional(true); });
        (void)success;
        (void)queueSize;
        handler.remove_queue("1");
        handler.start();

        future.wait();
        *value1 = future.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_create_during_callback) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        auto [success, queueSize, future] = handler.post("", 0, 0, [&handler] {
            handler.create_queue("1", 0);
            return std::make_optional(true);
        });
        (void)success;
        (void)queueSize;

        future.wait();
        *value1 = future.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_create_post_remove_queue_eventually_removed) {
    auto value1 = std::make_shared<bool>(false);
    auto num_queues = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 0);
        auto [success, queueSize, future] =
            handler.post("1", 0, 0, [] { return std::make_optional(true); });
        (void)success;
        (void)queueSize;
        handler.remove_queue("1");
        handler.start();

        future.wait();
        *value1 = future.get();
        auto [success2, queueSize2, future2] =
            handler.post("", 0, 0, []() { return std::make_optional(true); });
        (void)success2;
        (void)queueSize2;
        auto [success3, queueSize3, future3] =
            handler.post("", 0, 0, []() { return std::make_optional(true); });
        (void)success3;
        (void)queueSize3;

        future2.wait();
        future3.wait();
        *num_queues = handler.get_num_queues();

        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*num_queues, 1);
}

TEST(HandlerTest, test_create_post_remove_different_queue) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);
    auto num_queues = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 0);
        handler.create_queue("2", 0);
        auto [success1, queueSize1, future1] =
            handler.post("1", 0, 0, [] { return std::make_optional(true); });
        (void)success1;
        (void)queueSize1;
        auto [success2, queueSize2, future2] =
            handler.post("1", 0, 0, [] { return std::make_optional(true); });
        (void)success2;
        (void)queueSize2;
        handler.remove_queue("2");
        handler.start();

        future1.wait();
        *value1 = future1.get();
        future2.wait();
        *value2 = future2.get();
        *num_queues = handler.get_num_queues();

        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
    EXPECT_EQ(*num_queues, 2);
}

TEST(HandlerTest, test_post_queue_size_initial) {
    auto value1 = std::make_shared<uint32_t>(-1);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        auto [success1, queueSize1, future1] =
            handler.post("", 0, 0, [] { return std::make_optional(true); });
        (void)success1;
        *value1 = queueSize1;
        (void)future1;
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, 0);
}

TEST(HandlerTest, test_post_queue_size_increment) {
    auto value1 = std::make_shared<uint32_t>(0);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        auto [success1, queueSize1, future1] =
            handler.post("", 10, 0, [] { return std::make_optional(true); });
        (void)success1;
        *value1 = queueSize1;
        (void)future1;
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, 10);
}

TEST(HandlerTest, test_post_post_queue_size_increment) {
    auto value1 = std::make_shared<uint32_t>(0);
    auto value2 = std::make_shared<uint32_t>(0);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        auto [success1, queueSize1, future1] =
            handler.post("", 10, 0, [] { return std::make_optional(true); });
        (void)success1;
        *value1 = queueSize1;
        (void)future1;
        auto [success2, queueSize2, future2] =
            handler.post("", 25, 0, [] { return std::make_optional(true); });
        (void)success2;
        *value2 = queueSize2;
        (void)future2;
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, 10);
    EXPECT_EQ(*value2, 35);
}

TEST(HandlerTest, test_post_queue_size_increment_multiple_queues) {
    auto value1 = std::make_shared<uint32_t>(0);
    auto value2 = std::make_shared<uint32_t>(0);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 0);
        auto [success1, queueSize1, future1] =
            handler.post("", 10, 0, [] { return std::make_optional(true); });
        (void)success1;
        *value1 = queueSize1;
        (void)future1;
        auto [success2, queueSize2, future2] =
            handler.post("1", 25, 0, [] { return std::make_optional(true); });
        (void)success2;
        *value2 = queueSize2;
        (void)future2;
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, 10);
    EXPECT_EQ(*value2, 25);
}

TEST(HandlerTest, test_post_queue_full) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        auto [success1, queueSize1, future1] = handler.post(
            "", handler.max_queue_size + 1, 0, [] { return std::make_optional(true); });
        *value1 = (success1 != Handler::PostStatus::OK);
        (void)queueSize1;
        (void)future1;
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_post_queue_full_post) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        auto [success1, queueSize1, future1] = handler.post(
            "", handler.max_queue_size + 1, 0, [] { return std::make_optional(true); });
        *value1 = (success1 != Handler::PostStatus::OK);
        (void)queueSize1;
        (void)future1;
        auto [success2, queueSize2, future2] =
            handler.post("", handler.max_queue_size, 0, [] { return std::make_optional(true); });
        *value2 = (success2 == Handler::PostStatus::OK);
        (void)queueSize2;
        (void)future2;
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_post_queue_full) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        auto [success1, queueSize1, future1] =
            handler.post("", handler.max_queue_size, 0, [] { return std::make_optional(true); });
        *value1 = (success1 == Handler::PostStatus::OK);
        (void)queueSize1;
        (void)future1;
        auto [success2, queueSize2, future2] =
            handler.post("", handler.max_queue_size, 0, [] { return std::make_optional(true); });
        *value2 = (success2 != Handler::PostStatus::OK);
        (void)queueSize2;
        (void)future2;
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_post_queue_not_full) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 0);
        auto [success1, queueSize1, future1] =
            handler.post("", handler.max_queue_size, 0, [] { return std::make_optional(true); });
        *value1 = (success1 == Handler::PostStatus::OK);
        (void)queueSize1;
        (void)future1;
        auto [success2, queueSize2, future2] =
            handler.post("1", handler.max_queue_size, 0, [] { return std::make_optional(true); });
        *value2 = (success2 == Handler::PostStatus::OK);
        (void)queueSize2;
        (void)future2;
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_multiple_priorities) {
    auto value1 = std::make_shared<int>(-1);
    auto value2 = std::make_shared<int>(-1);
    auto value3 = std::make_shared<int>(-1);
    auto value4 = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        std::atomic<int> count1 = 0;
        std::atomic<int> count2 = 0;
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 0);
        handler.create_queue("2", 1);
        auto [success1, queueSize1, future1] =
            handler.post("1", 0, 0, [&] { return std::make_optional(++count1); });
        auto [success2, queueSize2, future2] =
            handler.post("1", 0, 0, [&] { return std::make_optional(++count2); });
        auto [success3, queueSize3, future3] =
            handler.post("2", 0, 0, [&] { return std::make_optional(++count2); });
        auto [success4, queueSize4, future4] =
            handler.post("2", 0, 0, [&] { return std::make_optional(++count1); });
        (void)success1;
        (void)queueSize1;
        (void)success2;
        (void)queueSize2;
        (void)success3;
        (void)queueSize3;
        (void)success4;
        (void)queueSize4;
        handler.start();

        future1.wait();
        *value1 = future1.get();
        future2.wait();
        *value2 = future2.get();
        future3.wait();
        *value3 = future3.get();
        future4.wait();
        *value4 = future4.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);

    // expect 2 to be prioritized
    EXPECT_EQ(*value1, 2);
    EXPECT_EQ(*value2, 2);
    EXPECT_EQ(*value3, 1);
    EXPECT_EQ(*value4, 1);
}

TEST(HandlerTest, test_multiple_priorities_default_queue) {
    auto value1 = std::make_shared<int>(-1);
    auto value2 = std::make_shared<int>(-1);
    auto value3 = std::make_shared<int>(-1);
    auto value4 = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        std::atomic<int> count1 = 0;
        std::atomic<int> count2 = 0;
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 1);
        auto [success1, queueSize1, future1] =
            handler.post("1", 0, 0, [&] { return std::make_optional(++count1); });
        auto [success2, queueSize2, future2] =
            handler.post("1", 0, 0, [&] { return std::make_optional(++count2); });
        auto [success3, queueSize3, future3] =
            handler.post("", 0, 0, [&] { return std::make_optional(++count2); });
        auto [success4, queueSize4, future4] =
            handler.post("", 0, 0, [&] { return std::make_optional(++count1); });
        (void)success1;
        (void)queueSize1;
        (void)success2;
        (void)queueSize2;
        (void)success3;
        (void)queueSize3;
        (void)success4;
        (void)queueSize4;
        handler.start();

        future1.wait();
        *value1 = future1.get();
        future2.wait();
        *value2 = future2.get();
        future3.wait();
        *value3 = future3.get();
        future4.wait();
        *value4 = future4.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);

    // expect 1 to be prioritized
    EXPECT_EQ(*value1, 1);
    EXPECT_EQ(*value2, 1);
    EXPECT_EQ(*value3, 2);
    EXPECT_EQ(*value4, 2);
}

TEST(HandlerTest, test_multiple_priorities_fairness) {
    auto value1 = std::make_shared<int>(-1);
    auto value2 = std::make_shared<int>(-1);
    auto value3 = std::make_shared<int>(-1);
    auto value4 = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        std::atomic<int> count1 = 0;
        std::atomic<int> count2 = 0;
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 1);
        handler.create_queue("2", 1);
        auto [success1, queueSize1, future1] =
            handler.post("1", 0, 0, [&] { return std::make_optional(++count1); });
        auto [success2, queueSize2, future2] =
            handler.post("1", 0, 0, [&] { return std::make_optional(++count2); });
        auto [success3, queueSize3, future3] =
            handler.post("2", 0, 0, [&] { return std::make_optional(++count2); });
        auto [success4, queueSize4, future4] =
            handler.post("2", 0, 0, [&] { return std::make_optional(++count1); });
        (void)success1;
        (void)queueSize1;
        (void)success2;
        (void)queueSize2;
        (void)success3;
        (void)queueSize3;
        (void)success4;
        (void)queueSize4;
        handler.start();

        future1.wait();
        *value1 = future1.get();
        future2.wait();
        *value2 = future2.get();
        future3.wait();
        *value3 = future3.get();
        future4.wait();
        *value4 = future4.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);

    // expect fair handling of queues
    EXPECT_EQ(*value1, 1);
    EXPECT_EQ(*value2, 2);
    EXPECT_EQ(*value3, 1);
    EXPECT_EQ(*value4, 2);
}

TEST(HandlerTest, test_create_remove_priority) {
    auto num_queues = std::make_shared<int>(-1);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 1);
        handler.remove_queue("1");

        handler.start();

        while (handler.get_num_queues() > 1) {
            std::this_thread::sleep_for(1ms * TIME_MULTIPLIER);
        }

        auto [success2, queueSize2, future2] =
            handler.post("", 0, 0, []() { return std::make_optional(true); });
        (void)success2;
        (void)queueSize2;

        ASSERT_THROW(handler.post("1", 0, 0, []() { return std::make_optional(true); }),
                     std::out_of_range);

        future2.wait();
        *num_queues = handler.get_num_queues();
        handler.stop();
    });

    EXPECT_EQ(finished, true);

    EXPECT_EQ(*num_queues, 1);
}

//
// blocked queues
//

TEST(HandlerTest, test_blocked_queue_unfinished) {
    auto status = std::make_shared<std::future_status>();

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);

        handler.start();
        auto [success, queueSize, future] =
            handler.post("", 0, 0, []() -> std::optional<bool> { return std::nullopt; });
        (void)success;
        (void)queueSize;

        *status = future.wait_for(10ms * TIME_MULTIPLIER);
        handler.stop();
    });

    EXPECT_EQ(finished, true);

    EXPECT_EQ(*status, std::future_status::timeout);
}

TEST(HandlerTest, test_blocked_queue_unblock) {
    bool finished = run_with_timeout([=] {
        auto promise_ptr = std::make_shared<std::promise<void>>();
        auto local_future = promise_ptr->get_future();

        std::atomic<int> count = 0;
        Handler handler("test-handler", max_queue_size, max_total_size);

        handler.start();
        auto [success, queueSize, future] = handler.post("", 0, 0, [&]() -> std::optional<bool> {
            if (!count++) {
                promise_ptr->set_value();
                return std::nullopt;
            } else {
                return true;
            }
        });
        (void)success;
        (void)queueSize;
        local_future.wait();
        handler.unblock_queue("");
        future.wait();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_blocked_queue_post_unblock) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        auto promise_ptr = std::make_shared<std::promise<void>>();
        auto local_future = promise_ptr->get_future();

        std::atomic<int> count = 0;
        Handler handler("test-handler", max_queue_size, max_total_size);

        handler.start();
        auto [success, queueSize, future] = handler.post("", 0, 0, [&]() -> std::optional<bool> {
            if (!count++) {
                promise_ptr->set_value();
                return std::nullopt;
            } else {
                return true;
            }
        });
        (void)success;
        (void)queueSize;

        auto [success2, queueSize2, future2] =
            handler.post("", 0, 0, [&] { return std::make_optional(true); });
        (void)success2;
        (void)queueSize2;

        local_future.wait();
        handler.unblock_queue("");
        future.wait();
        *value1 = future.get();
        future2.wait();
        *value2 = future2.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_blocked_queue_post_unblock_during_callback) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        std::atomic<int> count = 0;
        Handler handler("test-handler", max_queue_size, max_total_size);

        handler.start();
        auto [success, queueSize, future] = handler.post("", 0, 0, [&]() -> std::optional<bool> {
            if (!count++) {
                handler.unblock_queue("");
                return std::nullopt;
            } else {
                return true;
            }
        });
        (void)success;
        (void)queueSize;

        future.wait();
        *value1 = future.get();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_blocked_queue_other_queue) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        auto promise_ptr = std::make_shared<std::promise<void>>();
        auto local_future = promise_ptr->get_future();

        std::atomic<int> count = 0;
        Handler handler("test-handler", max_queue_size, max_total_size);

        handler.start();
        handler.create_queue("1", 0);

        auto [success, queueSize, future] = handler.post("", 0, 0, [&]() -> std::optional<bool> {
            if (!count++) {
                promise_ptr->set_value();
                return std::nullopt;
            } else {
                return true;
            }
        });
        (void)success;
        (void)queueSize;

        local_future.wait();

        auto [success2, queueSize2, future2] =
            handler.post("1", 0, 0, [&] { return std::make_optional(true); });
        (void)success2;
        (void)queueSize2;

        future2.wait();
        *value2 = future2.get();

        handler.unblock_queue("");
        future.wait();
        *value1 = future.get();

        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_blocked_queue_other_queue_different_priority) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        auto promise_ptr = std::make_shared<std::promise<void>>();
        auto local_future = promise_ptr->get_future();

        std::atomic<int> count = 0;
        Handler handler("test-handler", max_queue_size, max_total_size);

        handler.start();
        handler.create_queue("1", -1);

        auto [success, queueSize, future] = handler.post("", 0, 0, [&]() -> std::optional<bool> {
            if (!count++) {
                promise_ptr->set_value();
                return std::nullopt;
            } else {
                return true;
            }
        });
        (void)success;
        (void)queueSize;

        local_future.wait();

        auto [success2, queueSize2, future2] =
            handler.post("1", 0, 0, [&] { return std::make_optional(true); });
        (void)success2;
        (void)queueSize2;

        future2.wait();
        *value2 = future2.get();

        handler.unblock_queue("");
        future.wait();
        *value1 = future.get();

        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_blocked_queue_remove) {
    auto value = std::make_shared<bool>(true);
    bool finished = run_with_timeout([=] {
        auto promise_ptr = std::make_shared<std::promise<void>>();
        auto local_future = promise_ptr->get_future();

        std::atomic<int> count = 0;
        Handler handler("test-handler", max_queue_size, max_total_size);

        handler.start();
        handler.create_queue("1", 0);
        auto [success, queueSize, future] = handler.post("1", 0, 0, [&]() -> std::optional<bool> {
            if (!count++) {
                promise_ptr->set_value();
                return std::nullopt;
            } else {
                *value = false;
                return true;
            }
        });
        (void)success;
        (void)queueSize;
        (void)future;
        local_future.wait();
        handler.remove_queue("1");
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value, true);
}

TEST(HandlerTest, test_remove_block_queue) {
    auto value = std::make_shared<bool>(true);
    bool finished = run_with_timeout([=] {
        auto promise_ptr = std::make_shared<std::promise<void>>();
        auto local_future = promise_ptr->get_future();

        std::atomic<int> count = 0;
        Handler handler("test-handler", max_queue_size, max_total_size);

        handler.create_queue("1", 0);
        auto [success, queueSize, future] = handler.post("1", 0, 0, [&]() -> std::optional<bool> {
            if (!count++) {
                promise_ptr->set_value();
                return std::nullopt;
            } else {
                *value = false;
                return true;
            }
        });
        (void)success;
        (void)queueSize;
        (void)future;
        handler.remove_queue("1");
        handler.start();
        local_future.wait();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value, true);
}

TEST(HandlerTest, test_unblock_invalid_queue) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();

        // This is expected not to throw an error, and any internal error should be handled
        ASSERT_NO_THROW(handler.unblock_queue("1"));
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_unblock_unblocked_queue) {
    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);

        handler.create_queue("1", 0);
        auto [success, queueSize, future] =
            handler.post("1", 0, 0, [&] { return std::make_optional(true); });
        (void)success;
        (void)queueSize;
        (void)future;
        handler.unblock_queue("1");
        handler.start();
        future.wait();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_blocked_queue_remove_unblock) {
    bool finished = run_with_timeout([=] {
        auto promise_ptr = std::make_shared<std::promise<void>>();
        auto local_future = promise_ptr->get_future();

        std::atomic<int> count = 0;
        Handler handler("test-handler", max_queue_size, max_total_size);

        handler.start();
        handler.create_queue("1", 0);
        auto [success, queueSize, future] = handler.post("1", 0, 0, [&]() -> std::optional<bool> {
            if (!count++) {
                promise_ptr->set_value();
                return std::nullopt;
            } else {
                return true;
            }
        });
        (void)success;
        (void)queueSize;
        (void)future;
        local_future.wait();
        handler.remove_queue("1");
        handler.unblock_queue("1");
        future.wait();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_remove_block_queue_unblock) {
    bool finished = run_with_timeout([=] {
        auto promise_ptr = std::make_shared<std::promise<void>>();
        auto local_future = promise_ptr->get_future();

        std::atomic<int> count = 0;
        Handler handler("test-handler", max_queue_size, max_total_size);

        handler.create_queue("1", 0);
        auto [success, queueSize, future] = handler.post("1", 0, 0, [&]() -> std::optional<bool> {
            if (!count++) {
                promise_ptr->set_value();
                return std::nullopt;
            } else {
                return true;
            }
        });
        (void)success;
        (void)queueSize;
        (void)future;
        handler.remove_queue("1");
        handler.start();
        local_future.wait();
        handler.unblock_queue("1");
        future.wait();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
}

TEST(HandlerTest, test_post_queue_full_with_post_timeout) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        auto [success1, queueSize1, future1] = handler.post(
            "", handler.max_queue_size + 1, 1, [] { return std::make_optional(true); });
        *value1 = (success1 != Handler::PostStatus::OK);
        (void)queueSize1;
        (void)future1;
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_post_queue_full_post_with_post_timeout) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        auto [success1, queueSize1, future1] = handler.post(
            "", handler.max_queue_size + 1, 1, [] { return std::make_optional(true); });
        *value1 = (success1 != Handler::PostStatus::OK);
        (void)queueSize1;
        (void)future1;
        auto [success2, queueSize2, future2] =
            handler.post("", handler.max_queue_size, 1, [] { return std::make_optional(true); });
        *value2 = (success2 == Handler::PostStatus::OK);
        (void)queueSize2;
        (void)future2;
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_post_queue_full_with_post_timeout) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        auto [success1, queueSize1, future1] =
            handler.post("", handler.max_queue_size, 1, [] { return std::make_optional(true); });
        *value1 = (success1 == Handler::PostStatus::OK);
        (void)queueSize1;
        (void)future1;
        auto [success2, queueSize2, future2] =
            handler.post("", handler.max_queue_size, 1, [] { return std::make_optional(true); });
        *value2 = (success2 != Handler::PostStatus::OK);
        (void)queueSize2;
        (void)future2;
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_post_queue_full_post_timeout_success) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        auto [success1, queueSize1, future1] = handler.post("", handler.max_queue_size, 1, [] {
            // this sleep is because the callback must finish while post is waiting for its timeout,
            // so it can't signal us
            std::this_thread::sleep_for(20ms * TIME_MULTIPLIER);
            return std::make_optional(true);
        });
        *value1 = (success1 == Handler::PostStatus::OK);
        (void)queueSize1;
        (void)future1;
        auto [success2, queueSize2, future2] =
            handler.post("", handler.max_queue_size, 40 * TIME_MULTIPLIER,
                         [] { return std::make_optional(true); });
        *value2 = (success2 == Handler::PostStatus::OK);
        (void)queueSize2;
        (void)future2;
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_post_race_blocking_post_timeout_success) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        auto [success1, queueSize1, future1] = handler.post("", handler.max_queue_size, 1, [] {
            // this sleep is because the callback must finish while post is waiting for its timeout,
            // so it can't signal us
            std::this_thread::sleep_for(10ms * TIME_MULTIPLIER);
            return std::make_optional(true);
        });
        *value1 = (success1 == Handler::PostStatus::OK);
        (void)queueSize1;
        (void)future1;
        auto [success2, queueSize2, future2] = handler.post(
            "", handler.max_queue_size, RACE_BLOCKING, [] { return std::make_optional(true); });
        *value2 = (success2 == Handler::PostStatus::OK);
        (void)queueSize2;
        (void)future2;
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_post_race_blocking_post_timeout_blocks) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);
    auto value3 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        auto [success1, queueSize1, future1] = handler.post("", handler.max_queue_size, 1, [] {
            std::this_thread::sleep_for(20ms * TIME_MULTIPLIER);
            return std::make_optional(true);
        });
        *value1 = (success1 == Handler::PostStatus::OK);
        (void)queueSize1;
        (void)future1;

        // expect run_with_timeout to timeout after 10 ms, before the post has completed. The post
        // should finish after 20ms, once the for callback above has completed.
        std::promise<void> promise;
        bool finished2 = run_with_timeout(
            [&] {
                auto [success2, queueSize2, future2] =
                    handler.post("", handler.max_queue_size, RACE_BLOCKING,
                                 [] { return std::make_optional(true); });
                *value2 = (success2 == Handler::PostStatus::OK);
                (void)queueSize2;
                (void)future2;
                promise.set_value();
            },
            10 * TIME_MULTIPLIER);
        promise.get_future().wait();
        *value3 = !finished2;
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
    EXPECT_EQ(*value3, true);
}

TEST(HandlerTest, test_post_blocking_too_large) {
    auto value1 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.start();
        auto [success1, queueSize1, future1] = handler.post(
            "", handler.max_queue_size + 1, RACE_BLOCKING, [] { return std::make_optional(true); });
        *value1 = (success1 != Handler::PostStatus::OK);
        (void)queueSize1;
        (void)future1;
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
}

TEST(HandlerTest, test_post_post_handler_full_with_post_timeout) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        // max total size equals max queue size so it can be filled in a single post
        Handler handler("test-handler", max_queue_size, max_queue_size);
        handler.create_queue("1", 1);

        auto [success1, queueSize1, future1] =
            handler.post("1", handler.max_queue_size, 1, [] { return std::make_optional(true); });
        *value1 = (success1 == Handler::PostStatus::OK);
        (void)queueSize1;
        (void)future1;

        auto [success2, queueSize2, future2] =
            handler.post("", handler.max_queue_size, 1, [] { return std::make_optional(true); });
        *value2 = (success2 != Handler::PostStatus::OK);
        (void)queueSize2;
        (void)future2;
        handler.start();
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_post_handler_full_post_timeout_success) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        // max total size equals max queue size so it can be filled in a single post
        Handler handler("test-handler", max_queue_size, max_queue_size);
        handler.create_queue("1", 1);

        handler.start();
        auto [success1, queueSize1, future1] = handler.post("1", handler.max_queue_size, 1, [] {
            // this sleep is because the callback must finish while post is waiting for its timeout,
            // so it can't signal us
            std::this_thread::sleep_for(20ms * TIME_MULTIPLIER);
            return std::make_optional(true);
        });
        *value1 = (success1 == Handler::PostStatus::OK);
        (void)queueSize1;
        (void)future1;
        auto [success2, queueSize2, future2] =
            handler.post("", handler.max_queue_size, 40 * TIME_MULTIPLIER,
                         [] { return std::make_optional(true); });
        *value2 = (success2 == Handler::PostStatus::OK);
        (void)queueSize2;
        (void)future2;
        handler.stop();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_expired_before_start_work_timeout_timedout) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 1);

        // timeout immediately
        auto [success, queueSize, future] = handler.post(
            "1", 0, 0, [] { return std::make_optional(true); }, 0.0, [value1] { *value1 = true; });

        (void)queueSize;

        handler.start();
        try {
            future.wait();
            future.get();
        } catch (std::future_error) {
            // timeout should cause future to error
            *value2 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_before_start_work_timeout_timedout) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 1);

        auto promise_ptr = std::make_shared<std::promise<void>>();

        auto [success1, queueSize1, future1] = handler.post("1", 0, 0, [promise_ptr] {
            // wait for the work timeout of the next work
            promise_ptr->get_future().wait();
            return std::make_optional(true);
        });
        (void)success1;
        (void)queueSize1;
        (void)future1;

        // timeout 5 ms
        std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
        auto [success2, queueSize2, future2] = handler.post(
            "1", 0, 0, [] { return std::make_optional(true); },
            now.count() + 0.005 * TIME_MULTIPLIER,
            [value1, promise_ptr] {
                *value1 = true;
                promise_ptr->set_value();
            });

        (void)queueSize2;

        handler.start();
        try {
            future2.wait();
            future2.get();
        } catch (std::future_error) {
            // timeout should cause future to error
            *value2 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_expired_after_start_work_timeout_timedout) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 1);
        handler.start();

        // timeout immediately
        auto [success2, queueSize2, future2] = handler.post(
            "1", 0, 0, [] { return std::make_optional(true); }, 0.0, [value1] { *value1 = true; });

        (void)queueSize2;

        try {
            future2.wait();
            future2.get();
        } catch (std::future_error) {
            // timeout should cause future to error
            *value2 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_after_start_work_timeout_timedout) {
    auto value1 = std::make_shared<bool>(false);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 1);
        handler.start();

        auto promise_ptr = std::make_shared<std::promise<void>>();

        auto [success1, queueSize1, future1] = handler.post("1", 0, 0, [promise_ptr] {
            // wait for the work timeout of the next work
            promise_ptr->get_future().wait();
            return std::make_optional(true);
        });
        (void)success1;
        (void)queueSize1;
        (void)future1;

        // timeout 5 ms
        std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
        auto [success2, queueSize2, future2] = handler.post(
            "1", 0, 0, [] { return std::make_optional(true); },
            now.count() + 0.005 * TIME_MULTIPLIER,
            [value1, promise_ptr] {
                *value1 = true;
                promise_ptr->set_value();
            });

        (void)queueSize2;

        try {
            future2.wait();
            future2.get();
        } catch (std::future_error) {
            // timeout should cause future to error
            *value2 = true;
        }
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_work_timeout_during_callback) {
    auto value1 = std::make_shared<bool>(true);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 1);

        handler.start();

        std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
        auto [success1, queueSize1, future1] = handler.post(
            "1", 0, 0,
            [] {
                // this sleep is because the callback must finish while post is waiting for its
                // timeout, so it can't signal us
                std::this_thread::sleep_for(20ms * TIME_MULTIPLIER);
                return std::make_optional(true);
            },
            now.count() + 0.010 * TIME_MULTIPLIER, [value1] { *value1 = false; });
        (void)queueSize1;

        future1.wait();
        *value2 = future1.get();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}

TEST(HandlerTest, test_post_work_finished_no_timeout) {
    auto value1 = std::make_shared<bool>(true);
    auto value2 = std::make_shared<bool>(false);

    bool finished = run_with_timeout([=] {
        Handler handler("test-handler", max_queue_size, max_total_size);
        handler.create_queue("1", 1);

        handler.start();

        auto [success1, queueSize1, future1] = handler.post(
            "1", 0, 0, [] { return std::make_optional(true); },
            std::numeric_limits<double>::infinity(), [value1] { *value1 = false; });
        (void)queueSize1;

        future1.wait();
        *value2 = future1.get();
    });

    EXPECT_EQ(finished, true);
    EXPECT_EQ(*value1, true);
    EXPECT_EQ(*value2, true);
}