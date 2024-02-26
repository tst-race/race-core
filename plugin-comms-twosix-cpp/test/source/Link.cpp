
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

#include "../../source/base/Link.h"

#include <valgrind/valgrind.h>

#include <future>
#include <memory>

#include "MockChannel.h"
#include "MockLink.h"
#include "MockPluginComms.h"
#include "gtest/gtest.h"
#include "race/mocks/MockRaceSdkComms.h"

// macro because RUNNING_ON_VALGRIND isn't allowed at file scope
#define TIME_MULTIPLIER static_cast<int>(1 + (RUNNING_ON_VALGRIND)*10)

using ::testing::_;
using ::testing::Return;
using namespace std::chrono_literals;

// implement the pure virtual methods in order to test the implementation of
// methods that are actually implemented for Link
class TestLink : public Link {
public:
    TestLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin, Channel *channel,
             const LinkID &linkId, const LinkProperties &linkProperties,
             const LinkProfileParser &parser) :
        Link(sdk, plugin, channel, linkId, linkProperties, parser) {}

    ~TestLink() {
        shutdownLink();
    }

    MOCK_METHOD(std::shared_ptr<Connection>, openConnection,
                (LinkType linkType, const ConnectionID &connectionId, const std::string &linkHints,
                 int32_t sendTimeout),
                (override));
    MOCK_METHOD(void, closeConnection, (const ConnectionID &connectionId), (override));
    MOCK_METHOD(void, startConnection, (Connection *), (override));
    MOCK_METHOD(bool, sendPackageInternal, (RaceHandle handle, const EncPkg &pkg), (override));
    MOCK_METHOD(void, shutdownInternal, (), (override));
    MOCK_METHOD(std::string, getLinkAddress, (), (override));

    void setConnections(const std::vector<std::shared_ptr<Connection>> &connections) {
        mConnections = connections;
    }

    bool getShutdown() {
        return mShutdown;
    }

    bool getSendThreadFinished() {
        return mSendThreadShutdown;
    }

    size_t getSendQueueMaxCapacity() {
        return SEND_QUEUE_MAX_CAPACITY;
    }

    size_t getSendQueueSize() {
        return mSendQueue.size();
    }
};

TEST(Link, constructor_link_type_send) {
    LinkProperties linkProperties;
    linkProperties.linkType = LT_SEND;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);

    EXPECT_EQ(testLink.getShutdown(), false);
    EXPECT_EQ(testLink.getSendThreadFinished(), false);
}

TEST(Link, constructor_link_type_recv) {
    LinkProperties linkProperties;
    linkProperties.linkType = LT_RECV;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);

    EXPECT_EQ(testLink.getShutdown(), false);
    EXPECT_EQ(testLink.getSendThreadFinished(), true);
}

TEST(Link, sendPackage) {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_SEND;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);

    std::promise<void> promise;

    RaceHandle handle = 0;
    EncPkg pkg(0, 0, {0, 1, 2, 3});
    EXPECT_CALL(testLink, sendPackageInternal(handle, pkg))
        .WillOnce([&promise](RaceHandle, const EncPkg &) {
            promise.set_value();
            return true;
        });
    testLink.sendPackage(handle, pkg, std::numeric_limits<double>::infinity());
    promise.get_future().wait();
}

TEST(Link, sendPackage_link_sleeps_after_sending_amount) {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_SEND;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    parser.send_period_length = 0;
    parser.send_period_amount = 1;
    parser.sleep_period_length = 10;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);

    std::promise<void> promise;

    RaceHandle handle = 0;
    EncPkg pkg(0, 0, {0, 1, 2, 3});
    EXPECT_CALL(sdk, onPackageStatusChanged(handle, PACKAGE_FAILED_TIMEOUT, 0)).Times(0);
    EXPECT_CALL(sdk, onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, 0)).Times(1);
    EXPECT_CALL(testLink, sendPackageInternal(_, _))
        .Times(1)
        .WillOnce([&promise](RaceHandle, const EncPkg &) {
            promise.set_value();
            return true;
        });
    testLink.sendPackage(handle, pkg, std::numeric_limits<double>::infinity());
    testLink.sendPackage(handle, pkg, std::numeric_limits<double>::infinity());
    promise.get_future().wait();

    // the link should now be sleeping
    // wait to make the second package isn't called
    std::this_thread::sleep_for(10ms * TIME_MULTIPLIER);
}

TEST(Link, sendPackage_link_wakes_after_sending_amount) {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_SEND;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    parser.send_period_length = 0;
    parser.send_period_amount = 1;
    parser.sleep_period_length = 0;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);

    std::promise<void> promise;
    std::promise<void> promise2;

    RaceHandle handle = 0;
    EncPkg pkg(0, 0, {0, 1, 2, 3});
    EXPECT_CALL(sdk, onPackageStatusChanged(handle, PACKAGE_FAILED_TIMEOUT, 0)).Times(0);
    EXPECT_CALL(testLink, sendPackageInternal(_, _))
        .Times(2)
        .WillOnce([&promise](RaceHandle, const EncPkg &) {
            promise.set_value();
            return true;
        })
        .WillOnce([&promise2](RaceHandle, const EncPkg &) {
            promise2.set_value();
            return true;
        });
    testLink.sendPackage(handle, pkg, std::numeric_limits<double>::infinity());
    testLink.sendPackage(handle, pkg, std::numeric_limits<double>::infinity());
    promise.get_future().wait();

    // the link should go to sleep, but wake up immediately
    // wait to see if the second package gets called
    promise2.get_future().wait();
}

TEST(Link, sendPackage_link_calls_package_timeout_on_send_amount_sleep) {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_SEND;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    parser.send_period_length = 0;
    parser.send_period_amount = 1;
    parser.sleep_period_length = 10;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);

    std::promise<void> promise;
    std::promise<void> promise2;

    RaceHandle handle = 0;
    EncPkg pkg(0, 0, {0, 1, 2, 3});
    EXPECT_CALL(sdk, onPackageStatusChanged(handle, PACKAGE_FAILED_TIMEOUT, _))
        .Times(1)
        .WillOnce([&promise2](RaceHandle, PackageStatus, int32_t) {
            promise2.set_value();
            return SDK_OK;
        });
    EXPECT_CALL(testLink, sendPackageInternal(_, _))
        .Times(1)
        .WillOnce([&promise](RaceHandle, const EncPkg &) {
            promise.set_value();
            return true;
        });
    testLink.sendPackage(handle, pkg, std::numeric_limits<double>::infinity());
    testLink.sendPackage(handle, pkg, 0);
    promise.get_future().wait();
    promise2.get_future().wait();
}

TEST(Link, sendPackage_link_sleeps_after_time) {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_SEND;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    parser.send_period_length = 0.001;
    parser.send_period_amount = 0;
    parser.sleep_period_length = 10 * TIME_MULTIPLIER;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);

    RaceHandle handle = 0;
    EncPkg pkg(0, 0, {0, 1, 2, 3});
    EXPECT_CALL(sdk, onPackageStatusChanged(handle, PACKAGE_FAILED_TIMEOUT, 0)).Times(0);
    EXPECT_CALL(sdk, onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, 0)).Times(1);
    EXPECT_CALL(testLink, sendPackageInternal(_, _)).Times(0);
    // wait for send thread to go to sleep
    std::this_thread::sleep_for(10ms * TIME_MULTIPLIER);
    testLink.sendPackage(handle, pkg, std::numeric_limits<double>::infinity());

    // the link should now be sleeping
    // wait to make the second package isn't called
    std::this_thread::sleep_for(10ms * TIME_MULTIPLIER);
}

TEST(Link, sendPackage_link_wakes_after_time) {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_SEND;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    parser.send_period_length = 0.010 * TIME_MULTIPLIER;
    parser.send_period_amount = 0;
    parser.sleep_period_length = 0.020 * TIME_MULTIPLIER;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);

    std::promise<void> promise;

    RaceHandle handle = 0;
    EncPkg pkg(0, 0, {0, 1, 2, 3});
    EXPECT_CALL(sdk, onPackageStatusChanged(handle, PACKAGE_FAILED_TIMEOUT, 0)).Times(0);
    EXPECT_CALL(testLink, sendPackageInternal(_, _))
        .Times(1)
        .WillOnce([&promise](RaceHandle, const EncPkg &) {
            promise.set_value();
            return true;
        });
    // wait for send thread to go to sleep
    std::this_thread::sleep_for(20ms * TIME_MULTIPLIER);
    testLink.sendPackage(handle, pkg, std::numeric_limits<double>::infinity());

    // the link should go to sleep, but wake up soon afterward
    // wait to see if the package gets called
    promise.get_future().wait();
}

TEST(Link, sendPackage_link_calls_package_timeout_on_send_length_sleep) {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_SEND;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    parser.send_period_length = 0.001;
    parser.send_period_amount = 0;
    parser.sleep_period_length = 10 * TIME_MULTIPLIER;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);

    std::promise<void> promise;

    RaceHandle handle = 0;
    EncPkg pkg(0, 0, {0, 1, 2, 3});
    EXPECT_CALL(sdk, onPackageStatusChanged(handle, PACKAGE_FAILED_TIMEOUT, RACE_BLOCKING))
        .Times(1)
        .WillOnce([&promise](RaceHandle, PackageStatus, int32_t) {
            promise.set_value();
            return SDK_OK;
        });
    EXPECT_CALL(testLink, sendPackageInternal(_, _)).Times(0);
    // wait for send thread to go to sleep
    std::this_thread::sleep_for(10ms * TIME_MULTIPLIER);
    testLink.sendPackage(handle, pkg, 0);

    // the link should now be sleeping
    // wait to make the second package times out
    promise.get_future().wait();
}

// Test to make sure Link will respond with PLUGIN_TEMP_ERROR when its internal
// queue is full A link's internal queue will store a few items for the send
// thread to process when it is free, if this queue fills up, the link responds
// with PLUGIN_TEMP_ERROR and the sdk should stop attempting to send on this
// link until a package is either sent or fails
TEST(Link, sendPackage_temp_error) {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_SEND;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);

    std::promise<void> promise0;
    std::promise<void> promise1;
    std::promise<void> promise2;

    RaceHandle handle = 0;
    EncPkg pkg(0, 0, {});
    std::atomic<size_t> count = 0;

    // These actions happen on the send thread. The first time a package gets
    // processed it will signal the main thread, and then block until it receives
    // a signal. After that, it keeps count of all the packages processed and
    // signals the main thread when it reaches the expected amount. The expected
    // amount is the send queue max capacity even though the test attempt to send
    // one more than that because the last one the test sends is expected to fail.
    EXPECT_CALL(testLink, sendPackageInternal(handle, pkg))
        .Times(static_cast<int>(testLink.getSendQueueMaxCapacity()) + 1)
        .WillOnce([&promise0, &promise1](RaceHandle, const EncPkg &) {
            promise0.set_value();
            promise1.get_future().wait();
            return true;
        })
        .WillRepeatedly([&promise2, &testLink, &count](RaceHandle, const EncPkg &) {
            count++;
            if (count == testLink.getSendQueueMaxCapacity()) {
                promise2.set_value();
            }
            return true;
        });

    // send the first package and wait for it to be start being processed. This
    // gets popped off the send queue, so it does not count toward the send queue
    // limit.
    auto response = testLink.sendPackage(handle, pkg, std::numeric_limits<double>::infinity());
    EXPECT_EQ(response, PLUGIN_OK);
    promise0.get_future().wait();

    // send the maximum number of packages that the queue can hold. These should
    // all get added to the send queue
    for (size_t i = 0; i < testLink.getSendQueueMaxCapacity(); ++i) {
        response = testLink.sendPackage(handle, pkg, std::numeric_limits<double>::infinity());
        EXPECT_EQ(response, PLUGIN_OK);
        EXPECT_EQ(testLink.getSendQueueSize(), i + 1);
        EXPECT_EQ(count, 0);
    }

    // try to send one more. The send queue is full so it should fail. The
    // response PLUGIN_TEMP_ERROR should inform the sdk to try again later.
    response = testLink.sendPackage(handle, pkg, std::numeric_limits<double>::infinity());
    EXPECT_EQ(response, PLUGIN_TEMP_ERROR);
    EXPECT_EQ(testLink.getSendQueueSize(), 10);
    EXPECT_EQ(count, 0);

    // signal the send thread to continue and to process the rest of the packages,
    // and wait until it's done.
    promise1.set_value();
    promise2.get_future().wait();
}

TEST(Link, sendPackage_drop_package) {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_SEND;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    parser.send_drop_rate = 1;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);
    RaceHandle handle = 0;
    EncPkg pkg(0, 0, {0, 1, 2, 3});

    std::promise<void> promise;

    EXPECT_CALL(testLink, sendPackageInternal(handle, pkg)).Times(0);
    EXPECT_CALL(sdk, onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING))
        .Times(1)
        .WillOnce([&promise](RaceHandle, PackageStatus, int32_t) {
            promise.set_value();
            return SDK_OK;
        });

    testLink.sendPackage(handle, pkg, std::numeric_limits<double>::infinity());
    promise.get_future().wait();
    testLink.shutdown();
}

TEST(Link, sendPackage_corrupt_package) {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_SEND;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    parser.send_corrupt_rate = 1;
    parser.send_corrupt_amount = 100;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);

    std::promise<void> promise;

    RaceHandle handle = 0;
    EncPkg pkg(0, 0, {0, 1, 2, 3});
    EXPECT_CALL(testLink, sendPackageInternal(handle, _))
        .Times(1)
        .WillOnce([&](RaceHandle, const EncPkg &recvPkg) {
            EXPECT_NE(pkg, recvPkg);
            promise.set_value();
            return true;
        });
    testLink.sendPackage(handle, pkg, std::numeric_limits<double>::infinity());
    promise.get_future().wait();
}

TEST(Link, shutdown) {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_SEND;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);

    EXPECT_CALL(testLink, shutdownInternal()).Times(1);
    EXPECT_EQ(testLink.getShutdown(), false);
    EXPECT_EQ(testLink.getSendThreadFinished(), false);
    testLink.shutdown();
    EXPECT_EQ(testLink.getShutdown(), true);
    EXPECT_EQ(testLink.getSendThreadFinished(), true);
}

TEST(Link, shutdown_recv) {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_RECV;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    TestLink testLink(&sdk, &plugin, &channel, {}, linkProperties, parser);

    EXPECT_EQ(testLink.getShutdown(), false);
    EXPECT_EQ(testLink.getSendThreadFinished(), true);
    testLink.shutdown();
    EXPECT_EQ(testLink.getShutdown(), true);
    EXPECT_EQ(testLink.getSendThreadFinished(), true);
}

TEST(Link, test_getters) {
    LinkID linkId = "some link id";
    LinkProperties linkProperties;

    linkProperties.linkType = LT_RECV;
    linkProperties.transmissionType = TT_UNICAST;
    linkProperties.connectionType = CT_DIRECT;

    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);
    LinkProfileParser parser;
    TestLink testLink(&sdk, &plugin, &channel, linkId, linkProperties, parser);

    EXPECT_EQ(testLink.getId(), linkId);
    EXPECT_EQ(testLink.getProperties(), linkProperties);
}

TEST(Link, test_getConnections) {
    MockRaceSdkComms sdk;
    MockPluginComms plugin(sdk);
    MockChannel channel(plugin);

    LinkID linkId = "some link id";
    LinkProfileParser parser;
    LinkProperties linkProperties;
    linkProperties.linkType = LT_RECV;
    linkProperties.transmissionType = TT_UNICAST;
    linkProperties.connectionType = CT_DIRECT;

    TestLink testLink(&sdk, &plugin, &channel, linkId, linkProperties, parser);
    testLink.setConnections({
        std::make_shared<Connection>("First", LT_SEND, nullptr, "", 0),
        std::make_shared<Connection>("2", LT_RECV, nullptr, "", 0),
        std::make_shared<Connection>("Then", LT_SEND, nullptr, "", 0),
    });

    auto connections = testLink.getConnections();
    EXPECT_EQ(connections[0]->connectionId, "First");
    EXPECT_EQ(connections[0]->linkType, LT_SEND);
    EXPECT_EQ(connections[1]->connectionId, "2");
    EXPECT_EQ(connections[1]->linkType, LT_RECV);
    EXPECT_EQ(connections[2]->connectionId, "Then");
    EXPECT_EQ(connections[2]->linkType, LT_SEND);
}
