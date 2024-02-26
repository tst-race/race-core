
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

#include "LinkManager.h"
#include "MockPluginNM.h"
#include "gtest/gtest.h"
#include "race/mocks/MockRaceSdkNM.h"

using ::testing::_;
using ::testing::Return;

class LinkManagerTestFixture : public ::testing::Test {
public:
    void setupGenesisProfiles(const std::string &linkProfilesJsonContents) {
        EXPECT_CALL(sdk, readFile("link-profiles.json"))
            .WillOnce(Return(std::vector<std::uint8_t>{linkProfilesJsonContents.begin(),
                                                       linkProfilesJsonContents.end()}));
    }

    void expectWriteProfiles(const std::string &linkProfilesJsonContents) {
        EXPECT_CALL(sdk, writeFile("link-profiles.json",
                                   std::vector<std::uint8_t>{linkProfilesJsonContents.begin(),
                                                             linkProfilesJsonContents.end()}))
            .WillOnce(Return(SDK_OK));
    }

    void setupAndActivateChannel(const std::string &channelGid) {
        ChannelProperties props;
        props.channelGid = channelGid;
        props.channelStatus = CHANNEL_ENABLED;

        EXPECT_CALL(sdk, getAllChannelProperties())
            .WillOnce(Return(std::vector<ChannelProperties>{props}));

        EXPECT_CALL(sdk, activateChannel(channelGid, "initRole", _));

        linkManager.init({{channelGid, "initRole"}});
        linkManager.onChannelStatusChanged(0, channelGid, ChannelStatus::CHANNEL_AVAILABLE);
    }

public:
    MockRaceSdkNM sdk;
    MockPluginNM plugin{sdk};
    LinkManager linkManager{&plugin};
};

TEST_F(LinkManagerTestFixture, init_creator_links) {
    SdkResponse response1;
    response1.status = SDK_OK;
    response1.handle = 314159;
    EXPECT_CALL(sdk, createLinkFromAddress("create", "create address 1",
                                           std::vector<std::string>{"persona1"}, 0))
        .WillOnce(Return(response1));

    SdkResponse response2;
    response2.status = SDK_OK;
    response2.handle = 8675309;
    EXPECT_CALL(sdk, createLinkFromAddress("create", "create address 2",
                                           std::vector<std::string>{"persona2", "persona3"}, 0))
        .WillOnce(Return(response2));

    setupGenesisProfiles(R"({
        "create": [
            {
                "address": "create address 1",
                "description": "genesis create unicast",
                "personas": [
                    "persona1"
                ],
                "role": "creator"
            },
            {
                "address": "create address 2",
                "description": "genesis create multicast",
                "personas": [
                    "persona2",
                    "persona3"
                ],
                "role": "creator"
            }
        ]
    })");
    setupAndActivateChannel("create");

    linkManager.onLinkStatusChanged(8675309, "LinkID1", LINK_CREATED, {});
    EXPECT_CALL(plugin, onStaticLinksCreated());
    linkManager.onLinkStatusChanged(314159, "LinkID2", LINK_CREATED, {});

    LinkProperties destroyedProps;
    destroyedProps.channelGid = "create";

    expectWriteProfiles(R"({
    "create": [
        {
            "address": "create address 2",
            "description": "genesis create multicast",
            "personas": [
                "persona2",
                "persona3"
            ],
            "role": "creator"
        }
    ]
})");
    linkManager.onLinkStatusChanged(0, "LinkID2", LINK_DESTROYED, destroyedProps);

    expectWriteProfiles(R"({
    "create": []
})");
    linkManager.onLinkStatusChanged(0, "LinkID1", LINK_DESTROYED, destroyedProps);
}

TEST_F(LinkManagerTestFixture, init_load_single_address_links) {
    SdkResponse response1;
    response1.status = SDK_OK;
    response1.handle = 314159;
    EXPECT_CALL(sdk, loadLinkAddress("loadSingle", "load address 1",
                                     std::vector<std::string>{"persona1"}, 0))
        .WillOnce(Return(response1));

    SdkResponse response2;
    response2.status = SDK_OK;
    response2.handle = 8675309;
    EXPECT_CALL(sdk, loadLinkAddress("loadSingle", "load address 2",
                                     std::vector<std::string>{"persona2", "persona3"}, 0))
        .WillOnce(Return(response2));

    setupGenesisProfiles(R"({
        "loadSingle": [
            {
                "address": "load address 1",
                "description": "genesis load unicast",
                "personas": [
                    "persona1"
                ],
                "role": "loader"
            },
            {
                "address": "load address 2",
                "description": "genesis load multicast",
                "personas": [
                    "persona2",
                    "persona3"
                ],
                "role": "loader"
            }
        ]
    })");
    setupAndActivateChannel("loadSingle");

    linkManager.onLinkStatusChanged(8675309, "LinkID1", LINK_LOADED, {});
    EXPECT_CALL(plugin, onStaticLinksCreated());
    linkManager.onLinkStatusChanged(314159, "LinkID2", LINK_LOADED, {});

    LinkProperties destroyedProps;
    destroyedProps.channelGid = "loadSingle";

    expectWriteProfiles(R"({
    "loadSingle": [
        {
            "address": "load address 2",
            "description": "genesis load multicast",
            "personas": [
                "persona2",
                "persona3"
            ],
            "role": "loader"
        }
    ]
})");
    linkManager.onLinkStatusChanged(0, "LinkID2", LINK_DESTROYED, destroyedProps);

    expectWriteProfiles(R"({
    "loadSingle": []
})");
    linkManager.onLinkStatusChanged(0, "LinkID1", LINK_DESTROYED, destroyedProps);
}

TEST_F(LinkManagerTestFixture, init_load_multi_address_links) {
    SdkResponse response;
    response.status = SDK_OK;
    response.handle = 8675309;
    EXPECT_CALL(sdk, loadLinkAddresses("loadMulti",
                                       std::vector<std::string>{"load address 1", "load address 2"},
                                       std::vector<std::string>{"persona1", "persona2"}, 0))
        .WillOnce(Return(response));

    setupGenesisProfiles(R"({
        "loadMulti": [
            {
                "address_list": [
                    "load address 1",
                    "load address 2"
                ],
                "description": "genesis load multicast",
                "personas": [
                    "persona1",
                    "persona2"
                ],
                "role": "loader"
            }
        ]
    })");
    setupAndActivateChannel("loadMulti");

    EXPECT_CALL(plugin, onStaticLinksCreated());
    linkManager.onLinkStatusChanged(8675309, "LinkID1", LINK_LOADED, {});

    LinkProperties destroyedProps;
    destroyedProps.channelGid = "loadMulti";

    expectWriteProfiles(R"({
    "loadMulti": []
})");
    linkManager.onLinkStatusChanged(0, "LinkID1", LINK_DESTROYED, destroyedProps);
}

TEST_F(LinkManagerTestFixture, create_dynamic_link) {
    SdkResponse response1;
    response1.status = SDK_OK;
    response1.handle = 314159;
    EXPECT_CALL(sdk, createLink("create", std::vector<std::string>{"persona1"}, 0))
        .WillOnce(Return(response1));

    SdkResponse response2;
    response2.status = SDK_OK;
    response2.handle = 8675309;
    EXPECT_CALL(sdk, createLink("create", std::vector<std::string>{"persona2", "persona3"}, 0))
        .WillOnce(Return(response2));

    EXPECT_EQ(SDK_OK, linkManager.createLink("create", {"persona1"}).status);
    EXPECT_EQ(SDK_OK, linkManager.createLink("create", {"persona2", "persona3"}).status);

    expectWriteProfiles(R"({
    "create": [
        {
            "address": "created address 1",
            "description": "",
            "personas": [
                "persona1"
            ],
            "role": "creator"
        }
    ]
})");

    LinkProperties props1;
    props1.channelGid = "create";
    props1.linkAddress = "created address 1";

    linkManager.onLinkStatusChanged(314159, "LinkID1", LINK_CREATED, props1);

    expectWriteProfiles(R"({
    "create": [
        {
            "address": "created address 2",
            "description": "",
            "personas": [
                "persona2",
                "persona3"
            ],
            "role": "creator"
        },
        {
            "address": "created address 1",
            "description": "",
            "personas": [
                "persona1"
            ],
            "role": "creator"
        }
    ]
})");

    LinkProperties props2;
    props2.channelGid = "create";
    props2.linkAddress = "created address 2";

    linkManager.onLinkStatusChanged(8675309, "LinkID2", LINK_CREATED, props2);

    LinkProperties destroyedProps;
    destroyedProps.channelGid = "create";

    expectWriteProfiles(R"({
    "create": [
        {
            "address": "created address 2",
            "description": "",
            "personas": [
                "persona2",
                "persona3"
            ],
            "role": "creator"
        }
    ]
})");
    linkManager.onLinkStatusChanged(0, "LinkID1", LINK_DESTROYED, destroyedProps);

    expectWriteProfiles(R"({
    "create": []
})");
    linkManager.onLinkStatusChanged(0, "LinkID2", LINK_DESTROYED, destroyedProps);
}

TEST_F(LinkManagerTestFixture, create_dynamic_link_then_add_personas) {
    SdkResponse response;
    response.status = SDK_OK;
    response.handle = 314159;
    EXPECT_CALL(sdk, createLink("create", std::vector<std::string>{}, 0))
        .WillOnce(Return(response));

    EXPECT_EQ(SDK_OK, linkManager.createLink("create", {}).status);

    expectWriteProfiles(R"({
    "create": [
        {
            "address": "created address 1",
            "description": "",
            "personas": [],
            "role": "creator"
        }
    ]
})");

    LinkProperties props1;
    props1.channelGid = "create";
    props1.linkAddress = "created address 1";

    linkManager.onLinkStatusChanged(314159, "LinkID1", LINK_CREATED, props1);

    EXPECT_CALL(sdk, getLinkProperties("LinkID1")).WillOnce(Return(props1));
    EXPECT_CALL(sdk, setPersonasForLink("LinkID1", std::vector<std::string>{"persona4"}))
        .WillOnce(Return(SDK_OK));

    expectWriteProfiles(R"({
    "create": [
        {
            "address": "created address 1",
            "description": "",
            "personas": [
                "persona4"
            ],
            "role": "creator"
        }
    ]
})");

    EXPECT_EQ(SDK_OK, linkManager.setPersonasForLink("LinkID1", {"persona4"}).status);
}

TEST_F(LinkManagerTestFixture, load_dynamic_link_single_address) {
    SdkResponse response1;
    response1.status = SDK_OK;
    response1.handle = 314159;
    EXPECT_CALL(
        sdk, loadLinkAddress("load", "loaded address 1", std::vector<std::string>{"persona1"}, 0))
        .WillOnce(Return(response1));

    SdkResponse response2;
    response2.status = SDK_OK;
    response2.handle = 8675309;
    EXPECT_CALL(sdk, loadLinkAddress("load", "loaded address 2",
                                     std::vector<std::string>{"persona2", "persona3"}, 0))
        .WillOnce(Return(response2));

    expectWriteProfiles(R"({
    "load": [
        {
            "address": "loaded address 1",
            "description": "",
            "personas": [
                "persona1"
            ],
            "role": "loader"
        }
    ]
})");

    EXPECT_EQ(SDK_OK, linkManager.loadLinkAddress("load", "loaded address 1", {"persona1"}).status);
    linkManager.onLinkStatusChanged(314159, "LinkID1", LINK_LOADED, {});

    expectWriteProfiles(R"({
    "load": [
        {
            "address": "loaded address 2",
            "description": "",
            "personas": [
                "persona2",
                "persona3"
            ],
            "role": "loader"
        },
        {
            "address": "loaded address 1",
            "description": "",
            "personas": [
                "persona1"
            ],
            "role": "loader"
        }
    ]
})");

    EXPECT_EQ(
        SDK_OK,
        linkManager.loadLinkAddress("load", "loaded address 2", {"persona2", "persona3"}).status);
    linkManager.onLinkStatusChanged(8675309, "LinkID2", LINK_LOADED, {});

    LinkProperties destroyedProps;
    destroyedProps.channelGid = "load";

    expectWriteProfiles(R"({
    "load": [
        {
            "address": "loaded address 2",
            "description": "",
            "personas": [
                "persona2",
                "persona3"
            ],
            "role": "loader"
        }
    ]
})");
    linkManager.onLinkStatusChanged(0, "LinkID1", LINK_DESTROYED, destroyedProps);

    expectWriteProfiles(R"({
    "load": []
})");
    linkManager.onLinkStatusChanged(0, "LinkID2", LINK_DESTROYED, destroyedProps);
}

TEST_F(LinkManagerTestFixture, load_dynamic_link_multi_address) {
    SdkResponse response;
    response.status = SDK_OK;
    response.handle = 314159;
    EXPECT_CALL(sdk, loadLinkAddresses(
                         "load", std::vector<std::string>{"loaded address 1", "loaded address 2"},
                         std::vector<std::string>{"persona1", "persona2"}, 0))
        .WillOnce(Return(response));

    expectWriteProfiles(R"({
    "load": [
        {
            "address_list": [
                "loaded address 1",
                "loaded address 2"
            ],
            "description": "",
            "personas": [
                "persona1",
                "persona2"
            ],
            "role": "loader"
        }
    ]
})");

    EXPECT_EQ(SDK_OK, linkManager
                          .loadLinkAddresses("load", {"loaded address 1", "loaded address 2"},
                                             {"persona1", "persona2"})
                          .status);
    linkManager.onLinkStatusChanged(314159, "LinkID1", LINK_LOADED, {});

    LinkProperties destroyedProps;
    destroyedProps.channelGid = "load";

    expectWriteProfiles(R"({
    "load": []
})");
    linkManager.onLinkStatusChanged(0, "LinkID1", LINK_DESTROYED, destroyedProps);
}
