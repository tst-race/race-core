
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
#include <race/mocks/MockTransportSdk.h>

#include "Link.h"
#include "curlwrap.h"

std::vector<uint8_t> toByteVector(const std::string &str) {
    return {str.begin(), str.end()};
}

class TestLink : public ::testing::Test {
public:
    MockTransportSdk sdk;
    Link link;

    TestLink() : link("LinkID", {"test-hashtag"}, LinkProperties(), &sdk) {}
};

TEST_F(TestLink, enqueue_content_should_update_queue_size) {
    ::testing::Sequence sequence;
    ASSERT_EQ(COMPONENT_OK, link.enqueueContent(1, {0x12, 0x34, 0x56}));
    ASSERT_EQ(COMPONENT_OK, link.enqueueContent(2, {0x78, 0x90}));
}

TEST_F(TestLink, dequeue_content_should_update_queue_size_if_content_is_found) {
    ::testing::Sequence sequence;
    ASSERT_EQ(COMPONENT_OK, link.enqueueContent(1, {0x12, 0x34, 0x56}));
    ASSERT_EQ(COMPONENT_OK, link.dequeueContent(1));
}

TEST_F(TestLink, dequeue_content_should_not_update_queue_size_if_content_is_not_found) {
    ::testing::Sequence sequence;
    ASSERT_EQ(COMPONENT_OK, link.enqueueContent(1, {0x12, 0x34, 0x56}));
    ASSERT_EQ(COMPONENT_OK, link.dequeueContent(2));
}

TEST_F(TestLink, fetch_should_return_error_when_shutdown) {
    link.shutdown();
    ASSERT_EQ(COMPONENT_ERROR, link.fetch());
}

TEST_F(TestLink, fetch_should_return_error_when_action_queue_full) {
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(COMPONENT_OK, link.fetch());
    }
    ASSERT_EQ(COMPONENT_ERROR, link.fetch());
}

TEST_F(TestLink, post_should_return_error_when_shutdown) {
    link.shutdown();
    ASSERT_EQ(COMPONENT_ERROR, link.post({3}, 14));
}

TEST_F(TestLink, post_should_return_error_when_action_queue_full) {
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(COMPONENT_OK, link.fetch());
    }
    ASSERT_EQ(COMPONENT_ERROR, link.post({10}, 14));
}

TEST_F(TestLink, post_should_update_package_status_when_no_queued_content) {
    EXPECT_CALL(sdk, onPackageStatusChanged(3, PACKAGE_FAILED_GENERIC));
    ASSERT_EQ(COMPONENT_OK, link.post({3}, 14));
}

TEST_F(TestLink, post_succeed_for_queued_content) {
    ASSERT_EQ(COMPONENT_OK, link.enqueueContent(14, {0x12, 0x34}));
    ASSERT_EQ(COMPONENT_OK, link.post({3}, 14));
}

class LinkToTestActionThread : public Link {
public:
    using Link::Link;
    // Only mocks out the functions called by runActionThread
    MOCK_METHOD(int, getInitialIndex, (), (override));
    MOCK_METHOD(int, fetchOnActionThread, (int latestIndex), (override));
    MOCK_METHOD(void, postOnActionThread,
                (const std::vector<RaceHandle> &handles, uint64_t actionId), (override));
};

class TestLinkActionThread : public ::testing::Test {
public:
    MockTransportSdk sdk;
    LinkToTestActionThread link;

    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool actionExecuted{false};

    TestLinkActionThread() : link("LinkID", {"test-hashtag"}, LinkProperties(), &sdk) {}

    void markActionExecuted() {
        std::lock_guard<std::mutex> lock(mutex);
        actionExecuted = true;
        conditionVariable.notify_one();
    }

    void waitForActionToBeExecuted() {
        std::unique_lock<std::mutex> lock(mutex);
        conditionVariable.wait(lock, [this] { return actionExecuted; });
    }
};

TEST_F(TestLinkActionThread, action_thread_should_execute_queued_fetch_action) {
    EXPECT_CALL(link, getInitialIndex()).WillOnce(::testing::Return(42));
    EXPECT_CALL(link, fetchOnActionThread(42)).WillOnce(::testing::Invoke([this](int) {
        markActionExecuted();
        return 44;
    }));
    EXPECT_CALL(link, fetchOnActionThread(44)).WillOnce(::testing::Invoke([this](int) {
        markActionExecuted();
        return 45;
    }));

    link.start();

    ASSERT_EQ(COMPONENT_OK, link.fetch());
    waitForActionToBeExecuted();

    actionExecuted = false;

    ASSERT_EQ(COMPONENT_OK, link.fetch());
    waitForActionToBeExecuted();
}

TEST_F(TestLinkActionThread, action_thread_should_execute_queued_post_action) {
    EXPECT_CALL(link, postOnActionThread(std::vector<RaceHandle>{3}, 14))
        .WillOnce(::testing::Invoke([this](auto, auto) { markActionExecuted(); }));

    link.start();

    ASSERT_EQ(COMPONENT_OK, link.enqueueContent(14, {0x12, 0x34}));
    ASSERT_EQ(COMPONENT_OK, link.post({3}, 14));
    waitForActionToBeExecuted();
}

class LinkToTestActions : public Link {
public:
    using Link::Link;
    // Move these out to public scope
    using Link::fetchOnActionThread;
    using Link::getInitialIndex;
    using Link::postedMessageHashes;
    using Link::postOnActionThread;
    // Only mocks out the curl-invoking functions
    MOCK_METHOD(int, getIndexFromTimestamp, (double secondsSinceEpoch), (override));
    using GetNewPostsReturnType = std::tuple<std::vector<std::string>, int, double>;
    MOCK_METHOD(GetNewPostsReturnType, getNewPosts, (int latestIndex), (override));
    MOCK_METHOD(bool, postToWhiteboard, (const std::string &message), (override));
};

class TestLinkActions : public ::testing::Test {
public:
    MockTransportSdk sdk;
    LinkAddress address;
    LinkProperties properties;

    // Deferred creation of link so we can tweak the address/properties prior to creating
    std::shared_ptr<LinkToTestActions> createLink() {
        return std::make_shared<LinkToTestActions>("LinkID", address, properties, &sdk);
    }
};

TEST_F(TestLinkActions, get_initial_index_using_persisted_timestamp) {
    address.hostname = "whiteboard";
    address.port = 80;
    address.hashtag = "secret";
    auto link = createLink();
    EXPECT_CALL(sdk, readFile("lastTimestamp:whiteboard:80:secret"))
        .WillOnce(::testing::Return(toByteVector("314159265")));
    EXPECT_CALL(*link, getIndexFromTimestamp(314159265)).WillOnce(::testing::Return(2));
    ASSERT_EQ(2, link->getInitialIndex());
}

TEST_F(TestLinkActions, get_initial_index_using_address_timestamp) {
    address.timestamp = 8675309;
    auto link = createLink();
    EXPECT_CALL(*link, getIndexFromTimestamp(8675309)).WillOnce(::testing::Return(3));
    ASSERT_EQ(3, link->getInitialIndex());
}

TEST_F(TestLinkActions, get_initial_index_using_current_time) {
    auto link = createLink();
    // This test was written at 2022-05-27 13:42, which is 1653673320, so all future runs will be
    // at least that
    EXPECT_CALL(*link, getIndexFromTimestamp(::testing::Gt(1653673320)))
        .WillOnce(::testing::Return(4));
    ASSERT_EQ(4, link->getInitialIndex());
}

TEST_F(TestLinkActions, fetch_multiple_posts) {
    address.hostname = "whiteboard";
    address.port = 80;
    address.hashtag = "secret";

    std::string message1 = "abc";
    std::string message2 = "xyz";

    std::string message1Base64 = "YWJj";
    std::string message2Base64 = "eHl6";

    auto link = createLink();
    EXPECT_CALL(*link, getNewPosts(6))
        .WillOnce(::testing::Return(
            std::tuple{std::vector{message1Base64, message2Base64}, 8, 12345678}));
    EXPECT_CALL(sdk, onReceive("LinkID", ::testing::_,
                               std::vector<uint8_t>{message1.begin(), message1.end()}));
    EXPECT_CALL(sdk, onReceive("LinkID", ::testing::_,
                               std::vector<uint8_t>{message2.begin(), message2.end()}));
    EXPECT_CALL(sdk,
                writeFile("lastTimestamp:whiteboard:80:secret", toByteVector("12345678.000000")))
        .WillOnce(::testing::Return(ChannelResponse{CM_OK, 0}));
    ASSERT_EQ(8, link->fetchOnActionThread(6));
}

TEST_F(TestLinkActions, fetch_own_post) {
    address.hostname = "whiteboard";
    address.port = 80;
    address.hashtag = "secret";

    std::string messageBase64 = "YWJj";

    auto link = createLink();
    EXPECT_CALL(*link, getNewPosts(7))
        .WillOnce(::testing::Return(std::tuple{std::vector{messageBase64}, 8, 12345678}));
    EXPECT_CALL(sdk, onReceive(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(sdk,
                writeFile("lastTimestamp:whiteboard:80:secret", toByteVector("12345678.000000")))
        .WillOnce(::testing::Return(ChannelResponse{CM_OK, 0}));
    link->postedMessageHashes.addMessage(messageBase64);
    ASSERT_EQ(8, link->fetchOnActionThread(7));
}

TEST_F(TestLinkActions, fetch_max_retries) {
    address.maxTries = 2;
    auto link = createLink();
    EXPECT_CALL(*link, getNewPosts(9))
        .Times(2)
        .WillRepeatedly(::testing::Throw(curl_exception(CURLE_COULDNT_RESOLVE_HOST)));
    EXPECT_CALL(sdk, onReceive(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(sdk, updateState(COMPONENT_STATE_FAILED));
    ASSERT_EQ(9, link->fetchOnActionThread(9));
    ASSERT_EQ(9, link->fetchOnActionThread(9));
}

TEST_F(TestLinkActions, fetch_no_new_posts) {
    auto link = createLink();
    EXPECT_CALL(*link, getNewPosts(10))
        .WillOnce(::testing::Return(std::tuple{std::vector<std::string>(), 11, 12345678}));
    EXPECT_CALL(sdk, onReceive(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(sdk, writeFile(::testing::_, ::testing::_)).Times(0);
    ASSERT_EQ(11, link->fetchOnActionThread(10));
}

TEST_F(TestLinkActions, post_max_retries) {
    address.maxTries = 2;
    auto link = createLink();
    EXPECT_CALL(*link, postToWhiteboard(::testing::_))
        .Times(2)
        .WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(sdk, onPackageStatusChanged(4, PACKAGE_FAILED_GENERIC));
    ASSERT_EQ(COMPONENT_OK, link->enqueueContent(7, {0x12, 0x34}));
    link->postOnActionThread({4}, 7);
}

TEST_F(TestLinkActions, post_success) {
    auto link = createLink();

    std::string message = "abc";
    std::string messageBase64 = "YWJj";

    EXPECT_CALL(*link, postToWhiteboard(messageBase64)).WillOnce(::testing::Return(true));
    EXPECT_CALL(sdk, onPackageStatusChanged(4, PACKAGE_SENT));
    ASSERT_EQ(COMPONENT_OK, link->enqueueContent(7, {message.begin(), message.end()}));
    link->postOnActionThread({4}, 7);
}