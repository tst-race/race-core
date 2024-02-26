
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

#include "BootstrapManager.h"
#include "MockPluginNM.h"
#include "gtest/gtest.h"
#include "race/mocks/MockRaceSdkNM.h"

using ::testing::_;
using ::testing::Field;
using ::testing::Return;

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os, const ChannelProperties &props) {
    os << "ChannelProperties{connectionType: " << connectionTypeToString(props.connectionType)
       << ", linkDirection: " << linkDirectionToString(props.linkDirection) << "}";

    return os;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os,
                                                 const std::vector<std::string> &vec) {
    os << "[";
    for (auto &e : vec) {
        os << e << ", ";
    }
    os << "]";
    return os;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os,
                                                 const BootstrapManager::BootstrapMessage &bMsg) {
    os << "BootstrapMessage{type: " << bMsg.type << ", messageHandle: " << bMsg.messageHandle
       << ", bootstrapHandle: " << bMsg.bootstrapHandle << ", linkAddresses: " << bMsg.linkAddresses
       << ", channelGids: " << bMsg.channelGids << ", persona: " << bMsg.persona
       << ", key: " << bMsg.key << "}";

    return os;
}

bool operator==(const BootstrapManager::BootstrapMessage &lhs,
                const BootstrapManager::BootstrapMessage &rhs) {
    return lhs.type == rhs.type && lhs.messageHandle == rhs.messageHandle &&
           lhs.bootstrapHandle == rhs.bootstrapHandle && lhs.linkAddresses == rhs.linkAddresses &&
           lhs.channelGids == rhs.channelGids && lhs.persona == rhs.persona && lhs.key == rhs.key;
}

// mock to override sendBootstrapMsg for testing
class MockBootstrapManager : public BootstrapManager {
public:
    explicit MockBootstrapManager(PluginNMTwoSix *plugin) : BootstrapManager(plugin) {}
    virtual ~MockBootstrapManager() {}
    MOCK_METHOD(RaceHandle, sendBootstrapMsg,
                (const BootstrapMessage &bMsg, const std::string &dest), (override));
    MOCK_METHOD(void, sendBootstrapPkg,
                (const BootstrapMessage &bMsg, const std::string &dest, const ConnectionID &connId),
                (override));
};

class BootstrapManagerTestFixture : public ::testing::Test {
public:
    BootstrapManagerTestFixture() : sdk() {
        ON_CALL(sdk, getActivePersona()).WillByDefault(Return("test-persona"));

        ChannelProperties localProperties;
        localProperties.connectionType = CT_LOCAL;
        localProperties.linkDirection = LD_LOADER_TO_CREATOR;
        localProperties.currentRole.linkSide = LS_BOTH;

        ChannelProperties directProperties;
        directProperties.connectionType = CT_DIRECT;
        directProperties.linkDirection = LD_LOADER_TO_CREATOR;
        directProperties.currentRole.linkSide = LS_BOTH;

        ChannelProperties directPropertiesBidi;
        directPropertiesBidi.connectionType = CT_DIRECT;
        directPropertiesBidi.linkDirection = LD_BIDI;
        directPropertiesBidi.currentRole.linkSide = LS_BOTH;

        ChannelProperties indirectPropertiesLoader;
        indirectPropertiesLoader.connectionType = CT_INDIRECT;
        indirectPropertiesLoader.linkDirection = LD_LOADER_TO_CREATOR;
        indirectPropertiesLoader.currentRole.linkSide = LS_LOADER;

        ChannelProperties indirectPropertiesCreator;
        indirectPropertiesCreator.connectionType = CT_INDIRECT;
        indirectPropertiesCreator.linkDirection = LD_LOADER_TO_CREATOR;
        indirectPropertiesCreator.currentRole.linkSide = LS_CREATOR;

        ChannelProperties indirectPropertiesBidi;
        indirectPropertiesBidi.connectionType = CT_INDIRECT;
        indirectPropertiesBidi.linkDirection = LD_BIDI;
        indirectPropertiesBidi.currentRole.linkSide = LS_BOTH;

        auto channels = std::map<std::string, ChannelProperties>({
            {"local-channel", localProperties},
            {"direct-channel", directProperties},
            {"direct-channel-bidi", directPropertiesBidi},
            {"indirect-channel-loader", indirectPropertiesLoader},
            {"indirect-channel-creator", indirectPropertiesCreator},
            {"indirect-channel-bidi", indirectPropertiesBidi},
        });
        ON_CALL(sdk, getSupportedChannels()).WillByDefault(Return(channels));

        ChannelProperties maxLinksProperties;
        maxLinksProperties.maxLinks = 10;
        // getChannelProperties used when checking channel maxLinks
        ON_CALL(sdk, getChannelProperties(_)).WillByDefault(Return(maxLinksProperties));

        plugin = std::make_unique<MockPluginNM>(sdk);
        bootstrap = std::make_unique<MockBootstrapManager>(plugin.get());
    }
    virtual ~BootstrapManagerTestFixture() {}
    virtual void SetUp() override {}
    virtual void TearDown() override {}

public:
    MockRaceSdkNM sdk;
    std::unique_ptr<MockPluginNM> plugin;
    std::unique_ptr<MockBootstrapManager> bootstrap;
};

static ExtClrMsg replaceSender(const ExtClrMsg &in, const std::string &sender) {
    // ignore fields that don't matter for bootstrapping
    return ExtClrMsg(in.getMsg(), sender, in.getTo(), 0, 0, 0, 0, 0, 0, in.getMsgType());
}

TEST_F(BootstrapManagerTestFixture, init) {
    EXPECT_EQ(bootstrap->messageCounter, 1);
}

TEST_F(BootstrapManagerTestFixture, onPrepareToBootstrap) {
    RaceHandle handle = 32;
    const std::string configPath = "/tmp/BootstrapManagerTest/configs";
    const std::vector<std::string> entranceCommittee = {"node1", "node2", "node3"};
    LinkID linkId = "bootstrap-link";

    EXPECT_CALL(*bootstrap, sendBootstrapMsg(Field(&BootstrapManager::BootstrapMessage::type,
                                                   BootstrapManager::LINK_CREATE_REQUEST),
                                             "node1"));
    EXPECT_CALL(*bootstrap, sendBootstrapMsg(Field(&BootstrapManager::BootstrapMessage::type,
                                                   BootstrapManager::LINK_CREATE_REQUEST),
                                             "node2"));
    EXPECT_CALL(*bootstrap, sendBootstrapMsg(Field(&BootstrapManager::BootstrapMessage::type,
                                                   BootstrapManager::LINK_CREATE_REQUEST),
                                             "node3"));

    bootstrap->onPrepareToBootstrap(handle, linkId, configPath, entranceCommittee);

    EXPECT_EQ(bootstrap->outstandingBootstraps.size(), 1);
}

TEST_F(BootstrapManagerTestFixture, onBootstrapFinished) {
    RaceHandle handle = 32;
    RaceHandle closeConnHandle = 56;
    const std::string configPath = "/tmp/BootstrapManagerTest/configs";
    const std::vector<std::string> entranceCommittee = {"node1", "node2", "node3"};
    LinkID linkId = "bootstrap-link";
    ConnectionID connId = "bootstrap-connection";
    LinkProperties props;

    EXPECT_CALL(*bootstrap, sendBootstrapMsg(Field(&BootstrapManager::BootstrapMessage::type,
                                                   BootstrapManager::LINK_CREATE_REQUEST),
                                             "node1"));
    EXPECT_CALL(*bootstrap, sendBootstrapMsg(Field(&BootstrapManager::BootstrapMessage::type,
                                                   BootstrapManager::LINK_CREATE_REQUEST),
                                             "node2"));
    EXPECT_CALL(*bootstrap, sendBootstrapMsg(Field(&BootstrapManager::BootstrapMessage::type,
                                                   BootstrapManager::LINK_CREATE_REQUEST),
                                             "node3"));

    bootstrap->onPrepareToBootstrap(handle, linkId, configPath, entranceCommittee);

    // bootstrap must track open connections
    // assume sdk will call onConnectionStatusChanged with CONNECTION_OPEN
    EXPECT_EQ(bootstrap->outstandingBootstraps.size(), 1);
    RaceHandle connHandle = bootstrap->outstandingBootstraps[0].outstandingOpenConnectionHandle;
    bootstrap->onConnectionStatusChanged(connHandle, connId, CONNECTION_OPEN, linkId, props);

    // onBootstrapCancelled closes connections and destroys links
    EXPECT_CALL(sdk, closeConnection(connId, _));
    bootstrap->onBootstrapFinished(handle, BootstrapState::BOOTSTRAP_CANCELLED);

    // assume sdk will call onConnectionStatusChanged in response to closeConnection
    EXPECT_CALL(sdk, destroyLink(linkId, _));
    bootstrap->onConnectionStatusChanged(closeConnHandle, connId, CONNECTION_CLOSED, linkId, props);
    EXPECT_EQ(bootstrap->outstandingBootstraps.size(), 0);

    // onBootstrapCancelled destroys links if no connections
    BootstrapManager::OutstandingBootstrap request;
    request.sdkHandle = handle;
    request.configPath = configPath;
    request.bootstrapLinkId = linkId;
    bootstrap->outstandingBootstraps.push_back(request);
    EXPECT_CALL(sdk, destroyLink(linkId, _));
    bootstrap->onBootstrapFinished(handle, BootstrapState::BOOTSTRAP_CANCELLED);
    EXPECT_EQ(bootstrap->outstandingBootstraps.size(), 0);
}

TEST_F(BootstrapManagerTestFixture, linkCreateRequest) {
    BootstrapManager::BootstrapMessage bMsg;
    bMsg.type = BootstrapManager::LINK_CREATE_REQUEST;
    bMsg.messageHandle = 15;
    bMsg.bootstrapHandle = 31;
    bMsg.channelGids = {"local-channel", "indirect-channel-loader", "indirect-channel-creator",
                        "indirect-channel-bidi"};

    EXPECT_CALL(plugin->mockLinkManager, createLink(_, _)).Times(0);
    EXPECT_CALL(plugin->mockLinkManager, createLink("indirect-channel-bidi", _))
        .WillOnce(Return(SDK_OK));
    EXPECT_CALL(plugin->mockLinkManager, createLink("indirect-channel-creator", _))
        .WillOnce(Return(SDK_OK));

    ExtClrMsg msg = bootstrap->createClrMsg(bMsg, plugin->getUuid());

    bootstrap->onBootstrapMessage(msg);
}

TEST_F(BootstrapManagerTestFixture, linkCreateRequest2) {
    BootstrapManager::BootstrapMessage bMsg;
    bMsg.type = BootstrapManager::LINK_CREATE_REQUEST;
    bMsg.messageHandle = 15;
    bMsg.bootstrapHandle = 31;
    bMsg.channelGids = {"indirect-channel-creator", "indirect-channel-bidi"};

    RaceHandle handle1 = 16;
    LinkID linkId1 = "indirect-link";
    LinkProperties props1;
    props1.linkAddress = "indirect-link-address";
    props1.channelGid = "indirect-channel-creator";

    RaceHandle handle2 = 17;
    LinkID linkId2 = "indirect-bidi-link";
    LinkProperties props2;
    props2.linkAddress = "indirect-bidi-link-address";
    props2.channelGid = "indirect-channel-bidi";

    EXPECT_CALL(plugin->mockLinkManager, createLink(_, _)).Times(0);
    EXPECT_CALL(plugin->mockLinkManager, createLink(props1.channelGid, _))
        .WillOnce(Return(SdkResponse{SDK_OK, 0, handle1}));
    EXPECT_CALL(plugin->mockLinkManager, createLink(props2.channelGid, _))
        .WillOnce(Return(SdkResponse{SDK_OK, 0, handle2}));
    EXPECT_CALL(*bootstrap, sendBootstrapMsg(_, "test-persona"));

    ExtClrMsg msg = bootstrap->createClrMsg(bMsg, plugin->getUuid());

    bootstrap->onBootstrapMessage(msg);

    EXPECT_EQ(bootstrap->outstandingCreateLinks.size(), 1);

    bootstrap->onLinkStatusChanged(handle1, linkId1, LINK_CREATED, props1);

    EXPECT_EQ(bootstrap->outstandingCreateLinks.size(), 1);

    bootstrap->onLinkStatusChanged(handle2, linkId2, LINK_CREATED, props2);

    EXPECT_EQ(bootstrap->outstandingCreateLinks.size(), 0);
}

TEST_F(BootstrapManagerTestFixture, onPrepareToBootstrap2) {
    RaceHandle handle = 32;
    const std::string configPath = "/tmp/BootstrapManagerTest/configs";
    const std::vector<std::string> entranceCommittee = {"node1", "node2", "node3"};
    LinkID linkId = "bootstrap-link";
    LinkProperties props;
    props.linkAddress = "bootstrap-link-address";
    props.channelGid = "bootstrap-channel";

    EXPECT_CALL(*bootstrap, sendBootstrapMsg(_, "node1")).Times(1);
    EXPECT_CALL(*bootstrap, sendBootstrapMsg(_, "node2")).Times(1);
    EXPECT_CALL(*bootstrap, sendBootstrapMsg(_, "node3")).Times(1);
    EXPECT_CALL(sdk, getLinkProperties(linkId)).WillOnce(Return(props));

    bootstrap->onPrepareToBootstrap(handle, linkId, configPath, entranceCommittee);

    EXPECT_EQ(bootstrap->outstandingBootstraps.size(), 1);

    BootstrapManager::BootstrapMessage bMsg1;
    bMsg1.type = BootstrapManager::LINK_CREATE_RESPONSE;
    bMsg1.messageHandle = 1;
    bMsg1.bootstrapHandle = 1;
    bMsg1.persona = "node1";
    bMsg1.linkAddresses = {"link-address-1", "link-address-2"};
    bMsg1.channelGids = {"indirect-channel", "indirect-channel-bidi"};
    ExtClrMsg msg1 = bootstrap->createClrMsg(bMsg1, plugin->getUuid());

    BootstrapManager::BootstrapMessage bMsg2;
    bMsg2.type = BootstrapManager::LINK_CREATE_RESPONSE;
    bMsg2.messageHandle = 2;
    bMsg2.bootstrapHandle = 1;
    bMsg2.persona = "node2";
    bMsg2.linkAddresses = {"link-address-3", "link-address-4"};
    bMsg2.channelGids = {"indirect-channel", "indirect-channel-bidi"};
    ExtClrMsg msg2 = bootstrap->createClrMsg(bMsg2, plugin->getUuid());

    BootstrapManager::BootstrapMessage bMsg3;
    bMsg3.type = BootstrapManager::LINK_CREATE_RESPONSE;
    bMsg3.messageHandle = 3;
    bMsg3.bootstrapHandle = 1;
    bMsg3.persona = "node3";
    bMsg3.linkAddresses = {"link-address-5", "link-address-6"};
    bMsg3.channelGids = {"indirect-channel", "indirect-channel-bidi"};
    ExtClrMsg msg3 = bootstrap->createClrMsg(bMsg3, plugin->getUuid());

    std::string expectedConfigs = R"({
    "bootstrap-channel": [
        {
            "address": "bootstrap-link-address",
            "description": "",
            "personas": [
                "test-persona"
            ],
            "role": "loader"
        }
    ],
    "indirect-channel": [
        {
            "address": "link-address-1",
            "description": "",
            "personas": [
                "node1"
            ],
            "role": "loader"
        },
        {
            "address": "link-address-3",
            "description": "",
            "personas": [
                "node2"
            ],
            "role": "loader"
        },
        {
            "address": "link-address-5",
            "description": "",
            "personas": [
                "node3"
            ],
            "role": "loader"
        }
    ],
    "indirect-channel-bidi": [
        {
            "address": "link-address-2",
            "description": "",
            "personas": [
                "node1"
            ],
            "role": "loader"
        },
        {
            "address": "link-address-4",
            "description": "",
            "personas": [
                "node2"
            ],
            "role": "loader"
        },
        {
            "address": "link-address-6",
            "description": "",
            "personas": [
                "node3"
            ],
            "role": "loader"
        }
    ]
})";

    const std::vector<std::uint8_t> expectedConfigsRaw = {expectedConfigs.begin(),
                                                          expectedConfigs.end()};
    EXPECT_CALL(sdk, writeFile(configPath + "/link-profiles.json", expectedConfigsRaw)).Times(1);

    EXPECT_CALL(sdk, writeFile(configPath + "/config.json", _)).Times(1);

    bootstrap->onBootstrapMessage(msg1);
    bootstrap->onBootstrapMessage(msg2);
    bootstrap->onBootstrapMessage(msg3);
}

TEST_F(BootstrapManagerTestFixture, onBootstrapStart) {
    const std::vector<std::string> entranceCommittee = {"node1", "node2", "node3"};
    std::string introducer_persona = "introducer";
    LinkID linkId = "linkId";
    ConnectionID connId = "connId";

    EXPECT_CALL(*plugin, getExpectedChannels(_))
        .WillRepeatedly(::testing::Return(
            std::vector<std::string>{"local-channel", "indirect-channel-loader",
                                     "indirect-channel-creator", "indirect-channel-bidi"}));
    EXPECT_CALL(sdk, getPersonasForLink(linkId))
        .WillOnce(Return(std::vector<std::string>{introducer_persona}));
    EXPECT_CALL(*bootstrap, sendBootstrapPkg(_, introducer_persona, connId)).Times(1);
    EXPECT_CALL(plugin->mockLinkManager, createLink(_, _)).Times(3).WillRepeatedly(Return(SDK_OK));

    bootstrap->onBootstrapStart(introducer_persona, entranceCommittee, 1234567890);
    bootstrap->onConnectionStatusChanged(0, connId, CONNECTION_OPEN, linkId, {});

    EXPECT_EQ(bootstrap->outstandingCreateLinks.size(), 3);
}

TEST_F(BootstrapManagerTestFixture, onBootstrapStart2) {
    const std::vector<std::string> entranceCommittee = {"node1", "node2", "node3"};
    std::string introducer_persona = "introducer";

    RaceHandle handle1 = 16;
    RaceHandle handle2 = 26;
    RaceHandle handle3 = 36;
    LinkID linkId1 = "indirect-link";
    LinkProperties props1;
    props1.linkAddress = "indirect-link-address";
    props1.channelGid = "indirect-channel-creator";

    LinkID linkId = "linkId";
    ConnectionID connId = "connId";

    EXPECT_CALL(*plugin, getExpectedChannels(_))
        .WillRepeatedly(::testing::Return(
            std::vector<std::string>{"indirect-channel-creator", "indirect-channel-bidi"}));
    EXPECT_CALL(sdk, getPersonasForLink(linkId))
        .WillOnce(Return(std::vector<std::string>{introducer_persona}));
    EXPECT_CALL(*bootstrap, sendBootstrapMsg(_, introducer_persona)).Times(4);
    EXPECT_CALL(*bootstrap, sendBootstrapPkg(_, introducer_persona, connId)).Times(1);
    EXPECT_CALL(plugin->mockLinkManager, createLink(props1.channelGid, _))
        .Times(3)
        .WillOnce(Return(SdkResponse{SDK_OK, 0, handle1}))
        .WillOnce(Return(SdkResponse{SDK_OK, 0, handle2}))
        .WillOnce(Return(SdkResponse{SDK_OK, 0, handle3}));

    bootstrap->onBootstrapStart(introducer_persona, entranceCommittee, 1234567890);
    bootstrap->onConnectionStatusChanged(0, connId, CONNECTION_OPEN, linkId, {});
    EXPECT_EQ(bootstrap->outstandingCreateLinks.size(), 3);

    bootstrap->onLinkStatusChanged(handle1, linkId1, LINK_CREATED, props1);
    EXPECT_EQ(bootstrap->outstandingCreateLinks.size(), 2);

    bootstrap->onLinkStatusChanged(handle2, linkId1, LINK_CREATED, props1);
    EXPECT_EQ(bootstrap->outstandingCreateLinks.size(), 1);

    bootstrap->onLinkStatusChanged(handle3, linkId1, LINK_CREATED, props1);
    EXPECT_EQ(bootstrap->outstandingCreateLinks.size(), 0);
}

TEST_F(BootstrapManagerTestFixture, handleLinkLoadRequest) {
    std::string sender = "Alice";
    std::string destination = "com";

    BootstrapManager::BootstrapMessage bMsg;
    bMsg.type = BootstrapManager::LINK_LOAD_REQUEST;
    bMsg.persona = destination;
    bMsg.linkAddresses = {"link-address-1", "link-address-2"};
    bMsg.channelGids = {"channel-1", "channel-2"};
    ExtClrMsg msg = bootstrap->createClrMsg(bMsg, plugin->getUuid());
    msg = replaceSender(msg, sender);

    BootstrapManager::BootstrapMessage result;
    result.type = BootstrapManager::LINK_LOAD_REQUEST_FORWARD;
    result.persona = sender;
    result.linkAddresses = bMsg.linkAddresses;
    result.channelGids = bMsg.channelGids;

    EXPECT_CALL(*bootstrap, sendBootstrapMsg(result, destination)).Times(1);
    bootstrap->onBootstrapMessage(msg);
}

TEST_F(BootstrapManagerTestFixture, handleLinkLoadRequestForward) {
    std::string sender = "Alice";
    std::string introducer = "Bob";

    BootstrapManager::BootstrapMessage bMsg;
    bMsg.type = BootstrapManager::LINK_LOAD_REQUEST_FORWARD;
    bMsg.persona = sender;
    bMsg.linkAddresses = {"link-address-1", "link-address-2"};
    bMsg.channelGids = {"channel-1", "channel-2"};
    ExtClrMsg msg = bootstrap->createClrMsg(bMsg, plugin->getUuid());
    msg = replaceSender(msg, introducer);

    EXPECT_CALL(plugin->mockLinkManager,
                loadLinkAddress("channel-1", "link-address-1", std::vector<std::string>{sender}))
        .Times(1);
    EXPECT_CALL(plugin->mockLinkManager,
                loadLinkAddress("channel-2", "link-address-2", std::vector<std::string>{sender}))
        .Times(1);
    bootstrap->onBootstrapMessage(msg);
}

TEST_F(BootstrapManagerTestFixture, onBootstrapPkg) {
    std::string sender = "Alice";
    const std::vector<std::string> entranceCommittee = {"node1", "node2", "node3"};

    // std::string rawKey = "abcdefghijklmnopqrstuvwxyz1234567890"; // before base64

    BootstrapManager::BootstrapMessage bMsg;
    bMsg.type = BootstrapManager::BOOTSTRAP_PACKAGE;
    bMsg.persona = sender;
    bMsg.key = "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXoxMjM0NTY3ODkw";
    ExtClrMsg msg = bootstrap->createClrMsg(bMsg, plugin->getUuid());
    msg = replaceSender(msg, sender);

    BootstrapManager::BootstrapMessage result;
    result.type = BootstrapManager::ADD_PERSONA;
    result.persona = sender;
    result.key = bMsg.key;

    EXPECT_CALL(*bootstrap, sendBootstrapMsg(result, "node1")).Times(1);
    EXPECT_CALL(*bootstrap, sendBootstrapMsg(result, "node2")).Times(1);
    EXPECT_CALL(*bootstrap, sendBootstrapMsg(result, "node3")).Times(1);
    bootstrap->onBootstrapPackage(sender, msg, entranceCommittee);
}

TEST_F(BootstrapManagerTestFixture, handleAddPersona) {
    std::string sender = "Alice";
    std::string introducer = "Bob";
    std::string rawKey = "abcdefghijklmnopqrstuvwxyz1234567890";  // before base64

    BootstrapManager::BootstrapMessage bMsg;
    bMsg.type = BootstrapManager::ADD_PERSONA;
    bMsg.persona = sender;
    bMsg.key = "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXoxMjM0NTY3ODkw";
    ExtClrMsg msg = bootstrap->createClrMsg(bMsg, plugin->getUuid());
    msg = replaceSender(msg, introducer);

    EXPECT_CALL(*plugin, addClient(sender, RawData{rawKey.begin(), rawKey.end()})).Times(1);
    bootstrap->onBootstrapMessage(msg);
}

TEST_F(BootstrapManagerTestFixture, handleAddPersona2) {
    std::string sender = "Alice";
    std::string introducer = "Bob";

    BootstrapManager::BootstrapMessage bMsg;
    bMsg.type = BootstrapManager::LINK_CREATE_REQUEST;
    bMsg.messageHandle = 15;
    bMsg.bootstrapHandle = 31;
    bMsg.channelGids = {"local-channel", "indirect-channel-loader", "indirect-channel-creator",
                        "indirect-channel-bidi"};
    ExtClrMsg msg = bootstrap->createClrMsg(bMsg, plugin->getUuid());
    msg = replaceSender(msg, introducer);

    RaceHandle handle1 = 16;
    LinkID linkId1 = "indirect-link";
    LinkProperties props1;
    props1.linkAddress = "indirect-link-address";
    props1.channelGid = "indirect-channel-creator";
    props1.linkType = LT_RECV;

    RaceHandle handle2 = 17;
    LinkID linkId2 = "indirect-bidi-link";
    LinkProperties props2;
    props2.linkAddress = "indirect-bidi-link-address";
    props2.channelGid = "indirect-channel-bidi";
    props2.linkType = LT_BIDI;

    EXPECT_CALL(plugin->mockLinkManager, createLink(props1.channelGid, _))
        .WillOnce(Return(SdkResponse{SDK_OK, 0, handle1}));
    EXPECT_CALL(plugin->mockLinkManager, createLink(props2.channelGid, _))
        .WillOnce(Return(SdkResponse{SDK_OK, 0, handle2}));
    EXPECT_CALL(*bootstrap, sendBootstrapMsg(_, introducer));

    bootstrap->onBootstrapMessage(msg);

    EXPECT_EQ(bootstrap->outstandingCreateLinks.size(), 1);

    bootstrap->onLinkStatusChanged(handle1, linkId1, LINK_CREATED, props1);
    bootstrap->onLinkStatusChanged(handle2, linkId2, LINK_CREATED, props2);

    EXPECT_EQ(bootstrap->outstandingCreateLinks.size(), 0);

    std::string rawKey = "abcdefghijklmnopqrstuvwxyz1234567890";  // before base64

    BootstrapManager::BootstrapMessage bMsg2;
    bMsg2.type = BootstrapManager::ADD_PERSONA;
    bMsg2.persona = sender;
    bMsg2.bootstrapHandle = 31;
    bMsg2.key = "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXoxMjM0NTY3ODkw";
    ExtClrMsg msg2 = bootstrap->createClrMsg(bMsg2, plugin->getUuid());
    msg2 = replaceSender(msg2, introducer);

    EXPECT_CALL(sdk, getLinkProperties(linkId1)).WillOnce(Return(props1));
    EXPECT_CALL(sdk, getLinkProperties(linkId2)).WillOnce(Return(props2));
    EXPECT_CALL(*plugin, addClient(sender, RawData{rawKey.begin(), rawKey.end()})).Times(1);
    EXPECT_CALL(plugin->mockLinkManager,
                setPersonasForLink(linkId1, std::vector<std::string>{sender}))
        .WillOnce(Return(SDK_OK));
    EXPECT_CALL(plugin->mockLinkManager,
                setPersonasForLink(linkId2, std::vector<std::string>{sender}))
        .WillOnce(Return(SDK_OK));
    bootstrap->onBootstrapMessage(msg2);
}

TEST_F(BootstrapManagerTestFixture, test_channelLinksFull) {
    ChannelProperties props;
    props.maxLinks = 0;
    ON_CALL(sdk, getChannelProperties(_)).WillByDefault(Return(props));

    BootstrapManager::BootstrapMessage bMsg;
    bMsg.type = BootstrapManager::LINK_CREATE_REQUEST;
    bMsg.messageHandle = 15;
    bMsg.bootstrapHandle = 31;
    ExtClrMsg msg = bootstrap->createClrMsg(bMsg, plugin->getUuid());
    EXPECT_EQ(bootstrap->onBootstrapMessage(msg), PLUGIN_OK);
    EXPECT_EQ(bootstrap->outstandingCreateLinks.size(), 0);
}