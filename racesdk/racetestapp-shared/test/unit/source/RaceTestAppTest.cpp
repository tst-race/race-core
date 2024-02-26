
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

#include <IRaceSdkApp.h>

#include <stdexcept>  // std::invalid_argument

#include "../../common/MockRaceApp.h"
#include "../../common/MockRaceSdkApp.h"
#include "../../common/MockRaceTestAppOutput.h"
#include "OpenTracingHelpers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "racetestapp/RaceApp.h"
#include "racetestapp/RaceTestApp.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

TEST_F(RaceTestAppSharedTestFixture, parseAndPrepareToBootstrap_should_reject_bad_payload) {
    MockRaceTestAppOutput output;
    std::shared_ptr<opentracing::Tracer> tracer = createTracer("", "race-client-00001");

    EXPECT_CALL(output, writeOutput(::testing::StartsWith("ERROR:"))).Times(1);
    EXPECT_CALL(mockSdk, prepareToBootstrap(::testing::_, ::testing::_, ::testing::_)).Times(0);

    MockRaceApp mockRaceApp(output, mockSdk, createTracer("", "test persona"));
    RaceTestApp app(output, mockSdk, mockRaceApp, tracer);

    app.processRaceTestAppCommand(R"({
        "type": "prepare-to-bootstrap",
        "payload": {
            "platform": "linux",
            "architecture": "x86_64",
            "nodeType": "client"
        }
    })");
}

TEST_F(RaceTestAppSharedTestFixture, parseAndPrepareToBootstrap_should_invoke_sdk) {
    MockRaceTestAppOutput output;
    std::shared_ptr<opentracing::Tracer> tracer = createTracer("", "race-client-00001");

    EXPECT_CALL(mockSdk, prepareToBootstrap(
                             ::testing::AllOf(::testing::Field(&DeviceInfo::platform, "linux"),
                                              ::testing::Field(&DeviceInfo::architecture, "x86_64"),
                                              ::testing::Field(&DeviceInfo::nodeType, "client")),
                             "passphrase", "bootstrapChannel"))
        .Times(1);

    MockRaceApp mockRaceApp(output, mockSdk, createTracer("", "test persona"));
    RaceTestApp app(output, mockSdk, mockRaceApp, tracer);

    app.processRaceTestAppCommand(R"({
        "type": "prepare-to-bootstrap",
        "payload": {
            "platform": "linux",
            "architecture": "x86_64",
            "nodeType": "client",
            "passphrase": "passphrase",
            "bootstrapChannelId": "bootstrapChannel"
        }
    })");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RaceTestApp::sendMessage
////////////////////////////////////////////////////////////////////////////////////////////////////

class RaceTestAppTest : public RaceTestApp {
public:
    explicit RaceTestAppTest(IRaceTestAppOutput &_output, IRaceSdkTestApp &_sdk, RaceApp &_app) :
        RaceTestApp(_output, _sdk, _app, createTracer("", _sdk.getActivePersona())) {}

    void sendMessage(const rta::Message &message) {
        RaceTestApp::sendMessage(message);
    }

    void sendPeriodically(const std::vector<rta::Message> &message) {
        RaceTestApp::sendPeriodically({message});
    }
};

/**
 * @brief Custom matcher for googlemock to check that a ClrMsg is equivalent to an expected value.
 * Note that this does not compare time to the expected value since this is generated using the
 * current time and will change on every run. The match simply checks that time is greater than
 * zero.
 *
 */
class ClrMsgMatcher : public ::testing::MatcherInterface<ClrMsg> {
public:
    explicit ClrMsgMatcher(const ClrMsg &expected) : expectedClrMsg(expected) {}

    // cppcheck-suppress unusedFunction
    bool MatchAndExplain(ClrMsg msg, ::testing::MatchResultListener *) const override {
        return (msg.getMsg() == expectedClrMsg.getMsg() &&
                msg.getFrom() == expectedClrMsg.getFrom() &&
                msg.getTo() == expectedClrMsg.getTo() && msg.getTime() > 0 &&
                msg.getNonce() == expectedClrMsg.getNonce());
    }

    // cppcheck-suppress unusedFunction
    void DescribeTo(std::ostream *os) const override {
        *os << "is equivalent to the input ClrMsg";
    }

    // cppcheck-suppress unusedFunction
    void DescribeNegationTo(std::ostream *os) const override {
        *os << "is not equivalent to the input ClrMsg";
    }

private:
    ClrMsg expectedClrMsg;
};

static ::testing::Matcher<ClrMsg> IsEquivalentToClrMsg(const ClrMsg &expected) {
    return ::testing::MakeMatcher(new ClrMsgMatcher(expected));
}

TEST_F(RaceTestAppSharedTestFixture, will_send_message_to_sdk_core) {
    EXPECT_CALL(mockSdk, sendClientMessage(IsEquivalentToClrMsg(
                             ClrMsg("test-id hello someone", "my-persona", "to someone", 0, 10))))
        .Times(1);
    MockRaceTestAppOutput output;
    MockRaceApp mockRaceApp(output, mockSdk, createTracer("", "test persona"));
    RaceTestAppTest app(output, mockSdk, mockRaceApp);

    EXPECT_CALL(mockSdk, isConnected()).Times(1).WillOnce(::testing::Return(true));
    const auto message = rta::Message::createMessage(R"({
        "type": "send-message",
        "payload": {
            "send-type": "manual",
            "recipient": "to someone",
            "message": "hello someone",
            "test-id": "test-id",
            "network-manager-bypass-route": ""
        }
    })"_json);
    app.sendMessage(message[0]);
}

TEST_F(RaceTestAppSharedTestFixture, will_send_network_manager_bypass_message_to_sdk_core) {
    EXPECT_CALL(mockSdk,
                sendNMBypassMessage(IsEquivalentToClrMsg(
                                        ClrMsg("hello someone", "my-persona", "to someone", 0, 10)),
                                    "PluginId/ChannelId/LinkId"))
        .Times(1);
    MockRaceTestAppOutput output;
    MockRaceApp mockRaceApp(output, mockSdk, createTracer("", "test persona"));
    RaceTestAppTest app(output, mockSdk, mockRaceApp);

    const auto message = rta::Message::createMessage(R"({
        "type": "send-message",
        "payload": {
            "send-type": "manual",
            "recipient": "to someone",
            "message": "hello someone",
            "test-id": "",
            "network-manager-bypass-route": "PluginId/ChannelId/LinkId"
        }
    })"_json);
    app.sendMessage(message[0]);
}

ACTION(ThrowInvalidArgument) {
    throw std::invalid_argument("testing");
}

TEST_F(RaceTestAppSharedTestFixture, will_output_error_if_sdk_throws) {
    {
        EXPECT_CALL(mockSdk, sendClientMessage(::testing::_))
            .Times(1)
            .WillRepeatedly(ThrowInvalidArgument());
    }

    MockRaceTestAppOutput output;
    {
        EXPECT_CALL(output, writeOutput(::testing::_)).Times(::testing::AnyNumber());
        // Really just checking the above because it gets output. This is the real test.
        // EXPECT_CALLs that come later are matched first
        EXPECT_CALL(output,
                    writeOutput(::testing::HasSubstr("Exception thrown while sending a message: ")))
            .Times(1);
    }
    MockRaceApp mockRaceApp(output, mockSdk, createTracer("", "test persona"));
    RaceTestAppTest app(output, mockSdk, mockRaceApp);

    const auto message = rta::Message::createMessage(R"({
        "type": "send-message",
        "payload": {
            "send-type": "manual",
            "recipient": "to someone",
            "message": "hello someone",
            "test-id": "",
            "network-manager-bypass-route": ""
        }
    })"_json);
    app.sendMessage(message[0]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RaceTestApp::sendPeriodically
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(RaceTestAppSharedTestFixture, will_send_all_auto_message) {
    const auto messages = rta::Message::createMessage(R"({
        "type": "send-message",
        "payload": {
            "send-type": "auto",
            "recipient": "recipient",
            "quantity": 2,
            "period": 0,
            "size": 10,
            "test-id": "1234",
            "network-manager-bypass-route": ""
        }
    })"_json);
    // NOTE: this test depends on random characters NOT being generated,
    // so this must be true: size <= test-id.size() + 6

    EXPECT_CALL(mockSdk, sendClientMessage(IsEquivalentToClrMsg(
                             ClrMsg(messages[0].messageContent, "my-persona", "recipient", 0, 10))))
        .Times(1);
    EXPECT_CALL(mockSdk, sendClientMessage(IsEquivalentToClrMsg(
                             ClrMsg(messages[1].messageContent, "my-persona", "recipient", 0, 10))))
        .Times(1);
    MockRaceTestAppOutput output;
    MockRaceApp mockRaceApp(output, mockSdk, createTracer("", "test persona"));
    RaceTestAppTest app(output, mockSdk, mockRaceApp);

    EXPECT_CALL(mockSdk, isConnected()).WillRepeatedly(::testing::Return(true));
    app.sendPeriodically(messages);
}

TEST_F(RaceTestAppSharedTestFixture, will_send_auto_network_manager_bypass_message) {
    const auto messages = rta::Message::createMessage(R"({
        "type": "send-message",
        "payload": {
            "send-type": "auto",
            "recipient": "recipient",
            "quantity": 2,
            "period": 0,
            "size": 10,
            "test-id": "1234",
            "network-manager-bypass-route": "PluginId/ChannelId/LinkId/ConnId"
        }
    })"_json);
    // NOTE: this test depends on random characters NOT being generated,
    // so this must be true: size <= test-id.size() + 6

    EXPECT_CALL(mockSdk,
                sendNMBypassMessage(IsEquivalentToClrMsg(ClrMsg(messages[0].messageContent,
                                                                "my-persona", "recipient", 0, 10)),
                                    "PluginId/ChannelId/LinkId/ConnId"))
        .Times(1);
    EXPECT_CALL(mockSdk,
                sendNMBypassMessage(IsEquivalentToClrMsg(ClrMsg(messages[1].messageContent,
                                                                "my-persona", "recipient", 0, 10)),
                                    "PluginId/ChannelId/LinkId/ConnId"))
        .Times(1);
    MockRaceTestAppOutput output;
    MockRaceApp mockRaceApp(output, mockSdk, createTracer("", "test persona"));
    RaceTestAppTest app(output, mockSdk, mockRaceApp);

    app.sendPeriodically(messages);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RaceTestApp::parseAndExecuteRpcAction
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(RaceTestAppSharedTestFixture, parseAndExecuteRpcAction_should_reject_bad_payload) {
    MockRaceTestAppOutput output;
    std::shared_ptr<opentracing::Tracer> tracer = createTracer("", "race-client-00001");

    EXPECT_CALL(output, writeOutput(::testing::StartsWith("ERROR:"))).Times(5);

    MockRaceApp mockRaceApp(output, mockSdk, createTracer("", "test persona"));
    RaceTestApp app(output, mockSdk, mockRaceApp, tracer);

    app.processRaceTestAppCommand(R"({
        "type": "rpc"
    })");
    app.processRaceTestAppCommand(R"({
        "type": "rpc",
        "payload": {}
    })");
    app.processRaceTestAppCommand(R"({
        "type": "rpc",
        "payload": {"action": "deactivate-channel"}
    })");
    app.processRaceTestAppCommand(R"({
        "type": "rpc",
        "payload": {"action": "destroy-link"}
    })");
    app.processRaceTestAppCommand(R"({
        "type": "rpc",
        "payload": {"action": "close-connection"}
    })");
}

TEST_F(RaceTestAppSharedTestFixture,
       parseAndExecuteRpcAction_should_invoke_sdk_deactivate_channel) {
    MockRaceTestAppOutput output;
    std::shared_ptr<opentracing::Tracer> tracer = createTracer("", "race-client-00001");

    EXPECT_CALL(mockSdk, rpcDeactivateChannel("TestChannel")).Times(1);

    MockRaceApp mockRaceApp(output, mockSdk, createTracer("", "test persona"));
    RaceTestApp app(output, mockSdk, mockRaceApp, tracer);

    app.processRaceTestAppCommand(R"({
        "type": "rpc",
        "payload": {
            "action": "deactivate-channel",
            "channelGid": "TestChannel"
        }
    })");
}

TEST_F(RaceTestAppSharedTestFixture, parseAndExecuteRpcAction_should_invoke_sdk_destroy_link) {
    MockRaceTestAppOutput output;
    std::shared_ptr<opentracing::Tracer> tracer = createTracer("", "race-client-00001");

    EXPECT_CALL(mockSdk, rpcDestroyLink("TestLink")).Times(1);

    MockRaceApp mockRaceApp(output, mockSdk, createTracer("", "test persona"));
    RaceTestApp app(output, mockSdk, mockRaceApp, tracer);

    app.processRaceTestAppCommand(R"({
        "type": "rpc",
        "payload": {
            "action": "destroy-link",
            "linkId": "TestLink"
        }
    })");
}

TEST_F(RaceTestAppSharedTestFixture, parseAndExecuteRpcAction_should_invoke_sdk_close_connection) {
    MockRaceTestAppOutput output;
    std::shared_ptr<opentracing::Tracer> tracer = createTracer("", "race-client-00001");

    EXPECT_CALL(mockSdk, rpcCloseConnection("TestConnection")).Times(1);

    MockRaceApp mockRaceApp(output, mockSdk, createTracer("", "test persona"));
    RaceTestApp app(output, mockSdk, mockRaceApp, tracer);

    app.processRaceTestAppCommand(R"({
        "type": "rpc",
        "payload": {
            "action": "close-connection",
            "connectionId": "TestConnection"
        }
    })");
}
