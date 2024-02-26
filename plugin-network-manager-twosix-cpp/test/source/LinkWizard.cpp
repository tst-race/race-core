
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

#include "LinkWizard.h"

#include "ExtClrMsg.h"
#include "Log.h"
#include "MockPluginNM.h"
#include "RaceCrypto.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "race/mocks/MockRaceSdkNM.h"

using ::testing::_;

class TestPlugin : public MockPluginNM {
public:
    explicit TestPlugin(IRaceSdkNM &raceSdkIn) : MockPluginNM(raceSdkIn) {
        ON_CALL(*this, sendFormattedMsg(_, _, _, _))
            .WillByDefault([this](const std::string &dstUuid, const std::string &msgString,
                                  const std::uint64_t /*traceId*/, const std::uint64_t /*spanId*/) {
                sendQueues[dstUuid].push_back(msgString);
                return 1;
            });
    };

    ExtClrMsg popMsg(const std::string &uuid) {
        std::string msg = "";
        if (sendQueues.count(uuid) > 0 and sendQueues[uuid].size() > 0) {
            msg = sendQueues[uuid].at(0);
            sendQueues[uuid].erase(sendQueues[uuid].begin());
        }
        return messageParser.parseDelimitedExtMessage(msg);
    }

    std::unordered_map<std::string, std::vector<std::string>> sendQueues;
    RaceCrypto messageParser;
};

struct Node {
    std::string uuid;
    Persona persona;
    std::unique_ptr<MockRaceSdkNM> sdk;
    std::unique_ptr<TestPlugin> plugin;
    std::unique_ptr<LinkWizard> wizard;
    std::map<std::string, ChannelProperties> supportedChannels;
};

template <size_t S, size_t C>
class LinkWizardTestFixture : public ::testing::Test {
public:
    LinkWizardTestFixture() {
        for (size_t i = 0; i < S; ++i) {
            nodes[i].uuid = "race-server-" + std::to_string(i);
        }

        for (size_t i = 0; i < C; ++i) {
            nodes[C + i].uuid = "race-client-" + std::to_string(i);
        }

        for (auto &node : nodes) {
            node.persona.setDisplayName(node.uuid);
            node.persona.setRaceUuid(node.uuid);
            if (node.uuid.find("client") != std::string::npos) {
                node.persona.setPersonaType(P_CLIENT);
            } else {
                node.persona.setPersonaType(P_SERVER);
            }

            node.sdk = std::make_unique<MockRaceSdkNM>();
            ON_CALL(*node.sdk, getActivePersona()).WillByDefault(::testing::Return(node.uuid));
            ON_CALL(*node.sdk, getSupportedChannels()).WillByDefault(::testing::Invoke([&node]() {
                return node.supportedChannels;
            }));
            ON_CALL(*node.sdk, getChannelProperties(::testing::_))
                .WillByDefault(::testing::Invoke([&node](std::string channelGid) {
                    return node.supportedChannels[channelGid];
                }));
            node.plugin = std::make_unique<TestPlugin>(*node.sdk);
            node.wizard = std::make_unique<LinkWizard>(node.uuid, node.persona.getPersonaType(),
                                                       node.plugin.get());
            node.wizard->init();
            node.wizard->setReadyToRespond(true);
        }
    }
    virtual ~LinkWizardTestFixture() {}
    virtual void SetUp() override {}
    virtual void TearDown() override {}

    void expectSupportedChannelQuery(Node &queryNode, Node &respondingNode) {
        // Start wizard0's querying
        EXPECT_TRUE(queryNode.wizard->addPersona(respondingNode.persona));
        EXPECT_EQ(queryNode.plugin->sendQueues[respondingNode.uuid].size(), 1);
        // Process the query from wizard0
        EXPECT_TRUE(respondingNode.wizard->processLinkMsg(
            queryNode.persona, queryNode.plugin->popMsg(respondingNode.uuid)));
        EXPECT_THAT(respondingNode.plugin->sendQueues[queryNode.uuid].at(0),
                    testing::HasSubstr("\"supportedChannels\""));

        // Process the response
        EXPECT_TRUE(queryNode.wizard->processLinkMsg(
            respondingNode.persona, respondingNode.plugin->popMsg(queryNode.uuid)));
    }

    void expectTryObtainUnicastLinkCreatesLink(Node &originNode, Node &destNode,
                                               const std::string &expectedChannelGid,
                                               LinkType linkType) {
        // call tryObtainUnicastLink on wizard0
        RaceHandle createHandle = 42;
        SdkResponse createResp(SDK_OK, 0.0, createHandle);
        EXPECT_CALL(originNode.plugin->mockLinkManager,
                    createLink(expectedChannelGid, std::vector<std::string>{destNode.uuid}))
            .WillOnce(::testing::Return(createResp));
        EXPECT_TRUE(originNode.wizard->tryObtainUnicastLink(destNode.persona, linkType,
                                                            expectedChannelGid, LS_BOTH));

        LinkProperties linkProps;
        linkProps.linkAddress = originNode.uuid;
        linkProps.channelGid = expectedChannelGid;
        originNode.wizard->handleLinkStatusUpdate(createHandle, "linkId", LINK_CREATED, linkProps);
        EXPECT_THAT(originNode.plugin->sendQueues[destNode.uuid].at(0),
                    testing::HasSubstr("\"requestLoadLinkAddress\""));

        // wizard1 process the requestLoadLinkAddress from wizard0
        SdkResponse loadResp(SDK_OK, 0.0, 42);
        EXPECT_CALL(destNode.plugin->mockLinkManager,
                    loadLinkAddress(expectedChannelGid, originNode.uuid,
                                    std::vector<std::string>{originNode.uuid}))
            .WillOnce(::testing::Return(loadResp));
        EXPECT_TRUE(destNode.wizard->processLinkMsg(originNode.persona,
                                                    originNode.plugin->popMsg(destNode.uuid)));
    }

    void expectTryObtainUnicastLinkLoadsLink(Node &originNode, Node &destNode,
                                             const std::string &expectedChannelGid,
                                             LinkType linkType) {
        // call tryObtainUnicastLink on wizard0
        EXPECT_TRUE(originNode.wizard->tryObtainUnicastLink(destNode.persona, linkType,
                                                            expectedChannelGid, LS_BOTH));
        EXPECT_THAT(originNode.plugin->sendQueues[destNode.uuid].at(0),
                    testing::HasSubstr("\"requestCreateUnicastLink\""));

        // wizard1 process the requestCreateUnicastLink from wizard0
        RaceHandle createHandle = 42;
        SdkResponse createResp(SDK_OK, 0.0, createHandle);
        EXPECT_CALL(destNode.plugin->mockLinkManager,
                    createLink(expectedChannelGid, std::vector<std::string>{originNode.uuid}))
            .WillOnce(::testing::Return(createResp));
        EXPECT_TRUE(destNode.wizard->processLinkMsg(originNode.persona,
                                                    originNode.plugin->popMsg(destNode.uuid)));

        LinkProperties linkProps;
        linkProps.linkAddress = destNode.uuid;
        linkProps.channelGid = expectedChannelGid;
        destNode.wizard->handleLinkStatusUpdate(createHandle, "linkId", LINK_CREATED, linkProps);
        EXPECT_THAT(destNode.plugin->sendQueues[originNode.uuid].at(0),
                    testing::HasSubstr("\"requestLoadLinkAddress\""));

        // wizard0 process the requestLoadLinkAddress from wizard0
        SdkResponse loadResp(SDK_OK, 0.0, 42);
        EXPECT_CALL(originNode.plugin->mockLinkManager,
                    loadLinkAddress(expectedChannelGid, destNode.uuid,
                                    std::vector<std::string>{destNode.uuid}))
            .WillOnce(::testing::Return(loadResp));
        EXPECT_TRUE(originNode.wizard->processLinkMsg(destNode.persona,
                                                      destNode.plugin->popMsg(originNode.uuid)));
    }

public:
    std::array<Node, S + C> nodes;
};

using LinkWizardTestFixture2x0 = LinkWizardTestFixture<2, 0>;
using LinkWizardTestFixture3x0 = LinkWizardTestFixture<3, 0>;
using LinkWizardTestFixture1x1 = LinkWizardTestFixture<1, 1>;

TEST_F(LinkWizardTestFixture2x0, test_addPersona_queries) {
    // Including BLANK so our numerical indices match the names
    EXPECT_TRUE(nodes[0].wizard->addPersona(nodes[1].persona));
    EXPECT_EQ(nodes[0].plugin->sendQueues[nodes[1].uuid].size(), 1);
    EXPECT_THAT(nodes[0].plugin->sendQueues[nodes[1].uuid].at(0),
                testing::HasSubstr("{\"getSupportedChannels\": true}"));
}

TEST_F(LinkWizardTestFixture2x0, test_addPersona_query_response) {
    ChannelProperties props;
    nodes[1].supportedChannels = {{"channel1", props}};
    expectSupportedChannelQuery(nodes[0], nodes[1]);
}

TEST_F(LinkWizardTestFixture2x0, test_tryObtainUnicastLink_bidi) {
    // Setup the channel we will be using
    std::string channelGid = "uni-bidi";
    ChannelProperties props;
    props.transmissionType = TT_UNICAST;
    props.linkDirection = LD_BIDI;
    props.connectionType = CT_INDIRECT;
    props.maxLinks = 10;
    props.currentRole.linkSide = LS_BOTH;

    nodes[0].supportedChannels = {{channelGid, props}};
    nodes[1].supportedChannels = {{channelGid, props}};

    expectSupportedChannelQuery(nodes[0], nodes[1]);
    expectTryObtainUnicastLinkCreatesLink(nodes[0], nodes[1], channelGid, LT_BIDI);
}

TEST_F(LinkWizardTestFixture2x0, test_tryObtainUnicastLink_creator_to_loader) {
    // Setup the channel we will be using
    std::string channelGid = "uni-c2l";
    ChannelProperties props;
    props.transmissionType = TT_UNICAST;
    props.linkDirection = LD_CREATOR_TO_LOADER;
    props.connectionType = CT_INDIRECT;
    props.maxLinks = 10;
    props.currentRole.linkSide = LS_BOTH;

    nodes[0].supportedChannels = {{channelGid, props}};
    nodes[1].supportedChannels = {{channelGid, props}};

    expectSupportedChannelQuery(nodes[0], nodes[1]);
    expectTryObtainUnicastLinkCreatesLink(nodes[0], nodes[1], channelGid, LT_SEND);
}

TEST_F(LinkWizardTestFixture2x0, test_tryObtainUnicastLink_loader_to_creator) {
    // Setup the channel we will be using
    std::string channelGid = "uni-l2c";
    ChannelProperties props;
    props.transmissionType = TT_UNICAST;
    props.linkDirection = LD_LOADER_TO_CREATOR;
    props.connectionType = CT_INDIRECT;
    props.maxLinks = 10;
    props.currentRole.linkSide = LS_BOTH;

    nodes[0].supportedChannels = {{channelGid, props}};
    nodes[1].supportedChannels = {{channelGid, props}};

    expectSupportedChannelQuery(nodes[0], nodes[1]);
    expectTryObtainUnicastLinkLoadsLink(nodes[0], nodes[1], channelGid, LT_SEND);
}

TEST_F(LinkWizardTestFixture3x0, test_tryObtainMulticastSend_l2c) {
    // Setup the channel we will be using
    std::string channelGid = "uni-l2c";
    ChannelProperties props;
    props.transmissionType = TT_MULTICAST;
    props.linkDirection = LD_LOADER_TO_CREATOR;
    props.connectionType = CT_INDIRECT;
    props.maxLinks = 10;
    props.currentRole.linkSide = LS_BOTH;

    nodes[0].supportedChannels = {{channelGid, props}};
    nodes[1].supportedChannels = {{channelGid, props}};
    nodes[2].supportedChannels = {{channelGid, props}};

    expectSupportedChannelQuery(nodes[0], nodes[1]);
    expectSupportedChannelQuery(nodes[0], nodes[2]);

    // call tryObtainMulticastSend on wizard0
    EXPECT_TRUE(nodes[0].wizard->tryObtainMulticastSend({nodes[1].persona, nodes[2].persona},
                                                        LT_SEND, "uni-l2c", LS_BOTH));
    EXPECT_THAT(nodes[0].plugin->sendQueues[nodes[1].uuid].at(0),
                testing::HasSubstr("\"requestCreateMulticastRecvLink\""));
    EXPECT_THAT(nodes[0].plugin->sendQueues[nodes[2].uuid].at(0),
                testing::HasSubstr("\"requestCreateMulticastRecvLink\""));

    // wizard1 process the requestLoadLinkAddress from wizard0
    RaceHandle createHandle = 42;
    SdkResponse createResp(SDK_OK, 0.0, createHandle);
    LinkProperties linkProps1;
    linkProps1.linkAddress = nodes[1].uuid;
    linkProps1.channelGid = channelGid;
    EXPECT_CALL(nodes[1].plugin->mockLinkManager,
                createLink(channelGid, std::vector<std::string>{nodes[0].uuid}))
        .WillOnce(::testing::Return(createResp));
    EXPECT_TRUE(
        nodes[1].wizard->processLinkMsg(nodes[0].persona, nodes[0].plugin->popMsg(nodes[1].uuid)));
    nodes[1].wizard->handleLinkStatusUpdate(createHandle, "linkId", LINK_CREATED, linkProps1);

    // wizard2 process the requestLoadLinkAddress from wizard0
    LinkProperties linkProps2;
    linkProps2.linkAddress = nodes[2].uuid;
    linkProps2.channelGid = channelGid;
    EXPECT_CALL(nodes[2].plugin->mockLinkManager,
                createLink(channelGid, std::vector<std::string>{nodes[0].uuid}))
        .WillOnce(::testing::Return(createResp));
    EXPECT_TRUE(
        nodes[2].wizard->processLinkMsg(nodes[0].persona, nodes[0].plugin->popMsg(nodes[2].uuid)));
    nodes[2].wizard->handleLinkStatusUpdate(createHandle, "linkId", LINK_CREATED, linkProps2);

    // // wizard0 processes wizard2's response and can now call loadLinkAddresses
    SdkResponse loadResp(SDK_OK, 0.0, 42);
    // Note: the order is arbitrary because in the code these addresses and uuids are being pulled
    // out of an iterator over an unordered_map
    EXPECT_CALL(
        nodes[0].plugin->mockLinkManager,
        loadLinkAddresses(channelGid, std::vector<std::string>{nodes[2].uuid, nodes[1].uuid},
                          std::vector<std::string>{nodes[2].uuid, nodes[1].uuid}))
        .WillOnce(::testing::Return(loadResp));
    EXPECT_TRUE(
        nodes[0].wizard->processLinkMsg(nodes[1].persona, nodes[1].plugin->popMsg(nodes[0].uuid)));
    EXPECT_TRUE(
        nodes[0].wizard->processLinkMsg(nodes[2].persona, nodes[2].plugin->popMsg(nodes[0].uuid)));
}

TEST_F(LinkWizardTestFixture3x0, test_tryObtainMulticastSend_bidi) {
    // Setup the channel we will be using
    std::string channelGid = "uni-bidi";
    ChannelProperties props;
    props.transmissionType = TT_MULTICAST;
    props.linkDirection = LD_BIDI;
    props.connectionType = CT_INDIRECT;
    props.maxLinks = 10;
    props.currentRole.linkSide = LS_BOTH;

    nodes[0].supportedChannels = {{channelGid, props}};
    nodes[1].supportedChannels = {{channelGid, props}};
    nodes[2].supportedChannels = {{channelGid, props}};

    expectSupportedChannelQuery(nodes[0], nodes[1]);
    expectSupportedChannelQuery(nodes[0], nodes[2]);

    // call tryObtainMulticastSend on wizard0
    RaceHandle createHandle = 42;
    SdkResponse createResp(SDK_OK, 0.0, createHandle);
    EXPECT_CALL(nodes[0].plugin->mockLinkManager,
                createLink(channelGid, std::vector<std::string>{nodes[1].uuid, nodes[2].uuid}))
        .WillOnce(::testing::Return(createResp));
    EXPECT_TRUE(nodes[0].wizard->tryObtainMulticastSend({nodes[1].persona, nodes[2].persona},
                                                        LT_SEND, "uni-bidi", LS_BOTH));

    LinkProperties linkProps;
    linkProps.linkAddress = nodes[0].uuid;
    linkProps.channelGid = channelGid;
    nodes[0].wizard->handleLinkStatusUpdate(createHandle, "linkId", LINK_CREATED, linkProps);
    EXPECT_THAT(nodes[0].plugin->sendQueues[nodes[1].uuid].at(0),
                testing::HasSubstr("\"requestLoadLinkAddress\""));
    EXPECT_THAT(nodes[0].plugin->sendQueues[nodes[2].uuid].at(0),
                testing::HasSubstr("\"requestLoadLinkAddress\""));

    // wizard1 process the requestLoadLinkAddress from wizard0
    SdkResponse loadResp(SDK_OK, 0.0, 42);
    EXPECT_CALL(nodes[1].plugin->mockLinkManager,
                loadLinkAddress(channelGid, nodes[0].uuid, std::vector<std::string>{nodes[0].uuid}))
        .WillOnce(::testing::Return(loadResp));
    EXPECT_TRUE(
        nodes[1].wizard->processLinkMsg(nodes[0].persona, nodes[0].plugin->popMsg(nodes[1].uuid)));

    // wizard2 process the requestLoadLinkAddress from wizard0
    EXPECT_CALL(nodes[2].plugin->mockLinkManager,
                loadLinkAddress(channelGid, nodes[0].uuid, std::vector<std::string>{nodes[0].uuid}))
        .WillOnce(::testing::Return(loadResp));
    EXPECT_TRUE(
        nodes[2].wizard->processLinkMsg(nodes[0].persona, nodes[0].plugin->popMsg(nodes[2].uuid)));
}

// delayed requests by waiting for query response
TEST_F(LinkWizardTestFixture2x0, test_delayedTryObtain) {
    // Setup the channel we will be using
    std::string channelGid = "uni-bidi";
    ChannelProperties props;
    props.transmissionType = TT_UNICAST;
    props.linkDirection = LD_BIDI;
    props.connectionType = CT_INDIRECT;
    props.maxLinks = 10;
    props.currentRole.linkSide = LS_BOTH;

    nodes[0].supportedChannels = {{channelGid, props}};
    nodes[1].supportedChannels = {{channelGid, props}};

    // call tryObtainUnicastLink on wizard0
    RaceHandle createHandle = 42;
    SdkResponse createResp(SDK_OK, 0.0, createHandle);
    EXPECT_CALL(nodes[0].plugin->mockLinkManager,
                createLink(channelGid, std::vector<std::string>{nodes[1].uuid}))
        .WillOnce(::testing::Return(createResp));
    EXPECT_FALSE(
        nodes[0].wizard->tryObtainUnicastLink(nodes[1].persona, LT_BIDI, "uni-bidi", LS_BOTH));

    expectSupportedChannelQuery(nodes[0], nodes[1]);

    LinkProperties linkProps;
    linkProps.linkAddress = nodes[0].uuid;
    linkProps.channelGid = channelGid;
    nodes[0].wizard->handleLinkStatusUpdate(createHandle, "linkId", LINK_CREATED, linkProps);
    EXPECT_THAT(nodes[0].plugin->sendQueues[nodes[1].uuid].at(0),
                testing::HasSubstr("\"requestLoadLinkAddress\""));

    // wizard1 process the requestLoadLinkAddress from wizard0
    SdkResponse loadResp(SDK_OK, 0.0, 42);
    EXPECT_CALL(nodes[1].plugin->mockLinkManager,
                loadLinkAddress(channelGid, nodes[0].uuid, std::vector<std::string>{nodes[0].uuid}))
        .WillOnce(::testing::Return(loadResp));
    EXPECT_TRUE(
        nodes[1].wizard->processLinkMsg(nodes[0].persona, nodes[0].plugin->popMsg(nodes[1].uuid)));
}

// selection indirect for clients
TEST_F(LinkWizardTestFixture1x1, test_no_direct_for_clients) {
    // Setup the channel we will be using
    std::string channelGid = "uni-bidi";
    ChannelProperties props;
    props.transmissionType = TT_UNICAST;
    props.linkDirection = LD_BIDI;
    props.connectionType = CT_DIRECT;

    nodes[0].supportedChannels = {{channelGid, props}};
    nodes[1].supportedChannels = {{channelGid, props}};

    expectSupportedChannelQuery(nodes[0], nodes[1]);

    // call tryObtainUnicastLink on wizard0
    RaceHandle createHandle = 42;
    SdkResponse createResp(SDK_OK, 0.0, createHandle);
    EXPECT_FALSE(
        nodes[0].wizard->tryObtainUnicastLink(nodes[1].persona, LT_SEND, "uni-bidi", LS_BOTH));
}

// handles limitations on channels supported by the other node
TEST_F(LinkWizardTestFixture2x0, test_handles_limited_channels) {
    // Setup the channel we will be using
    std::string channelGid1 = "uni-bidi1";
    ChannelProperties props1;
    props1.transmissionType = TT_UNICAST;
    props1.linkDirection = LD_BIDI;
    props1.connectionType = CT_INDIRECT;
    props1.creatorExpected.send.bandwidth_bps = 10;
    props1.maxLinks = 10;
    props1.currentRole.linkSide = LS_BOTH;

    std::string channelGid2 = "uni-bidi2";
    ChannelProperties props2;
    props2.transmissionType = TT_UNICAST;
    props2.linkDirection = LD_BIDI;
    props2.connectionType = CT_INDIRECT;
    props2.creatorExpected.send.bandwidth_bps = 5;
    props2.maxLinks = 10;
    props2.currentRole.linkSide = LS_BOTH;

    std::string linkId = "link";
    LinkProperties linkProps;
    linkProps.channelGid = "uni-bidi1";
    EXPECT_CALL(*nodes[0].sdk,
                getLinksForPersonas(std::vector<std::string>{nodes[1].uuid}, LT_BIDI))
        .WillRepeatedly(::testing::Return(
            std::vector<std::string>{}));  // No links exist so uniqueness is not a concern
    EXPECT_CALL(*nodes[0].sdk, getLinkProperties(linkId))
        .WillRepeatedly(::testing::Return(linkProps));

    nodes[0].supportedChannels = {{channelGid1, props1}, {channelGid2, props2}};
    nodes[1].supportedChannels = {{channelGid2, props2}};  // node1 only supports channelGid2

    expectSupportedChannelQuery(nodes[0], nodes[1]);
    expectTryObtainUnicastLinkCreatesLink(nodes[0], nodes[1], channelGid2, LT_BIDI);
}

// selection prefers non-used channels
TEST_F(LinkWizardTestFixture2x0, test_prefer_unique_channels) {
    // Setup the channel we will be using
    std::string channelGid1 = "uni-bidi1";
    ChannelProperties props1;
    props1.transmissionType = TT_UNICAST;
    props1.linkDirection = LD_BIDI;
    props1.connectionType = CT_INDIRECT;
    props1.creatorExpected.send.bandwidth_bps = 10;
    props1.maxLinks = 10;
    props1.currentRole.linkSide = LS_BOTH;

    std::string channelGid2 = "uni-bidi2";
    ChannelProperties props2;
    props2.transmissionType = TT_UNICAST;
    props2.linkDirection = LD_BIDI;
    props2.connectionType = CT_INDIRECT;
    props2.creatorExpected.send.bandwidth_bps = 5;
    props2.maxLinks = 10;
    props2.currentRole.linkSide = LS_BOTH;

    std::string linkId = "link";
    LinkProperties linkProps;
    linkProps.channelGid = "uni-bidi1";
    EXPECT_CALL(*nodes[0].sdk,
                getLinksForPersonas(std::vector<std::string>{nodes[1].uuid}, LT_BIDI))
        .WillRepeatedly(::testing::Return(std::vector<std::string>{linkId}));
    EXPECT_CALL(*nodes[0].sdk, getLinkProperties(linkId))
        .WillRepeatedly(::testing::Return(linkProps));

    nodes[0].supportedChannels = {{channelGid1, props1}, {channelGid2, props2}};
    nodes[1].supportedChannels = {{channelGid1, props1}, {channelGid2, props2}};

    expectSupportedChannelQuery(nodes[0], nodes[1]);
    expectTryObtainUnicastLinkCreatesLink(nodes[0], nodes[1], channelGid2, LT_BIDI);
}

TEST_F(LinkWizardTestFixture2x0, test_handles_role_select_c2l_creator) {
    // Setup the channel we will be using
    std::string channelGid1 = "uni-c2l1";
    ChannelProperties props1;
    props1.transmissionType = TT_UNICAST;
    props1.linkDirection = LD_CREATOR_TO_LOADER;
    props1.connectionType = CT_INDIRECT;
    props1.creatorExpected.send.bandwidth_bps = 10;
    props1.maxLinks = 10;

    ChannelProperties props1_node0 = props1;
    ChannelProperties props1_node1 = props1;
    props1_node0.currentRole.linkSide = LS_LOADER;
    props1_node1.currentRole.linkSide = LS_CREATOR;

    std::string channelGid2 = "uni-c2l2";
    ChannelProperties props2;
    props2.transmissionType = TT_UNICAST;
    props2.linkDirection = LD_CREATOR_TO_LOADER;
    props2.connectionType = CT_INDIRECT;
    props2.creatorExpected.send.bandwidth_bps = 10;
    props2.maxLinks = 10;

    ChannelProperties props2_node0 = props2;
    ChannelProperties props2_node1 = props2;
    props2_node0.currentRole.linkSide = LS_CREATOR;
    props2_node1.currentRole.linkSide = LS_LOADER;

    nodes[0].supportedChannels = {{channelGid1, props1_node0}, {channelGid2, props2_node0}};
    nodes[1].supportedChannels = {{channelGid1, props1_node1}, {channelGid2, props2_node1}};

    expectSupportedChannelQuery(nodes[0], nodes[1]);
    expectTryObtainUnicastLinkCreatesLink(nodes[0], nodes[1], channelGid2, LT_SEND);
}

TEST_F(LinkWizardTestFixture2x0, test_handles_role_select_c2l_loader) {
    // Setup the channel we will be using
    std::string channelGid1 = "uni-c2l1";
    ChannelProperties props1;
    props1.transmissionType = TT_UNICAST;
    props1.linkDirection = LD_CREATOR_TO_LOADER;
    props1.connectionType = CT_INDIRECT;
    props1.creatorExpected.send.bandwidth_bps = 10;
    props1.maxLinks = 10;

    // Only supports node1 -> node0
    ChannelProperties props1_node0 = props1;
    ChannelProperties props1_node1 = props1;
    props1_node0.currentRole.linkSide = LS_LOADER;
    props1_node1.currentRole.linkSide = LS_CREATOR;

    std::string channelGid2 = "uni-c2l2";
    ChannelProperties props2;
    props2.transmissionType = TT_UNICAST;
    props2.linkDirection = LD_CREATOR_TO_LOADER;
    props2.connectionType = CT_INDIRECT;
    props2.creatorExpected.send.bandwidth_bps = 10;
    props2.maxLinks = 10;

    // Only supports node0 -> node1
    ChannelProperties props2_node0 = props2;
    ChannelProperties props2_node1 = props2;
    props2_node0.currentRole.linkSide = LS_CREATOR;
    props2_node1.currentRole.linkSide = LS_LOADER;

    nodes[0].supportedChannels = {{channelGid1, props1_node0}, {channelGid2, props2_node0}};
    nodes[1].supportedChannels = {{channelGid1, props1_node1}, {channelGid2, props2_node1}};

    expectSupportedChannelQuery(nodes[0], nodes[1]);
    expectTryObtainUnicastLinkLoadsLink(nodes[0], nodes[1], channelGid1, LT_RECV);
}

TEST_F(LinkWizardTestFixture2x0, test_handles_role_select_l2c_creator) {
    // Setup the channel we will be using
    std::string channelGid1 = "uni-l2c1";
    ChannelProperties props1;
    props1.transmissionType = TT_UNICAST;
    props1.linkDirection = LD_LOADER_TO_CREATOR;
    props1.connectionType = CT_INDIRECT;
    props1.creatorExpected.send.bandwidth_bps = 10;
    props1.maxLinks = 10;

    ChannelProperties props1_node0 = props1;
    ChannelProperties props1_node1 = props1;
    props1_node0.currentRole.linkSide = LS_LOADER;
    props1_node1.currentRole.linkSide = LS_CREATOR;

    std::string channelGid2 = "uni-l2c2";
    ChannelProperties props2;
    props2.transmissionType = TT_UNICAST;
    props2.linkDirection = LD_LOADER_TO_CREATOR;
    props2.connectionType = CT_INDIRECT;
    props2.creatorExpected.send.bandwidth_bps = 10;
    props2.maxLinks = 10;

    ChannelProperties props2_node0 = props2;
    ChannelProperties props2_node1 = props2;
    props2_node0.currentRole.linkSide = LS_CREATOR;
    props2_node1.currentRole.linkSide = LS_LOADER;

    nodes[0].supportedChannels = {{channelGid1, props1_node0}, {channelGid2, props2_node0}};
    nodes[1].supportedChannels = {{channelGid1, props1_node1}, {channelGid2, props2_node1}};

    expectSupportedChannelQuery(nodes[0], nodes[1]);
    expectTryObtainUnicastLinkLoadsLink(nodes[0], nodes[1], channelGid1, LT_SEND);
}

TEST_F(LinkWizardTestFixture2x0, test_handles_role_select_l2c_loader) {
    // Setup the channel we will be using
    std::string channelGid1 = "uni-l2c1";
    ChannelProperties props1;
    props1.transmissionType = TT_UNICAST;
    props1.linkDirection = LD_LOADER_TO_CREATOR;
    props1.connectionType = CT_INDIRECT;
    props1.creatorExpected.send.bandwidth_bps = 10;
    props1.maxLinks = 10;

    ChannelProperties props1_node0 = props1;
    ChannelProperties props1_node1 = props1;
    props1_node0.currentRole.linkSide = LS_LOADER;
    props1_node1.currentRole.linkSide = LS_CREATOR;

    std::string channelGid2 = "uni-l2c2";
    ChannelProperties props2;
    props2.transmissionType = TT_UNICAST;
    props2.linkDirection = LD_LOADER_TO_CREATOR;
    props2.connectionType = CT_INDIRECT;
    props2.creatorExpected.send.bandwidth_bps = 10;
    props2.maxLinks = 10;

    ChannelProperties props2_node0 = props2;
    ChannelProperties props2_node1 = props2;
    props2_node0.currentRole.linkSide = LS_CREATOR;
    props2_node1.currentRole.linkSide = LS_LOADER;

    nodes[0].supportedChannels = {{channelGid1, props1_node0}, {channelGid2, props2_node0}};
    nodes[1].supportedChannels = {{channelGid1, props1_node1}, {channelGid2, props2_node1}};

    expectSupportedChannelQuery(nodes[0], nodes[1]);
    expectTryObtainUnicastLinkCreatesLink(nodes[0], nodes[1], channelGid2, LT_RECV);
}

TEST_F(LinkWizardTestFixture2x0, test_selectChannel_maxLinks_check) {
    // Setup the channel we will be using
    std::string channelGid = "uni-bidi";
    ChannelProperties props;
    props.transmissionType = TT_UNICAST;
    props.linkDirection = LD_BIDI;
    props.connectionType = CT_INDIRECT;
    props.maxLinks = 0;

    nodes[0].supportedChannels = {{channelGid, props}};
    nodes[1].supportedChannels = {{channelGid, props}};

    expectSupportedChannelQuery(nodes[0], nodes[1]);

    // call tryObtainUnicastLink on wizard0
    RaceHandle createHandle = 42;
    SdkResponse createResp(SDK_OK, 0.0, createHandle);
    EXPECT_FALSE(
        nodes[0].wizard->tryObtainUnicastLink(nodes[1].persona, LT_BIDI, "uni-bidi", LS_BOTH));
}
