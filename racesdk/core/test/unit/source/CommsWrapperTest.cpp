
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

#include <jaegertracing/Tracer.h>
#include <valgrind/valgrind.h>  // RUNNING_ON_VALGRIND

#include "../../../source/CommsWrapper.h"
#include "../../common/MockRacePluginComms.h"
#include "../../common/MockRaceSdk.h"
#include "../../common/race_printers.h"
#include "gtest/gtest.h"

// macro because RUNNING_ON_VALGRIND isn't allowed at file scope
#define TIME_MULTIPLIER static_cast<int>(1 + (RUNNING_ON_VALGRIND)*10)

using ::testing::Invoke;
using ::testing::Return;
using ::testing::Unused;

using namespace std::chrono_literals;

TEST(CommsWrapperTest, test_constructor) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);
}

TEST(CommsWrapperTest, test_getters) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    EXPECT_EQ(wrapper.getId(), "MockComms");
    EXPECT_EQ(wrapper.getDescription(), "Mock Comms Testing");
}

TEST(CommsWrapperTest, startHandler) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);
    wrapper.startHandler();

    // destructor should stop handler thread
}

TEST(CommsWrapperTest, start_stop_handler) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);
    wrapper.startHandler();
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, init) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    PluginConfig pluginConfig;
    pluginConfig.etcDirectory = "bloop";
    pluginConfig.loggingDirectory = "foo";
    pluginConfig.auxDataDirectory = "bar";

    EXPECT_CALL(*mockComms, init(pluginConfig)).Times(1).WillOnce(Return(PLUGIN_OK));
    wrapper.init(pluginConfig);
}

TEST(CommsWrapperTest, shutdown_before_init_fails) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);
    EXPECT_CALL(*mockComms, shutdown()).Times(0);

    wrapper.startHandler();
    EXPECT_EQ(wrapper.shutdown(), false);
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, shutdown_after_init_succeeds) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    PluginConfig pluginConfig;
    pluginConfig.etcDirectory = "bloop";
    pluginConfig.loggingDirectory = "foo";
    pluginConfig.auxDataDirectory = "bar";

    EXPECT_CALL(*mockComms, init(pluginConfig)).WillOnce(Return(PLUGIN_OK));
    wrapper.init(pluginConfig);

    EXPECT_CALL(*mockComms, shutdown()).WillOnce(Return(PLUGIN_OK));
    wrapper.startHandler();
    EXPECT_EQ(wrapper.shutdown(), true);
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, sendPackage) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    ConnectionID connId = "MockComms/ConnectionID";
    const std::string cipherText = "my cipher text";
    EncPkg sentPkg({cipherText.begin(), cipherText.end()});
    RaceHandle handle = 42;

    EXPECT_CALL(*mockComms,
                sendPackage(handle, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(1);

    wrapper.startHandler();
    wrapper.openConnection(handle, linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);
    wrapper.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, {}, 0);
    wrapper.sendPackage(handle, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, sendPackage_too_large_package) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    ConnectionID connId = "MockComms/ConnectionID";

    // will never fit in queue. This should cause invalid argument to be returned
    const std::string cipherText(sdk.getRaceConfig().wrapperQueueMaxSize + 1, '0');
    EncPkg sentPkg({cipherText.begin(), cipherText.end()});
    RaceHandle handle = 42;

    // This shouldn't get called
    EXPECT_CALL(*mockComms,
                sendPackage(handle, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(0);

    wrapper.startHandler();
    wrapper.openConnection(handle, linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);
    wrapper.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, {}, 0);
    SdkResponse response = wrapper.sendPackage(handle, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    wrapper.stopHandler();

    EXPECT_EQ(response.status, SDK_INVALID_ARGUMENT);
}

TEST(CommsWrapperTest, sendPackage_queue_full) {
    using namespace std::chrono_literals;
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    ConnectionID connId = "MockComms/ConnectionID";
    const std::string cipherText(sdk.getRaceConfig().wrapperQueueMaxSize / 2 + 1,
                                 '0');  // two won't fit
    EncPkg sentPkg({cipherText.begin(), cipherText.end()});
    RaceHandle handle = 42;
    RaceHandle handle2 = 1337;

    std::promise<void> promise;
    auto future = promise.get_future();

    // This should only get called once
    EXPECT_CALL(*mockComms,
                sendPackage(handle, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(1)
        .WillOnce([&](RaceHandle, std::string, EncPkg, double, uint64_t) {
            future.wait();
            return PLUGIN_OK;
        });

    // This shouldn't get called
    EXPECT_CALL(*mockComms,
                sendPackage(handle2, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(0);

    wrapper.startHandler();
    wrapper.openConnection(handle, linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);
    wrapper.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, {}, 0);
    SdkResponse response1 = wrapper.sendPackage(handle, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    SdkResponse response2 = wrapper.sendPackage(handle2, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    promise.set_value();
    wrapper.stopHandler();

    EXPECT_EQ(response1.status, SDK_OK);
    EXPECT_EQ(response2.status, SDK_QUEUE_FULL);
}

/*
 * Make sure timeout will cause posting to block until space is available
 */
TEST(CommsWrapperTest, sendPackage_queue_full_timeout) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    ConnectionID connId = "MockComms/ConnectionID";
    const std::string cipherText(sdk.getRaceConfig().wrapperQueueMaxSize / 2 + 1,
                                 '0');  // 5MB so two won't fit
    EncPkg sentPkg({cipherText.begin(), cipherText.end()});
    RaceHandle handle = 42;
    RaceHandle handle2 = 1337;

    EXPECT_CALL(*mockComms,
                sendPackage(handle, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(1)
        .WillOnce([&](RaceHandle, std::string, EncPkg, double, uint64_t) {
            std::this_thread::sleep_for(10ms * TIME_MULTIPLIER);
            return PLUGIN_OK;
        });
    EXPECT_CALL(*mockComms,
                sendPackage(handle2, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(1);

    wrapper.startHandler();
    wrapper.openConnection(handle, linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);
    wrapper.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, {}, 0);
    SdkResponse response1 = wrapper.sendPackage(handle, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    SdkResponse response2 =
        wrapper.sendPackage(handle2, connId, sentPkg, 10000, RACE_BATCH_ID_NULL);
    wrapper.stopHandler();

    EXPECT_EQ(response1.status, SDK_OK);
    EXPECT_EQ(response2.status, SDK_OK);
}

TEST(CommsWrapperTest, sendPackage_queue_utilization) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    ConnectionID connId = "MockComms/ConnectionID";
    const std::string cipherText(sdk.getRaceConfig().wrapperQueueMaxSize / 100,
                                 '0');  // should result in 0.01 utilization
    EncPkg sentPkg({cipherText.begin(), cipherText.end()});
    RaceHandle handle = 42;

    EXPECT_CALL(*mockComms,
                sendPackage(handle, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(1);

    wrapper.startHandler();
    wrapper.openConnection(handle, linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);
    wrapper.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, {}, 0);
    SdkResponse response = wrapper.sendPackage(handle, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    wrapper.stopHandler();

    EXPECT_NEAR(response.queueUtilization, 0.01, 0.0001);
}

TEST(CommsWrapperTest, sendPackage_blocked_queue) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    ConnectionID connId = "MockComms/ConnectionID";
    const std::string cipherText = "my cipher text";
    EncPkg sentPkg({cipherText.begin(), cipherText.end()});
    RaceHandle handle = 42;
    RaceHandle handle2 = 43;
    RaceHandle handle3 = 44;

    // First call will block the queue so the second never gets called
    EXPECT_CALL(*mockComms,
                sendPackage(handle2, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(1)
        .WillOnce(Return(PLUGIN_TEMP_ERROR));
    EXPECT_CALL(*mockComms,
                sendPackage(handle3, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(0);

    wrapper.startHandler();
    wrapper.openConnection(handle, linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);
    wrapper.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, {}, 0);
    wrapper.sendPackage(handle2, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    wrapper.sendPackage(handle3, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, sendPackage_blocked_queue_unblock) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    ConnectionID connId = "MockComms/ConnectionID";
    const std::string cipherText = "my cipher text";
    EncPkg sentPkg({cipherText.begin(), cipherText.end()});
    RaceHandle handle = 42;
    RaceHandle handle2 = 43;
    RaceHandle handle3 = 44;
    RaceHandle handle4 = 45;
    RaceHandle handle5 = 46;

    EXPECT_CALL(*mockComms, openConnection(handle, linkType, linkId, linkHints, RACE_UNLIMITED))
        .WillOnce(Return(PLUGIN_OK));
    EXPECT_CALL(*mockComms,
                sendPackage(handle2, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(1)
        .WillOnce(Return(PLUGIN_OK));

    // First call will block the queue, will be called a second time after unblocking
    EXPECT_CALL(*mockComms,
                sendPackage(handle3, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(2)
        .WillOnce(Return(PLUGIN_TEMP_ERROR))
        .WillOnce(Return(PLUGIN_OK));
    EXPECT_CALL(*mockComms,
                sendPackage(handle4, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(1)
        .WillOnce(Return(PLUGIN_OK));

    wrapper.startHandler();
    wrapper.openConnection(handle, linkType, linkId, linkHints, 1, RACE_UNLIMITED, 0);
    wrapper.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, {}, 0);
    wrapper.sendPackage(handle2, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    wrapper.sendPackage(handle3, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    wrapper.sendPackage(handle4, connId, sentPkg, 0, RACE_BATCH_ID_NULL);

    // should unblock the queue
    // invoke in callback to prevent race conditions
    EXPECT_CALL(*mockComms, openConnection(handle5, linkType, linkId, linkHints, RACE_UNLIMITED))
        .WillOnce([&](RaceHandle, LinkType, std::string, std::string, int32_t) {
            wrapper.unblockQueue(connId);
            return PLUGIN_OK;
        });
    wrapper.openConnection(handle5, linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);

    // force all callbacks to complete before stopping
    wrapper.waitForCallbacks();
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, sendPackage_timeout_timestamp_correct) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    ConnectionID connId = "MockComms/ConnectionID";
    const std::string cipherText = "my cipher text";
    EncPkg sentPkg({cipherText.begin(), cipherText.end()});
    RaceHandle handle = 42;
    RaceHandle handle2 = 43;
    int32_t sendTimeout = 12345;

    LinkProperties properties;
    properties.reliable = true;

    std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
    double approxTimestamp = now.count() + sendTimeout;

    EXPECT_CALL(*mockComms, openConnection(handle, linkType, linkId, linkHints, sendTimeout))
        .WillOnce(Return(PLUGIN_OK));
    EXPECT_CALL(*mockComms, sendPackage(handle2, connId, sentPkg, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce([&](RaceHandle, ConnectionID, EncPkg, double timeoutTimestamp, uint64_t) {
            EXPECT_NEAR(timeoutTimestamp, approxTimestamp, 1);
            return PLUGIN_OK;
        });

    wrapper.startHandler();
    wrapper.openConnection(handle, linkType, linkId, linkHints, 1, sendTimeout, 0);
    wrapper.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, properties, 0);
    wrapper.sendPackage(handle2, connId, sentPkg, 0, RACE_BATCH_ID_NULL);

    // force all callbacks to complete before stopping
    wrapper.waitForCallbacks();
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, sendPackage_timeout_package_failed) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    ConnectionID connId = "MockComms/ConnectionID";
    ConnectionID connId2 = "MockComms/ConnectionID2";
    const std::string cipherText = "my cipher text";
    EncPkg sentPkg({cipherText.begin(), cipherText.end()});
    RaceHandle handle = 42;
    RaceHandle handle2 = 43;
    RaceHandle handle3 = 44;
    RaceHandle handle4 = 45;
    int32_t sendTimeout1 = RACE_UNLIMITED;
    int32_t sendTimeout2 = 0;

    LinkProperties properties;
    properties.reliable = true;

    std::promise<void> promise;

    EXPECT_CALL(*mockComms, openConnection(handle, linkType, linkId, linkHints, sendTimeout1))
        .WillOnce(Return(PLUGIN_OK));
    EXPECT_CALL(*mockComms, openConnection(handle2, linkType, linkId, linkHints, sendTimeout2))
        .WillOnce(Return(PLUGIN_OK));
    EXPECT_CALL(*mockComms, sendPackage(::testing::_, ::testing::_, ::testing::_, ::testing::_,
                                        RACE_BATCH_ID_NULL))
        .Times(1)
        .WillOnce([&](RaceHandle, ConnectionID, EncPkg, double, uint64_t) {
            promise.get_future().wait();
            return PLUGIN_OK;
        });

    EXPECT_CALL(sdk, onPackageStatusChanged(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(Return(SDK_OK));

    wrapper.startHandler();
    wrapper.openConnection(handle, linkType, linkId, linkHints, 1, sendTimeout1, 0);
    wrapper.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, properties, 0);
    wrapper.openConnection(handle2, linkType, linkId, linkHints, 1, sendTimeout2, 0);
    wrapper.onConnectionStatusChanged(handle2, connId2, CONNECTION_OPEN, properties, 0);
    wrapper.sendPackage(handle3, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    wrapper.sendPackage(handle4, connId2, sentPkg, 0, RACE_BATCH_ID_NULL);

    // have to wait for handler timeout thread to timeout the second post
    std::this_thread::sleep_for(10ms * TIME_MULTIPLIER);
    promise.set_value();

    // force all callbacks to complete before stopping
    wrapper.waitForCallbacks();
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, openConnection) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    RaceHandle handle = 42;

    EXPECT_CALL(*mockComms, openConnection(handle, linkType, linkId, linkHints, RACE_UNLIMITED))
        .Times(1);

    wrapper.startHandler();
    wrapper.openConnection(handle, linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, openConnection_priority) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "MockComms/LinkID1";
    ConnectionID connId1 = "MockComms/LinkID1_ConnectionID1";
    ConnectionID connId2 = "MockComms/LinkID2_ConnectionID2";
    std::string linkHints = "{}";
    LinkProperties properties;

    int timeout = 12;

    EncPkg pkg({});

    EXPECT_CALL(*mockComms, openConnection(1, linkType, linkId, linkHints, RACE_UNLIMITED))
        .Times(1);
    EXPECT_CALL(*mockComms, openConnection(2, linkType, linkId, linkHints, RACE_UNLIMITED))
        .Times(1);

    auto wrapperValidator = [&wrapper](const CommsWrapper &arg) { return &wrapper == &arg; };
    EXPECT_CALL(sdk, onConnectionStatusChanged(::testing::Truly(wrapperValidator), 1, connId1,
                                               CONNECTION_OPEN, properties, timeout))
        .Times(1)
        .WillOnce(Return(SDK_OK));
    EXPECT_CALL(sdk, onConnectionStatusChanged(::testing::Truly(wrapperValidator), 2, connId2,
                                               CONNECTION_OPEN, properties, timeout))
        .Times(1)
        .WillOnce(Return(SDK_OK));

    int count = 0;
    int result1 = -1;
    int result2 = -1;
    int result3 = -1;
    int result4 = -1;

    EXPECT_CALL(*mockComms,
                sendPackage(3, connId1, ::testing::_, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .WillOnce(Invoke([&count, &result1](Unused, Unused, Unused, Unused, Unused) {
            result1 = count++;
            return PLUGIN_OK;
        }));
    EXPECT_CALL(*mockComms,
                sendPackage(4, connId2, ::testing::_, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .WillOnce(Invoke([&count, &result2](Unused, Unused, Unused, Unused, Unused) {
            result2 = count++;
            return PLUGIN_OK;
        }));
    EXPECT_CALL(*mockComms,
                sendPackage(5, connId1, ::testing::_, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .WillOnce(Invoke([&count, &result3](Unused, Unused, Unused, Unused, Unused) {
            result3 = count++;
            return PLUGIN_OK;
        }));
    EXPECT_CALL(*mockComms,
                sendPackage(6, connId2, ::testing::_, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .WillOnce(Invoke([&count, &result4](Unused, Unused, Unused, Unused, Unused) {
            result4 = count++;
            return PLUGIN_OK;
        }));

    wrapper.openConnection(1, linkType, linkId, linkHints, 1, RACE_UNLIMITED, 0);
    wrapper.openConnection(2, linkType, linkId, linkHints, 2, RACE_UNLIMITED, 0);

    // slight abuse by getting callbacks before the handler runs, but should work
    wrapper.onConnectionStatusChanged(1, connId1, CONNECTION_OPEN, properties, timeout);
    wrapper.onConnectionStatusChanged(2, connId2, CONNECTION_OPEN, properties, timeout);

    wrapper.sendPackage(3, connId1, pkg, 0, RACE_BATCH_ID_NULL);
    wrapper.sendPackage(4, connId2, pkg, 0, RACE_BATCH_ID_NULL);
    wrapper.sendPackage(5, connId1, pkg, 0, RACE_BATCH_ID_NULL);
    wrapper.sendPackage(6, connId2, pkg, 0, RACE_BATCH_ID_NULL);

    wrapper.startHandler();
    wrapper.stopHandler();

    EXPECT_EQ(result1, 2);
    EXPECT_EQ(result2, 0);
    EXPECT_EQ(result3, 3);
    EXPECT_EQ(result4, 1);
}

TEST(CommsWrapperTest, closeConnection) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    ConnectionID connId = "MockComms/ConnectionID";
    RaceHandle handle = 42;

    EXPECT_CALL(*mockComms, closeConnection(handle, connId)).Times(1);

    wrapper.startHandler();
    wrapper.openConnection(handle, linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);
    wrapper.onConnectionStatusChanged(handle, connId, CONNECTION_OPEN, {}, 0);
    wrapper.closeConnection(handle, connId, 0);
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, destroyLink) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkID linkId = "LinkId";
    RaceHandle handle = 42;

    EXPECT_CALL(*mockComms, destroyLink(handle, linkId)).Times(1);

    wrapper.startHandler();
    wrapper.destroyLink(handle, linkId, 0);
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, createLink) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    std::string channelGid = "channel1";
    RaceHandle handle = 42;

    EXPECT_CALL(*mockComms, createLink(handle, channelGid)).Times(1);

    wrapper.startHandler();
    wrapper.createLink(handle, channelGid, 0);
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, loadLinkAddress) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    std::string channelGid = "channel1";
    std::string linkAddress = "{}";
    RaceHandle handle = 42;

    EXPECT_CALL(*mockComms, loadLinkAddress(handle, channelGid, linkAddress)).Times(1);

    wrapper.startHandler();
    wrapper.loadLinkAddress(handle, channelGid, linkAddress, 0);
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, loadLinkAddressses) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    std::string channelGid = "channel1";
    std::vector<std::string> linkAddresses = {"{}", "{}"};
    RaceHandle handle = 42;

    EXPECT_CALL(*mockComms, loadLinkAddresses(handle, channelGid, linkAddresses)).Times(1);

    wrapper.startHandler();
    wrapper.loadLinkAddresses(handle, channelGid, linkAddresses, 0);
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, deactivateChannel) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    std::string channelGid = "channel1";
    RaceHandle handle = 42;

    EXPECT_CALL(*mockComms, deactivateChannel(handle, channelGid)).Times(1);

    wrapper.startHandler();
    wrapper.deactivateChannel(handle, channelGid, 0);
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, onUserInputReceived) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    EXPECT_CALL(*mockComms, onUserInputReceived(0x11223344l, true, "expected-response")).Times(1);

    wrapper.startHandler();
    wrapper.onUserInputReceived(0x11223344l, true, "expected-response", 0);
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, getEntropy) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    uint32_t numBytes = 1;
    std::vector<uint8_t> bytes = {0x42};

    EXPECT_CALL(sdk, getEntropy(numBytes)).Times(1).WillOnce(Return(bytes));
    auto ret = wrapper.getEntropy(numBytes);

    EXPECT_EQ(ret, bytes);
}

TEST(CommsWrapperTest, getActivePersona) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    std::string persona = "persona";

    EXPECT_CALL(sdk, getActivePersona()).Times(1).WillOnce(Return(persona));
    auto ret = wrapper.getActivePersona();

    EXPECT_EQ(ret, persona);
}

TEST(CommsWrapperTest, DISABLED_asyncError) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    RaceHandle handle = 42;
    PluginResponse status = PLUGIN_ERROR;

    EXPECT_CALL(sdk, asyncError(handle, status)).Times(1).WillOnce(Return(SDK_OK));
    auto ret = wrapper.asyncError(handle, status);

    EXPECT_EQ(ret.status, SDK_OK);
}

TEST(CommsWrapperTest, asyncFatal) {
    using ::testing::_;
    using testing::SetArgReferee;
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    EXPECT_CALL(sdk, shutdownPluginAsync(_)).WillOnce(Return());
    auto ret = wrapper.asyncError(42, PLUGIN_FATAL);
    EXPECT_EQ(ret.status, SDK_OK);
}

TEST(CommsWrapperTest, onPackageStatusChanged) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    RaceHandle handle = 53;
    PackageStatus status = PACKAGE_SENT;
    int timeout = 13;

    auto wrapperValidator = [&wrapper](const CommsWrapper &arg) { return &wrapper == &arg; };
    EXPECT_CALL(sdk,
                onPackageStatusChanged(::testing::Truly(wrapperValidator), handle, status, timeout))
        .Times(1)
        .WillOnce(Return(SDK_OK));
    auto ret = wrapper.onPackageStatusChanged(handle, status, timeout);

    EXPECT_EQ(ret.status, SDK_OK);
}

TEST(CommsWrapperTest, onConnectionStatusChanged_open_error) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    RaceHandle handle = 64;
    ConnectionID connId = "MockComms/Connection_1";
    ConnectionStatus status = CONNECTION_OPEN;
    LinkProperties properties;
    int timeout = 13;

    // should fail in the wrapper and not call the sdk
    EXPECT_CALL(sdk, onConnectionStatusChanged(::testing::_, ::testing::_, ::testing::_,
                                               ::testing::_, ::testing::_, ::testing::_))
        .Times(0);
    auto ret = wrapper.onConnectionStatusChanged(handle, connId, status, properties, timeout);

    EXPECT_EQ(ret.status, SDK_INVALID_ARGUMENT);
}

TEST(CommsWrapperTest, onConnectionStatusChanged_open) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    RaceHandle handle = 42;
    ConnectionID connId = "MockComms/Connection_1";
    ConnectionStatus status = CONNECTION_OPEN;
    LinkProperties properties;
    int timeout = 13;

    auto wrapperValidator = [&wrapper](const CommsWrapper &arg) { return &wrapper == &arg; };
    EXPECT_CALL(*mockComms, openConnection(handle, linkType, linkId, linkHints, RACE_UNLIMITED))
        .Times(1);
    EXPECT_CALL(sdk, onConnectionStatusChanged(::testing::Truly(wrapperValidator), handle, connId,
                                               status, properties, timeout))
        .Times(1)
        .WillOnce(Return(SDK_OK));

    wrapper.startHandler();
    wrapper.openConnection(handle, linkType, linkId, linkHints, 1, RACE_UNLIMITED, 0);
    auto ret = wrapper.onConnectionStatusChanged(handle, connId, status, properties, timeout);
    wrapper.stopHandler();

    EXPECT_EQ(ret.status, SDK_OK);
}

TEST(CommsWrapperTest, onConnectionStatusChanged_open_error_on_called_twice) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    RaceHandle handle = 42;
    ConnectionID connId = "MockComms/Connection_1";
    ConnectionStatus status = CONNECTION_OPEN;
    LinkProperties properties;
    int timeout = 13;

    auto wrapperValidator = [&wrapper](const CommsWrapper &arg) { return &wrapper == &arg; };
    EXPECT_CALL(*mockComms, openConnection(handle, linkType, linkId, linkHints, RACE_UNLIMITED))
        .Times(1);
    EXPECT_CALL(sdk, onConnectionStatusChanged(::testing::Truly(wrapperValidator), handle, connId,
                                               status, properties, timeout))
        .Times(1)
        .WillOnce(Return(SDK_OK));

    wrapper.startHandler();
    wrapper.openConnection(handle, linkType, linkId, linkHints, 1, RACE_UNLIMITED, 0);
    auto ret1 = wrapper.onConnectionStatusChanged(handle, connId, status, properties, timeout);
    auto ret2 = wrapper.onConnectionStatusChanged(handle, connId, status, properties, timeout);
    wrapper.stopHandler();

    EXPECT_EQ(ret1.status, SDK_OK);
    EXPECT_EQ(ret2.status, SDK_INVALID_ARGUMENT);
}

TEST(CommsWrapperTest, onConnectionStatusChanged_closed_no_open) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    RaceHandle handle = 64;
    ConnectionID connId = "MockComms/Connection_1";
    ConnectionStatus status = CONNECTION_CLOSED;
    LinkProperties properties;
    int timeout = 13;

    // this should still work. even though the wrapper should warn about trying to close a
    // non-existent queue
    auto wrapperValidator = [&wrapper](const CommsWrapper &arg) { return &wrapper == &arg; };
    EXPECT_CALL(sdk, onConnectionStatusChanged(::testing::Truly(wrapperValidator), handle, connId,
                                               status, properties, timeout))
        .Times(1)
        .WillOnce(Return(SDK_OK));
    auto ret = wrapper.onConnectionStatusChanged(handle, connId, status, properties, timeout);

    EXPECT_EQ(ret.status, SDK_OK);
}

TEST(CommsWrapperTest, updateLinkProperties) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkID linkId = "MockComms/Link-1";
    LinkProperties properties;
    int timeout = 13;

    auto wrapperValidator = [&wrapper](const CommsWrapper &arg) { return &wrapper == &arg; };
    EXPECT_CALL(
        sdk, updateLinkProperties(::testing::Truly(wrapperValidator), linkId, properties, timeout))
        .Times(1)
        .WillOnce(Return(SDK_OK));
    auto ret = wrapper.updateLinkProperties(linkId, properties, timeout);

    EXPECT_EQ(ret.status, SDK_OK);
}

TEST(CommsWrapperTest, generateConnectionId) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkID linkId = "MockComms/Link-1";
    ConnectionID connId = "MockComms/Link-1_Connection-1";

    auto wrapperValidator = [&wrapper](const CommsWrapper &arg) { return &wrapper == &arg; };
    EXPECT_CALL(sdk, generateConnectionId(::testing::Truly(wrapperValidator), linkId))
        .Times(1)
        .WillOnce(Return(connId));
    auto ret = wrapper.generateConnectionId(linkId);

    EXPECT_EQ(ret, connId);
}

TEST(CommsWrapperTest, generateLinkId) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkID linkId = "MockComms//channel1/Link-1";

    auto wrapperValidator = [&wrapper](const CommsWrapper &arg) { return &wrapper == &arg; };
    EXPECT_CALL(sdk, generateLinkId(::testing::Truly(wrapperValidator), "channel1"))
        .Times(1)
        .WillOnce(Return(linkId));
    auto ret = wrapper.generateLinkId("channel1");

    EXPECT_EQ(ret, linkId);
}

TEST(CommsWrapperTest, receiveEncPkg) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    std::vector<ConnectionID> connIds = {"MockComms/ConnectionID-1", "MockComms/ConnectionID-2"};
    const std::string cipherText = "my cipher text";
    EncPkg sentPkg({cipherText.begin(), cipherText.end()});
    int timeout = 13;

    auto wrapperValidator = [&wrapper](const CommsWrapper &arg) { return &wrapper == &arg; };
    auto packageValidator = [&sentPkg](const EncPkg &pkg) {
        return sentPkg.getCipherText() == pkg.getCipherText();
    };
    EXPECT_CALL(sdk, receiveEncPkg(::testing::Truly(wrapperValidator),
                                   ::testing::Truly(packageValidator), connIds, timeout))
        .Times(1)
        .WillOnce(Return(SDK_OK));
    auto ret = wrapper.receiveEncPkg(sentPkg, connIds, timeout);

    EXPECT_EQ(ret.status, SDK_OK);
}

TEST(CommsWrapperTest, sendPackage_calls_package_failed_after_shutdown) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    ConnectionID connId = "MockComms/ConnectionID";
    const std::string cipherText = "my cipher text";
    EncPkg sentPkg({cipherText.begin(), cipherText.end()});
    RaceHandle handle1 = 1;
    RaceHandle handle2 = 2;
    RaceHandle handle3 = 3;

    std::promise<void> promise;
    std::promise<void> promise2;
    EXPECT_CALL(*mockComms, openConnection(::testing::_, ::testing::_, ::testing::_, ::testing::_,
                                           ::testing::_))
        .Times(1);
    EXPECT_CALL(sdk, onConnectionStatusChanged(::testing::_, ::testing::_, ::testing::_,
                                               ::testing::_, ::testing::_, ::testing::_))
        .Times(1);
    EXPECT_CALL(*mockComms,
                sendPackage(handle2, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(1)
        .WillOnce([&](RaceHandle, ConnectionID, EncPkg, double, uint64_t) {
            promise2.set_value();
            promise.get_future().wait();
            return PLUGIN_OK;
        });
    EXPECT_CALL(sdk,
                onPackageStatusChanged(::testing::_, handle3, PACKAGE_FAILED_GENERIC, ::testing::_))
        .Times(1);
    EXPECT_CALL(*mockComms,
                sendPackage(handle3, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(0);

    wrapper.startHandler();
    wrapper.openConnection(handle1, linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);
    wrapper.onConnectionStatusChanged(handle1, connId, CONNECTION_OPEN, {}, 0);
    wrapper.sendPackage(handle2, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    wrapper.sendPackage(handle3, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    promise2.get_future().wait();
    wrapper.shutdown(0);
    promise.set_value();
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, openConnection_calls_connection_failed_after_shutdown) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    ConnectionID connId = "MockComms/ConnectionID";
    const std::string cipherText = "my cipher text";
    EncPkg sentPkg({cipherText.begin(), cipherText.end()});
    RaceHandle handle1 = 1;
    RaceHandle handle2 = 2;

    std::promise<void> promise;
    std::promise<void> promise2;
    EXPECT_CALL(*mockComms,
                openConnection(handle1, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce([&](RaceHandle, LinkType, LinkID, std::string, int32_t) {
            promise2.set_value();
            promise.get_future().wait();
            return PLUGIN_OK;
        });
    EXPECT_CALL(*mockComms,
                openConnection(handle2, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(0);
    EXPECT_CALL(sdk, onConnectionStatusChanged(::testing::_, handle2, ::testing::_,
                                               CONNECTION_CLOSED, ::testing::_, ::testing::_))
        .Times(1);

    wrapper.startHandler();
    wrapper.openConnection(handle1, linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);
    wrapper.openConnection(handle2, linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);
    promise2.get_future().wait();
    wrapper.shutdown(0);
    promise.set_value();
    wrapper.stopHandler();
}

TEST(CommsWrapperTest, closeConnection_after_shutdown) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    LinkType linkType = LT_SEND;
    LinkID linkId = "LinkID";
    std::string linkHints = "{}";
    ConnectionID connId = "MockComms/ConnectionID";
    const std::string cipherText = "my cipher text";
    EncPkg sentPkg({cipherText.begin(), cipherText.end()});
    RaceHandle handle1 = 1;
    RaceHandle handle2 = 2;
    RaceHandle handle3 = 3;

    std::promise<void> promise;
    std::promise<void> promise2;
    EXPECT_CALL(*mockComms,
                openConnection(handle1, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(1);
    EXPECT_CALL(*mockComms,
                sendPackage(handle2, connId, sentPkg, std::numeric_limits<double>::infinity(),
                            RACE_BATCH_ID_NULL))
        .Times(1)
        .WillOnce([&](RaceHandle, ConnectionID, EncPkg, double, uint64_t) {
            promise2.set_value();
            promise.get_future().wait();
            return PLUGIN_OK;
        });
    EXPECT_CALL(*mockComms, closeConnection(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(sdk, onConnectionStatusChanged(::testing::_, ::testing::_, ::testing::_,
                                               ::testing::_, ::testing::_, ::testing::_))
        .Times(1);

    wrapper.startHandler();
    wrapper.openConnection(handle1, linkType, linkId, linkHints, 0, RACE_UNLIMITED, 0);
    wrapper.onConnectionStatusChanged(handle1, connId, CONNECTION_OPEN, {}, 0);
    wrapper.sendPackage(handle2, connId, sentPkg, 0, RACE_BATCH_ID_NULL);
    wrapper.closeConnection(handle3, connId, 0);
    promise2.get_future().wait();
    wrapper.shutdown(0);
    promise.set_value();
    wrapper.stopHandler();
}

/**
 * @brief The constructor has an optional parameter for the configuration path. If an argument is
 * NOT provided then it should default to use the provided plugin ID.
 *
 */
TEST(CommsWrapperTest, config_path_should_default_to_id) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk);

    EXPECT_EQ(wrapper.getConfigPath(), "MockComms");
}

/**
 * @brief The constructor has an optional parameter for the configuration path. If an argument is
 * provided then it should set the config path for the object.
 *
 */
TEST(CommsWrapperTest, constructor_should_set_the_config_path) {
    auto mockComms = std::make_shared<MockRacePluginComms>();
    MockRaceSdk sdk;
    CommsWrapper wrapper(mockComms, "MockComms", "Mock Comms Testing", sdk, "my/config/path/");

    EXPECT_EQ(wrapper.getConfigPath(), "my/config/path/");
}
