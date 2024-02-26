
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

#include "MockLink.h"
#include "PluginCommsTwoSixStubTransport.h"

class TestedTransport : public PluginCommsTwoSixStubTransport {
public:
    ITransportSdk *transportSdk;
    std::unordered_map<LinkID, std::shared_ptr<MockLink>> &mockLinks;

    TestedTransport(ITransportSdk *sdk,
                    std::unordered_map<LinkID, std::shared_ptr<MockLink>> &mockLinks) :
        PluginCommsTwoSixStubTransport(sdk), transportSdk(sdk), mockLinks(mockLinks) {}

    std::shared_ptr<Link> createLinkInstance(const LinkID &linkId, const LinkAddress &address,
                                             const LinkProperties &properties) override {
        auto link = std::make_shared<MockLink>(linkId, address, properties, transportSdk);
        mockLinks[linkId] = link;
        return link;
    }
};

class TestPluginCommsTwoSixStubTransport : public ::testing::Test {
public:
    ::testing::NiceMock<MockTransportSdk> sdk;
    ChannelProperties channelProps;
    std::unordered_map<LinkID, std::shared_ptr<MockLink>> mockLinks;

    TestPluginCommsTwoSixStubTransport() {
        channelProps.maxLinks = 10;
        channelProps.currentRole.linkSide = LS_CREATOR;
        ON_CALL(sdk, getActivePersona()).WillByDefault(::testing::Return("race-client-1"));
        // Using a lambda so we can set specific channel props in tests. Using ::testing::Return
        // creates a copy of the channelProps so tests can't update it
        ON_CALL(sdk, getChannelProperties()).WillByDefault(::testing::Invoke([this]() {
            return channelProps;
        }));
    }

    std::unique_ptr<PluginCommsTwoSixStubTransport> createTransport() {
        return std::make_unique<TestedTransport>(&sdk, mockLinks);
    }
};

TEST_F(TestPluginCommsTwoSixStubTransport, should_refuse_to_create_link_when_max_links_exceeded) {
    channelProps.maxLinks = 0;
    auto transport = createTransport();
    EXPECT_CALL(sdk, onLinkStatusChanged(1u, "LinkID_1", LINK_DESTROYED, ::testing::_));
    ASSERT_EQ(COMPONENT_ERROR, transport->createLink(1u, "LinkID_1"));
}

TEST_F(TestPluginCommsTwoSixStubTransport,
       should_refuse_to_create_link_when_invalid_role_link_side) {
    channelProps.currentRole.linkSide = LS_LOADER;
    auto transport = createTransport();
    EXPECT_CALL(sdk, onLinkStatusChanged(2u, "LinkID_2", LINK_DESTROYED, ::testing::_));
    ASSERT_EQ(COMPONENT_ERROR, transport->createLink(2u, "LinkID_2"));
}

TEST_F(TestPluginCommsTwoSixStubTransport, should_create_link) {
    channelProps.currentRole.linkSide = LS_CREATOR;
    auto transport = createTransport();
    EXPECT_CALL(sdk, onLinkStatusChanged(3u, "LinkID_3", LINK_CREATED, ::testing::_));
    ASSERT_EQ(COMPONENT_OK, transport->createLink(3u, "LinkID_3"));
}

TEST_F(TestPluginCommsTwoSixStubTransport, should_load_link_address) {
    channelProps.currentRole.linkSide = LS_LOADER;
    auto transport = createTransport();
    EXPECT_CALL(sdk, onLinkStatusChanged(4u, "LinkID_4", LINK_LOADED, ::testing::_));
    ASSERT_EQ(COMPONENT_OK,
              transport->loadLinkAddress(4u, "LinkID_4", "{\"hashtag\":\"test_hashtag\"}"));
}

TEST_F(TestPluginCommsTwoSixStubTransport, should_not_load_link_addresses) {
    auto transport = createTransport();
    EXPECT_CALL(sdk, onLinkStatusChanged(5u, "LinkID_5", LINK_DESTROYED, ::testing::_));
    ASSERT_EQ(COMPONENT_ERROR, transport->loadLinkAddresses(5u, "LinkID_5", {}));
}

TEST_F(TestPluginCommsTwoSixStubTransport, should_create_link_from_address) {
    channelProps.currentRole.linkSide = LS_CREATOR;
    auto transport = createTransport();
    EXPECT_CALL(sdk, onLinkStatusChanged(6u, "LinkID_6", LINK_CREATED, ::testing::_));
    ASSERT_EQ(COMPONENT_OK,
              transport->createLinkFromAddress(6u, "LinkID_6", "{\"hashtag\":\"test_hashtag\"}"));
}

TEST_F(TestPluginCommsTwoSixStubTransport, should_destroy_link) {
    auto transport = createTransport();
    ASSERT_EQ(COMPONENT_ERROR, transport->destroyLink(8u, "LinkID_8"));
    ASSERT_EQ(COMPONENT_OK, transport->createLink(8u, "LinkID_8"));
    EXPECT_CALL(*mockLinks["LinkID_8"], shutdown());
    ASSERT_EQ(COMPONENT_OK, transport->destroyLink(8u, "LinkID_8"));
}

TEST_F(TestPluginCommsTwoSixStubTransport, should_create_encoding_params_for_fetch_action) {
    auto transport = createTransport();
    Action action{8675309, 42, "{\"linkId\":\"LinkID_1\",\"type\":\"fetch\"}"};
    auto params = transport->getActionParams(action);
    EXPECT_EQ(0u, params.size());
}

TEST_F(TestPluginCommsTwoSixStubTransport, should_create_encoding_params_for_post_action) {
    auto transport = createTransport();
    Action action{8675309, 42, "{\"linkId\":\"LinkID_1\",\"type\":\"post\"}"};
    auto params = transport->getActionParams(action);
    ASSERT_EQ(1u, params.size());
    auto param = params.front();
    EXPECT_EQ("LinkID_1", param.linkId);
    EXPECT_EQ("*/*", param.type);
    EXPECT_TRUE(param.encodePackage);
    // TODO validate json if anything of importance gets added
}

TEST_F(TestPluginCommsTwoSixStubTransport, should_enqueue_content_for_fetch_action) {
    auto transport = createTransport();
    Action action{8675309, 42, "{\"linkId\":\"LinkID_1\",\"type\":\"fetch\"}"};
    ASSERT_EQ(COMPONENT_OK, transport->enqueueContent({}, action, {}));
}

TEST_F(TestPluginCommsTwoSixStubTransport, should_enqueue_content_for_post_action) {
    auto transport = createTransport();
    ASSERT_EQ(COMPONENT_OK, transport->createLink(1u, "LinkID_1"));
    std::vector<uint8_t> content{0x31, 0x41, 0x59};
    EXPECT_CALL(*mockLinks["LinkID_1"], enqueueContent(42, content))
        .WillOnce(::testing::Return(COMPONENT_OK));
    Action action{8675309, 42, "{\"linkId\":\"LinkID_1\",\"type\":\"post\"}"};
    EncodingParameters encodeParams{"LinkID_1", "*/*", true, ""};
    ASSERT_EQ(COMPONENT_OK, transport->enqueueContent(encodeParams, action, {0x31, 0x41, 0x59}));
}

TEST_F(TestPluginCommsTwoSixStubTransport, should_dequeue_content_for_fetch_action) {
    auto transport = createTransport();
    Action action{8675309, 42, "{\"linkId\":\"LinkID_1\",\"type\":\"fetch\"}"};
    ASSERT_EQ(COMPONENT_OK, transport->dequeueContent(action));
}

TEST_F(TestPluginCommsTwoSixStubTransport, should_dequeue_content_for_post_action) {
    auto transport = createTransport();
    ASSERT_EQ(COMPONENT_OK, transport->createLink(1u, "LinkID_1"));
    EXPECT_CALL(*mockLinks["LinkID_1"], dequeueContent(42))
        .WillOnce(::testing::Return(COMPONENT_OK));
    Action action{8675309, 42, "{\"linkId\":\"LinkID_1\",\"type\":\"post\"}"};
    ASSERT_EQ(COMPONENT_OK, transport->dequeueContent(action));
}

TEST_F(TestPluginCommsTwoSixStubTransport, should_perform_fetch_action) {
    auto transport = createTransport();
    ASSERT_EQ(COMPONENT_OK, transport->createLink(2u, "LinkID_2"));
    EXPECT_CALL(*mockLinks["LinkID_2"], fetch()).WillOnce(::testing::Return(COMPONENT_OK));
    Action action{8675309, 42, "{\"linkId\":\"LinkID_2\",\"type\":\"fetch\"}"};
    ASSERT_EQ(COMPONENT_OK, transport->doAction({7}, action));
}

TEST_F(TestPluginCommsTwoSixStubTransport, should_perform_post_action) {
    auto transport = createTransport();
    ASSERT_EQ(COMPONENT_OK, transport->createLink(2u, "LinkID_2"));
    EXPECT_CALL(*mockLinks["LinkID_2"], post(std::vector<RaceHandle>{7}, 42))
        .WillOnce(::testing::Return(COMPONENT_OK));
    Action action{8675309, 42, "{\"linkId\":\"LinkID_2\",\"type\":\"post\"}"};
    ASSERT_EQ(COMPONENT_OK, transport->doAction({7}, action));
}