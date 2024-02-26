
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

#include "PluginNMTestHarness.h"
#include "gtest/gtest.h"
#include "race/mocks/MockRaceSdkNM.h"

class PluginNMTestHarnessProtectedAccess : public PluginNMTestHarness {
public:
    PluginNMTestHarnessProtectedAccess(IRaceSdkNM *sdk) : PluginNMTestHarness(sdk) {}
};

class PluginNMTestHarnessTest : public ::testing::Test {
public:
    PluginNMTestHarnessTest() : plugin(&mockRaceSdk) {
        // Set up mock output for getActivePersona()
        ON_CALL(mockRaceSdk, getActivePersona())
            .WillByDefault(::testing::Return("to some recipient"));
    }

protected:
    void SetUp() override {
        plugin.init({});
    }

    void TearDown() override {
        plugin.shutdown();
    }

public:
    MockRaceSdkNM mockRaceSdk;
    PluginNMTestHarnessProtectedAccess plugin;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PluginNMTestHarness::start
////////////////////////////////////////////////////////////////////////////////////////////////////

class PluginNMTestHarnessConfigFileTest : public PluginNMTestHarnessTest {
protected:
    void SetUp() override {
        // TODO: this test depends on file I/O, something unit tests should not do. May want to
        // refactor this test at some point.

        PluginConfig pluginConfig;

        plugin.init(pluginConfig);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PluginNMTestHarness::presentCleartextMessage
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(PluginNMTestHarnessTest, processEncPkg_should_call_sdk_for_valid_cipher_text) {
    // clang-format off
    const std::string validCipherText =
        "000000c"               // header
        "some message"          // message
        "0000010"               // header
        "from some sender"      // from
        "0000011"               // header
        "to some recipient"     // to
        "0000013"               // header
        "9223372036854775807"   // time
        "000000a"               // header
        "2147483647";           // nonce
    // clang-format on
    const EncPkg package(0, 0, {validCipherText.begin(), validCipherText.end()});

    const ClrMsg expectedClrMsg("some message", "from some sender", "to some recipient",
                                9223372036854775807, 2147483647);

    EXPECT_CALL(mockRaceSdk, presentCleartextMessage(expectedClrMsg)).Times(1);

    plugin.processEncPkg(0, package, {});
}

TEST_F(PluginNMTestHarnessTest, processEncPkg_should_not_call_sdk_for_valid_cipher_text_wrong_to) {
    // clang-format off
    const std::string validCipherText =
        "000000c"               // header
        "some message"          // message
        "0000010"               // header
        "from some sender"      // from
        "0000011"               // header
        "to different recipient"     // to
        "0000013"               // header
        "9223372036854775807"   // time
        "000000a"               // header
        "2147483647";           // nonce
    // clang-format on
    const EncPkg package(0, 0, {validCipherText.begin(), validCipherText.end()});

    EXPECT_CALL(mockRaceSdk, presentCleartextMessage(::testing::_)).Times(0);

    plugin.processEncPkg(0, package, {});
}

TEST_F(PluginNMTestHarnessTest, processEncPkg_should_not_call_sdk_for_invalid_cipher_text) {
    const std::string invalidCipherText = "some invlid cipher text";
    const EncPkg package({invalidCipherText.begin(), invalidCipherText.end()});

    EXPECT_CALL(mockRaceSdk, presentCleartextMessage(::testing::_)).Times(0);

    plugin.processEncPkg(0, package, {});
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PluginNMTestHarness::processNMBypassMsg
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(PluginNMTestHarnessTest, processNMBypassMsg_should_return_error_on_invalid_route) {
    ClrMsg clrMsg("plain text", "race-client-00001", "race-client-00002", 1234, 10, 0, 7777, 4242);
    ASSERT_EQ(PLUGIN_ERROR, plugin.processNMBypassMsg(0, "", clrMsg));
    ASSERT_EQ(PLUGIN_ERROR, plugin.processNMBypassMsg(0, "plugin/channel/link/conn/other", clrMsg));
}

TEST_F(PluginNMTestHarnessTest, processNMBypassMsg_should_send_on_specific_connection_id) {
    EXPECT_CALL(mockRaceSdk,
                sendEncryptedPackage(::testing::_,
                                     "TwoSixPlugin/twoSixChannel/Link_7/Connection_14", 0, 0));

    ClrMsg clrMsg("plain text", "race-client-00001", "race-client-00002", 1234, 10, 0, 7777, 4242);
    ASSERT_EQ(PLUGIN_OK, plugin.processNMBypassMsg(
                             0, "TwoSixPlugin/twoSixChannel/Link_7/Connection_14", clrMsg));
}

TEST_F(PluginNMTestHarnessTest, processNMBypassMsg_should_open_and_send_on_specified_link) {
    ::testing::InSequence sequence;
    EXPECT_CALL(mockRaceSdk, openConnection(LT_SEND, "TwoSixPlugin/twoSixChannel/Link_7",
                                            ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(SdkResponse(SDK_OK, 0, 123456)));
    EXPECT_CALL(mockRaceSdk,
                sendEncryptedPackage(::testing::_,
                                     "TwoSixPlugin/twoSixChannel/Link_7/Connection_42", 0, 0));
    EXPECT_CALL(mockRaceSdk,
                closeConnection("TwoSixPlugin/twoSixChannel/Link_7/Connection_42", ::testing::_));

    ClrMsg clrMsg("plain text", "race-client-00001", "race-client-00002", 1234, 10, 0, 7777, 4242);
    ASSERT_EQ(PLUGIN_OK, plugin.processNMBypassMsg(0, "TwoSixPlugin/twoSixChannel/Link_7", clrMsg));
    ASSERT_EQ(PLUGIN_OK,
              plugin.onConnectionStatusChanged(
                  123456, "TwoSixPlugin/twoSixChannel/Link_7/Connection_42", CONNECTION_OPEN,
                  "TwoSixPlugin/twoSixChannel/Link_7", LinkProperties()));
}

TEST_F(PluginNMTestHarnessTest,
       processNMBypassMsg_should_open_and_send_on_specified_plugin_channel) {
    ::testing::InSequence sequence;
    EXPECT_CALL(mockRaceSdk,
                getLinksForPersonas(std::vector<std::string>{"race-client-00002"}, LT_SEND))
        .WillOnce(::testing::Return(std::vector<std::string>{
            "TwoSixPlugin/twoSixOtherChannel/Link_16", "TwoSixPlugin/twoSixChannel/Link_11"}));

    LinkProperties otherLinkProps;
    otherLinkProps.channelGid = "twoSixOtherChannel";
    EXPECT_CALL(mockRaceSdk, getLinkProperties("TwoSixPlugin/twoSixOtherChannel/Link_16"))
        .WillOnce(::testing::Return(otherLinkProps));

    LinkProperties linkProps;
    linkProps.channelGid = "twoSixChannel";
    EXPECT_CALL(mockRaceSdk, getLinkProperties("TwoSixPlugin/twoSixChannel/Link_11"))
        .WillOnce(::testing::Return(linkProps));

    EXPECT_CALL(mockRaceSdk, openConnection(LT_SEND, "TwoSixPlugin/twoSixChannel/Link_11",
                                            ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(SdkResponse(SDK_OK, 0, 2468)));
    EXPECT_CALL(mockRaceSdk,
                sendEncryptedPackage(::testing::_,
                                     "TwoSixPlugin/twoSixChannel/Link_11/Connection_17", 0, 0));
    EXPECT_CALL(mockRaceSdk,
                closeConnection("TwoSixPlugin/twoSixChannel/Link_11/Connection_17", ::testing::_));

    ClrMsg clrMsg("plain text", "race-client-00001", "race-client-00002", 1234, 10, 0, 7777, 4242);
    ASSERT_EQ(PLUGIN_OK, plugin.processNMBypassMsg(0, "TwoSixPlugin/twoSixChannel", clrMsg));
    ASSERT_EQ(PLUGIN_OK,
              plugin.onConnectionStatusChanged(
                  2468, "TwoSixPlugin/twoSixChannel/Link_11/Connection_17", CONNECTION_OPEN,
                  "TwoSixPlugin/twoSixChannel/Link_11", LinkProperties()));
}

TEST_F(PluginNMTestHarnessTest, processNMBypassMsg_should_open_and_send_on_specified_channel) {
    ::testing::InSequence sequence;
    EXPECT_CALL(mockRaceSdk,
                getLinksForPersonas(std::vector<std::string>{"race-client-00002"}, LT_SEND))
        .WillOnce(::testing::Return(std::vector<std::string>{
            "TwoSixPlugin/twoSixOtherChannel/Link_16", "TwoSixPlugin/twoSixChannel/Link_11"}));

    LinkProperties otherLinkProps;
    otherLinkProps.channelGid = "twoSixOtherChannel";
    EXPECT_CALL(mockRaceSdk, getLinkProperties("TwoSixPlugin/twoSixOtherChannel/Link_16"))
        .WillOnce(::testing::Return(otherLinkProps));

    LinkProperties linkProps;
    linkProps.channelGid = "twoSixChannel";
    EXPECT_CALL(mockRaceSdk, getLinkProperties("TwoSixPlugin/twoSixChannel/Link_11"))
        .WillOnce(::testing::Return(linkProps));

    EXPECT_CALL(mockRaceSdk, openConnection(LT_SEND, "TwoSixPlugin/twoSixChannel/Link_11",
                                            ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(SdkResponse(SDK_OK, 0, 2468)));
    EXPECT_CALL(mockRaceSdk,
                sendEncryptedPackage(::testing::_,
                                     "TwoSixPlugin/twoSixChannel/Link_11/Connection_17", 0, 0));
    EXPECT_CALL(mockRaceSdk,
                closeConnection("TwoSixPlugin/twoSixChannel/Link_11/Connection_17", ::testing::_));

    ClrMsg clrMsg("plain text", "race-client-00001", "race-client-00002", 1234, 10, 0, 7777, 4242);
    ASSERT_EQ(PLUGIN_OK, plugin.processNMBypassMsg(0, "twoSixChannel", clrMsg));
    ASSERT_EQ(PLUGIN_OK,
              plugin.onConnectionStatusChanged(
                  2468, "TwoSixPlugin/twoSixChannel/Link_11/Connection_17", CONNECTION_OPEN,
                  "TwoSixPlugin/twoSixChannel/Link_11", LinkProperties()));
}

TEST_F(PluginNMTestHarnessTest, processNMBypassMsg_should_open_and_send_on_wildcard_channel) {
    ::testing::InSequence sequence;
    EXPECT_CALL(mockRaceSdk,
                getLinksForPersonas(std::vector<std::string>{"race-client-00002"}, LT_SEND))
        .WillOnce(::testing::Return(std::vector<std::string>{
            "TwoSixPlugin/twoSixOtherChannel/Link_16", "TwoSixPlugin/twoSixChannel/Link_11"}));

    LinkProperties otherLinkProps;
    otherLinkProps.channelGid = "twoSixOtherChannel";
    EXPECT_CALL(mockRaceSdk, getLinkProperties("TwoSixPlugin/twoSixOtherChannel/Link_16"))
        .WillOnce(::testing::Return(otherLinkProps));

    EXPECT_CALL(mockRaceSdk, openConnection(LT_SEND, "TwoSixPlugin/twoSixOtherChannel/Link_16",
                                            ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(SdkResponse(SDK_OK, 0, 1357)));
    EXPECT_CALL(mockRaceSdk,
                sendEncryptedPackage(
                    ::testing::_, "TwoSixPlugin/twoSixOtherChannel/Link_16/Connection_24", 0, 0));
    EXPECT_CALL(
        mockRaceSdk,
        closeConnection("TwoSixPlugin/twoSixOtherChannel/Link_16/Connection_24", ::testing::_));

    ClrMsg clrMsg("plain text", "race-client-00001", "race-client-00002", 1234, 10, 0, 7777, 4242);
    ASSERT_EQ(PLUGIN_OK, plugin.processNMBypassMsg(0, "*", clrMsg));
    ASSERT_EQ(PLUGIN_OK,
              plugin.onConnectionStatusChanged(
                  1357, "TwoSixPlugin/twoSixOtherChannel/Link_16/Connection_24", CONNECTION_OPEN,
                  "TwoSixPlugin/twoSixOtherChannel/Link_16", LinkProperties()));
}

TEST_F(PluginNMTestHarnessTest, processNMBypassMsg_should_return_error_when_link_not_found) {
    ::testing::InSequence sequence;
    EXPECT_CALL(mockRaceSdk,
                getLinksForPersonas(std::vector<std::string>{"race-client-00002"}, LT_SEND))
        .WillOnce(
            ::testing::Return(std::vector<std::string>{"TwoSixPlugin/twoSixOtherChannel/Link_16"}));

    LinkProperties otherLinkProps;
    otherLinkProps.channelGid = "twoSixOtherChannel";
    EXPECT_CALL(mockRaceSdk, getLinkProperties("TwoSixPlugin/twoSixOtherChannel/Link_16"))
        .WillOnce(::testing::Return(otherLinkProps));

    ClrMsg clrMsg("plain text", "race-client-00001", "race-client-00002", 1234, 10, 0, 7777, 4242);
    ASSERT_EQ(PLUGIN_ERROR, plugin.processNMBypassMsg(0, "twoSixChannel", clrMsg));
}

TEST_F(PluginNMTestHarnessTest,
       processNMBypassMsg_should_retry_opening_link_if_closed_before_opening) {
    ::testing::InSequence sequence;
    EXPECT_CALL(mockRaceSdk, openConnection(LT_SEND, "TwoSixPlugin/twoSixChannel/Link_7",
                                            ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(SdkResponse(SDK_OK, 0, 123456)))
        .WillOnce(::testing::Return(SdkResponse(SDK_OK, 0, 242424)));
    EXPECT_CALL(mockRaceSdk,
                sendEncryptedPackage(::testing::_,
                                     "TwoSixPlugin/twoSixChannel/Link_7/Connection_42", 0, 0));
    EXPECT_CALL(mockRaceSdk,
                closeConnection("TwoSixPlugin/twoSixChannel/Link_7/Connection_42", ::testing::_));

    ClrMsg clrMsg("plain text", "race-client-00001", "race-client-00002", 1234, 10, 0, 7777, 4242);
    ASSERT_EQ(PLUGIN_OK, plugin.processNMBypassMsg(0, "TwoSixPlugin/twoSixChannel/Link_7", clrMsg));
    ASSERT_EQ(PLUGIN_OK,
              plugin.onConnectionStatusChanged(
                  123456, "TwoSixPlugin/twoSixChannel/Link_7/Connection_41", CONNECTION_CLOSED,
                  "TwoSixPlugin/twoSixChannel/Link_7", LinkProperties()));
    ASSERT_EQ(PLUGIN_OK,
              plugin.onConnectionStatusChanged(
                  242424, "TwoSixPlugin/twoSixChannel/Link_7/Connection_42", CONNECTION_OPEN,
                  "TwoSixPlugin/twoSixChannel/Link_7", LinkProperties()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PluginNMTestHarness::openRecvConnection
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(PluginNMTestHarnessTest, openRecvConnection_should_open_specified_link) {
    ::testing::InSequence sequence;
    EXPECT_CALL(mockRaceSdk, openConnection(LT_RECV, "TwoSixPlugin/twoSixChannel/Link_7",
                                            ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(SdkResponse(SDK_OK, 0, 123456)));
    EXPECT_CALL(mockRaceSdk,
                closeConnection("TwoSixPlugin/twoSixChannel/Link_7/Connection_42", ::testing::_));

    const ClrMsg expectedClrMsg("some message", "from some sender", "to some recipient",
                                9223372036854775807, 2147483647);
    EXPECT_CALL(mockRaceSdk, presentCleartextMessage(expectedClrMsg));

    // clang-format off
    const std::string validCipherText =
        "000000c"               // header
        "some message"          // message
        "0000010"               // header
        "from some sender"      // from
        "0000011"               // header
        "to some recipient"     // to
        "0000013"               // header
        "9223372036854775807"   // time
        "000000a"               // header
        "2147483647";           // nonce
    // clang-format on
    const EncPkg package(0, 0, {validCipherText.begin(), validCipherText.end()});

    EXPECT_EQ(PLUGIN_OK, plugin.openRecvConnection(0, "race-client-00002",
                                                   "TwoSixPlugin/twoSixChannel/Link_7"));
    ASSERT_EQ(PLUGIN_OK,
              plugin.onConnectionStatusChanged(
                  123456, "TwoSixPlugin/twoSixChannel/Link_7/Connection_42", CONNECTION_OPEN,
                  "TwoSixPlugin/twoSixChannel/Link_7", LinkProperties()));
    ASSERT_EQ(PLUGIN_OK, plugin.processEncPkg(0, package,
                                              {"TwoSixPlugin/twoSixChannel/Link_7/Connection_42"}));
}

TEST_F(PluginNMTestHarnessTest, openRecvConnection_should_open_specified_channel) {
    ::testing::InSequence sequence;
    EXPECT_CALL(mockRaceSdk,
                getLinksForPersonas(std::vector<std::string>{"race-client-00002"}, LT_RECV))
        .WillOnce(
            ::testing::Return(std::vector<std::string>{"TwoSixPlugin/twoSixChannel/Link_11"}));

    LinkProperties linkProps;
    linkProps.channelGid = "twoSixChannel";
    EXPECT_CALL(mockRaceSdk, getLinkProperties("TwoSixPlugin/twoSixChannel/Link_11"))
        .WillOnce(::testing::Return(linkProps));

    EXPECT_CALL(mockRaceSdk, openConnection(LT_RECV, "TwoSixPlugin/twoSixChannel/Link_11",
                                            ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(SdkResponse(SDK_OK, 0, 242424)));
    EXPECT_CALL(mockRaceSdk,
                closeConnection("TwoSixPlugin/twoSixChannel/Link_11/Connection_17", ::testing::_));

    const ClrMsg expectedClrMsg("some message", "from some sender", "to some recipient",
                                9223372036854775807, 2147483647);
    EXPECT_CALL(mockRaceSdk, presentCleartextMessage(expectedClrMsg));

    // clang-format off
    const std::string validCipherText =
        "000000c"               // header
        "some message"          // message
        "0000010"               // header
        "from some sender"      // from
        "0000011"               // header
        "to some recipient"     // to
        "0000013"               // header
        "9223372036854775807"   // time
        "000000a"               // header
        "2147483647";           // nonce
    // clang-format on
    const EncPkg package(0, 0, {validCipherText.begin(), validCipherText.end()});

    EXPECT_EQ(PLUGIN_OK, plugin.openRecvConnection(0, "race-client-00002", "twoSixChannel"));
    ASSERT_EQ(PLUGIN_OK,
              plugin.onConnectionStatusChanged(
                  242424, "TwoSixPlugin/twoSixChannel/Link_11/Connection_17", CONNECTION_OPEN,
                  "TwoSixPlugin/twoSixChannel/Link_11", LinkProperties()));
    ASSERT_EQ(PLUGIN_OK, plugin.processEncPkg(
                             0, package, {"TwoSixPlugin/twoSixChannel/Link_11/Connection_17"}));
}

TEST_F(PluginNMTestHarnessTest, openRecvConnection_should_return_error_when_link_not_found) {
    ::testing::InSequence sequence;
    EXPECT_CALL(mockRaceSdk,
                getLinksForPersonas(std::vector<std::string>{"race-client-00002"}, LT_RECV))
        .WillOnce(
            ::testing::Return(std::vector<std::string>{"TwoSixPlugin/twoSixOtherChannel/Link_16"}));

    LinkProperties otherLinkProps;
    otherLinkProps.channelGid = "twoSixOtherChannel";
    EXPECT_CALL(mockRaceSdk, getLinkProperties("TwoSixPlugin/twoSixOtherChannel/Link_16"))
        .WillOnce(::testing::Return(otherLinkProps));

    EXPECT_EQ(PLUGIN_ERROR, plugin.openRecvConnection(0, "race-client-00002", "twoSixChannel"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PluginNMTestHarness::rpcDeactivateChannel
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(PluginNMTestHarnessTest, rpcDeactivateChannel_should_return_error_when_sdk_responds_not_ok) {
    EXPECT_CALL(mockRaceSdk, deactivateChannel("testChannel", RACE_BLOCKING))
        .WillOnce(::testing::Return(SDK_INVALID_ARGUMENT));
    EXPECT_EQ(PLUGIN_ERROR, plugin.rpcDeactivateChannel("testChannel"));
}

TEST_F(PluginNMTestHarnessTest, rpcDeactivateChannel_should_return_ok_when_sdk_responds_ok) {
    EXPECT_CALL(mockRaceSdk, deactivateChannel("testChannel", RACE_BLOCKING))
        .WillOnce(::testing::Return(SDK_OK));
    EXPECT_EQ(PLUGIN_OK, plugin.rpcDeactivateChannel("testChannel"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PluginNMTestHarness::rpcDestroyLink
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(PluginNMTestHarnessTest, rpcDestroyLink_should_return_error_when_link_id_is_invalid) {
    EXPECT_EQ(PLUGIN_ERROR, plugin.rpcDestroyLink("testChannel"));
    EXPECT_EQ(PLUGIN_ERROR, plugin.rpcDestroyLink("testChannel/testLink"));
    EXPECT_EQ(PLUGIN_ERROR, plugin.rpcDestroyLink("testPlugin/testChannel/testLink/testConn"));
}

TEST_F(PluginNMTestHarnessTest,
       rpcDestroyLink_single_should_return_error_when_sdk_responds_not_ok) {
    EXPECT_CALL(mockRaceSdk, destroyLink("testPlugin/testChannel/testLink", RACE_BLOCKING))
        .WillOnce(::testing::Return(SDK_INVALID_ARGUMENT));
    EXPECT_EQ(PLUGIN_ERROR, plugin.rpcDestroyLink("testPlugin/testChannel/testLink"));
}

TEST_F(PluginNMTestHarnessTest, rpcDestroyLink_single_should_return_ok_when_sdk_responds_ok) {
    EXPECT_CALL(mockRaceSdk, destroyLink("testPlugin/testChannel/testLink", RACE_BLOCKING))
        .WillOnce(::testing::Return(SDK_OK));
    EXPECT_EQ(PLUGIN_OK, plugin.rpcDestroyLink("testPlugin/testChannel/testLink"));
}

TEST_F(PluginNMTestHarnessTest, rpcDestroyLink_wildcard_when_one_has_failure) {
    EXPECT_CALL(mockRaceSdk, getLinksForChannel("testChannel"))
        .WillOnce(::testing::Return(std::vector<LinkID>{"testPlugin/testChannel/testLink1",
                                                        "testPlugin/testChannel/testLink2",
                                                        "testPlugin/testChannel/testLink3"}));
    EXPECT_CALL(mockRaceSdk, destroyLink("testPlugin/testChannel/testLink1", RACE_BLOCKING))
        .WillOnce(::testing::Return(SDK_OK));
    EXPECT_CALL(mockRaceSdk, destroyLink("testPlugin/testChannel/testLink2", RACE_BLOCKING))
        .WillOnce(::testing::Return(SDK_INVALID_ARGUMENT));
    EXPECT_CALL(mockRaceSdk, destroyLink("testPlugin/testChannel/testLink3", RACE_BLOCKING))
        .WillOnce(::testing::Return(SDK_OK));
    EXPECT_EQ(PLUGIN_OK, plugin.rpcDestroyLink("testPlugin/testChannel/*"));
}

TEST_F(PluginNMTestHarnessTest, rpcDestroyLink_wildcard_when_all_succeed) {
    EXPECT_CALL(mockRaceSdk, getLinksForChannel("testChannel"))
        .WillOnce(::testing::Return(std::vector<LinkID>{"testPlugin/testChannel/testLink1",
                                                        "testPlugin/testChannel/testLink2",
                                                        "testPlugin/testChannel/testLink3"}));
    EXPECT_CALL(mockRaceSdk, destroyLink("testPlugin/testChannel/testLink1", RACE_BLOCKING))
        .WillOnce(::testing::Return(SDK_OK));
    EXPECT_CALL(mockRaceSdk, destroyLink("testPlugin/testChannel/testLink2", RACE_BLOCKING))
        .WillOnce(::testing::Return(SDK_OK));
    EXPECT_CALL(mockRaceSdk, destroyLink("testPlugin/testChannel/testLink3", RACE_BLOCKING))
        .WillOnce(::testing::Return(SDK_OK));
    EXPECT_EQ(PLUGIN_OK, plugin.rpcDestroyLink("testChannel/*"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PluginNMTestHarness::rpcCloseConnection
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(PluginNMTestHarnessTest, rpcCloseConnection_should_return_error_when_conn_id_is_invalid) {
    EXPECT_EQ(PLUGIN_ERROR, plugin.rpcCloseConnection("testChannel"));
    EXPECT_EQ(PLUGIN_ERROR, plugin.rpcCloseConnection("testPlugin/testChannel"));
    EXPECT_EQ(PLUGIN_ERROR, plugin.rpcCloseConnection("testPlugin/testChannel/testLink"));
    EXPECT_EQ(PLUGIN_ERROR,
              plugin.rpcCloseConnection("testPlugin/testChannel/testLink/testConnection/extra"));
}

TEST_F(PluginNMTestHarnessTest,
       rpcCloseConnection_single_should_return_error_when_sdk_responds_not_ok) {
    EXPECT_CALL(mockRaceSdk,
                closeConnection("testPlugin/testChannel/testLink/testConn", RACE_BLOCKING))
        .WillOnce(::testing::Return(SDK_INVALID_ARGUMENT));
    EXPECT_EQ(PLUGIN_ERROR, plugin.rpcCloseConnection("testPlugin/testChannel/testLink/testConn"));
}

TEST_F(PluginNMTestHarnessTest, rpcCloseConnection_single_should_return_ok_when_sdk_responds_ok) {
    EXPECT_CALL(mockRaceSdk,
                closeConnection("testPlugin/testChannel/testLink/testConn", RACE_BLOCKING))
        .WillOnce(::testing::Return(SDK_OK));
    EXPECT_EQ(PLUGIN_OK, plugin.rpcCloseConnection("testPlugin/testChannel/testLink/testConn"));
}
