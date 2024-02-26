
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

#include "../../../include/RaceLinks.h"
#include "../../common/race_printers.h"
#include "gtest/gtest.h"

TEST(RaceLinks, getAllReachablePersonas_returns_empty_when_no_personas_added) {
    RaceLinks links;

    const personas::PersonaSet personas = links.getAllPersonaSet();
    EXPECT_EQ(personas.size(), 0);
}

inline LinkID addLink(RaceLinks &links, const personas::PersonaSet &personas,
                      const LinkType linkType = LT_UNDEF) {
    static int count = 0;
    const LinkID linkId = std::to_string(count++);
    links.addLink(linkId, personas);
    if (linkType != LT_UNDEF) {
        LinkProperties props;
        props.linkType = linkType;
        links.updateLinkProperties(linkId, props);
    }
    return linkId;
}

TEST(RaceLinks, getAllPersonaSet_returns_added_personsas) {
    RaceLinks links;

    addLink(links, {"1", "2", "3"});
    addLink(links, {"1", "2", "3"});
    addLink(links, {"4", "5", "6"});
    addLink(links, {"3", "4", "5"});

    const personas::PersonaSet personas = links.getAllPersonaSet();
    EXPECT_EQ(personas.size(), 6);
    EXPECT_TRUE(std::find(personas.begin(), personas.end(), "1") != personas.end());
    EXPECT_TRUE(std::find(personas.begin(), personas.end(), "2") != personas.end());
    EXPECT_TRUE(std::find(personas.begin(), personas.end(), "3") != personas.end());
    EXPECT_TRUE(std::find(personas.begin(), personas.end(), "4") != personas.end());
    EXPECT_TRUE(std::find(personas.begin(), personas.end(), "5") != personas.end());
    EXPECT_TRUE(std::find(personas.begin(), personas.end(), "6") != personas.end());
}

TEST(RaceLinks, getAllLinksForPersona_returns_empty_when_no_personas_added) {
    RaceLinks links;

    const personas::PersonaSet personas = {"some persona 1"};
    const LinkType linkType = LT_UNDEF;

    const std::vector<LinkID> linkProfiles = links.getAllLinksForPersonas(personas, linkType);

    EXPECT_EQ(linkProfiles.size(), 0);
}

TEST(RaceLinks, getAllLinksForPersona_returns_link_ids_for_personas) {
    RaceLinks links;

    const auto linkId0 = addLink(links, {"1", "2", "3"}, LT_SEND);
    const auto linkId1 = addLink(links, {"1", "2", "3"}, LT_SEND);
    addLink(links, {"4", "5", "6"}, LT_SEND);
    const auto linkId3 = addLink(links, {"3", "2", "5"}, LT_SEND);

    const personas::PersonaSet personas = {"2", "3"};
    const LinkType linkType = LT_SEND;

    const std::vector<LinkID> linkProfiles = links.getAllLinksForPersonas(personas, linkType);

    EXPECT_EQ(linkProfiles.size(), 3);
    EXPECT_TRUE(std::find(linkProfiles.begin(), linkProfiles.end(), linkId0) != linkProfiles.end());
    EXPECT_TRUE(std::find(linkProfiles.begin(), linkProfiles.end(), linkId1) != linkProfiles.end());
    EXPECT_TRUE(std::find(linkProfiles.begin(), linkProfiles.end(), linkId3) != linkProfiles.end());
}

TEST(RaceLinks, getAllLinksForPersona_returns_all_links_for_empty_personas) {
    RaceLinks links;

    addLink(links, {"1", "2", "3"}, LT_SEND);
    addLink(links, {"1", "2", "3"}, LT_SEND);
    addLink(links, {"4", "5", "6"}, LT_SEND);
    addLink(links, {"3", "2", "5"}, LT_SEND);

    const personas::PersonaSet personas = {};
    const LinkType linkType = LT_SEND;

    const std::vector<LinkID> linkProfiles = links.getAllLinksForPersonas(personas, linkType);

    EXPECT_EQ(linkProfiles.size(), 4);
}

TEST(RaceLinks, updateLinkProperties_throws_if_link_does_not_exist) {
    RaceLinks links;
    const LinkID linkId = "Test/some non-existant link ID";
    LinkProperties props;
    props.linkType = LT_BIDI;

    ASSERT_THROW(links.updateLinkProperties(linkId, props), std::out_of_range);
}

TEST(RaceLinks, updateLinkProperties_throws_if_link_type_is_invalid) {
    RaceLinks links;
    const LinkID linkId = "Test/some non-existant link ID";
    LinkProperties props;
    props.linkType = static_cast<LinkType>(123);

    ASSERT_THROW(links.updateLinkProperties(linkId, props), std::invalid_argument);
}

TEST(RaceLinks, updateLinkProperties_adds_props_for_a_link) {
    RaceLinks links;
    LinkProperties props;
    props.linkType = LT_BIDI;

    const LinkID linkId = "LinkId-0";
    links.addLink(linkId, {"1"});
    links.updateLinkProperties(linkId, props);

    const LinkProperties result = links.getLinkProperties(linkId);

    EXPECT_EQ(result.linkType, props.linkType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// addConnection
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(RaceLinks, addConnection) {
    RaceLinks links;
    const LinkID linkId = "LinkId-0";
    links.addLink(linkId, {"some fake persona"});
    ConnectionID connectionId = "pluginID/channelGID/LinkId-0/connection_1234";
    links.addConnectionRequest(0, linkId);
    links.addConnection(0, connectionId);
}

TEST(RaceLinks, addConnection_throw_if_link_does_not_exist) {
    RaceLinks links;
    const LinkID linkId = "fake link ID";
    ConnectionID connectionId = "pluginID/channelGID/LinkId-0/connection_1234";
    links.addConnectionRequest(1, linkId);
    ASSERT_THROW(links.addConnection(1, connectionId), std::invalid_argument);
}

TEST(RaceLinks, addConnection_throw_if_connection_id_already_exists) {
    RaceLinks links;
    const LinkID linkId = "LinkId-0";
    links.addLink(linkId, {"some fake persona"});
    ConnectionID connectionId = "pluginID/channelGID/LinkId-0/connection_1234";
    links.addConnectionRequest(0, linkId);
    links.addConnection(0, connectionId);

    links.addConnectionRequest(1, linkId);
    ASSERT_THROW(links.addConnection(1, connectionId), std::invalid_argument);
}

TEST(RaceLinks, addConnection_throw_if_connection_id_already_exists_on_another_link) {
    RaceLinks links;
    const LinkID linkId = "LinkId-0";
    links.addLink(linkId, {"some fake persona"});
    ConnectionID connectionId = "pluginID/channelGID/LinkId-0/connection_1234";
    links.addConnectionRequest(0, linkId);
    links.addConnection(0, connectionId);

    const LinkID otherLinkId = "LinkId-1";
    links.addLink(otherLinkId, {"some fake persona"});
    ConnectionID otherConnectionId = "pluginID/channelGID/LinkId-0/connection_1234";
    links.addConnectionRequest(1, otherLinkId);
    ASSERT_THROW(links.addConnection(1, otherConnectionId), std::invalid_argument);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// doesConnectionExist
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(RaceLinks, doesConnectionExist_returns_false_if_connection_does_not_exist) {
    RaceLinks links;
    const ConnectionID connectionId = "pluginID/channelGID/LinkId-0/connection_1234";
    EXPECT_FALSE(links.doesConnectionExist(connectionId));
}

TEST(RaceLinks, doesConnectionExist_returns_true_if_connection_does_exist) {
    RaceLinks links;
    const LinkID linkId = "LinkId-0";
    links.addLink(linkId, {"some fake persona"});
    const ConnectionID connectionId = "pluginID/channelGID/LinkId-0/connection_1234";
    links.addConnectionRequest(0, linkId);
    links.addConnection(0, connectionId);

    EXPECT_TRUE(links.doesConnectionExist(connectionId));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// removeConnection
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(RaceLinks, removeConnection_connection_does_not_exist) {
    RaceLinks links;
    const ConnectionID connectionId = "pluginid/channelgid/linkid/connection_1234";
    links.removeConnection(connectionId);
}

TEST(RaceLinks, removeConnection_removes_an_added_connection) {
    RaceLinks links;
    const LinkID linkId = "LinkId-0";
    links.addLink(linkId, {"some fake persona"});
    const ConnectionID connectionId = "pluginID/channelGID/LinkId-0/connection_1234";
    links.addConnectionRequest(1, linkId);
    links.addConnection(1, connectionId);

    links.removeConnection(connectionId);

    EXPECT_FALSE(links.doesConnectionExist(connectionId));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// removeLink
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(RaceLinks, removeLink_removes) {
    RaceLinks links;
    const LinkID linkId = "LinkId-0";
    links.addLink(linkId, {"some fake persona"});
    links.removeLink(linkId);
    ASSERT_THROW(links.getLinkProperties(linkId), std::out_of_range);
}

TEST(RaceLinks, removeLink_nonexistent_link) {
    RaceLinks links;
    const LinkID linkId = "LinkId-0";
    links.removeLink(linkId);
    ASSERT_THROW(links.getLinkProperties(linkId), std::out_of_range);
}

TEST(RaceLinks, removed_link_has_personas) {
    RaceLinks links;
    RaceHandle handle = 0;
    const LinkID linkId = "LinkId-0";
    links.addNewLinkRequest(handle, {"alice"}, "");
    links.completeNewLinkRequest(handle, linkId);
    EXPECT_EQ(links.getAllPersonasForLink(linkId), personas::PersonaSet({"alice"}));

    links.removeLink(linkId);
    EXPECT_EQ(links.getAllPersonasForLink(linkId), personas::PersonaSet({"alice"}));
}

TEST(RaceLinks, removeLink_removes_an_added_connection) {
    RaceLinks links;
    const LinkID linkId = "LinkId-0";
    links.addLink(linkId, {"some fake persona"});
    const ConnectionID connectionId = "pluginID/channelGID/LinkId-0/connection_1234";
    links.addConnectionRequest(1, linkId);
    links.addConnection(1, connectionId);

    links.removeLink(linkId);

    EXPECT_FALSE(links.doesConnectionExist(connectionId));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// New Link Requests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(RaceLinks, add_then_completeNewLinkRequest) {
    RaceLinks links;
    RaceHandle handle = 0;
    const LinkID linkId = "LinkId-0";
    links.addNewLinkRequest(handle, {"alice"}, "");
    links.completeNewLinkRequest(handle, linkId);
    EXPECT_EQ(links.getAllPersonasForLink(linkId), personas::PersonaSet({"alice"}));
}

TEST(RaceLinks, add_remove_then_completeNewLinkRequest) {
    RaceLinks links;
    RaceHandle handle = 0;
    const LinkID linkId = "LinkId-0";
    links.addNewLinkRequest(handle, {"alice"}, "");
    links.removeNewLinkRequest(handle, linkId);
    ASSERT_THROW(links.completeNewLinkRequest(handle, linkId), std::invalid_argument);
    EXPECT_EQ(links.getAllPersonasForLink(linkId), personas::PersonaSet({"alice"}));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// LinkAddress passed to addNewLinkRequest is returned by completeNewLinkRequest
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(RaceLinks, load_has_address) {
    RaceLinks links;
    RaceHandle handle = 0;
    const LinkID linkId = "LinkId-0";
    const std::string address = "address-1";
    links.addNewLinkRequest(handle, {"alice"}, address);
    EXPECT_EQ(links.completeNewLinkRequest(handle, linkId), address);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// cached package handles
////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(RaceLinks, cached_package_handles) {
    RaceLinks links;
    RaceHandle h1 = 1, h2 = 2, h3 = 3;
    ConnectionID c1 = "C1", c2 = "C2";
    LinkID l1 = "L1", l2 = "l2";

    links.addNewLinkRequest(h1, {}, l1);
    links.addNewLinkRequest(h2, {}, l2);
    links.addLink(l1, {});
    links.addLink(l2, {});
    links.addConnectionRequest(h1, l1);
    links.addConnectionRequest(h2, l2);
    links.addConnection(h1, c1);
    links.addConnection(h2, c2);

    auto conns = links.getLinkConnections(l1);
    EXPECT_EQ(*conns.begin(), c1);
    conns = links.getLinkConnections(l2);
    EXPECT_EQ(*conns.begin(), c2);

    links.cachePackageHandle(c1, h1);
    auto handles = links.getCachedPackageHandles(c1);
    EXPECT_EQ(handles.size(), 1);
    EXPECT_EQ(*handles.begin(), h1);

    handles = links.getCachedPackageHandles(c2);
    EXPECT_EQ(handles.size(), 0);

    links.removeCachedPackageHandle(h1);
    handles = links.getCachedPackageHandles(c1);
    EXPECT_EQ(handles.size(), 0);

    links.cachePackageHandle(c1, h1);
    links.cachePackageHandle(c1, h2);

    handles = links.getCachedPackageHandles(c1);
    EXPECT_EQ(handles.size(), 2);
    EXPECT_NE(handles.find(h1), handles.end());
    EXPECT_NE(handles.find(h2), handles.end());
    EXPECT_EQ(handles.find(h3), handles.end());

    links.removeCachedPackageHandle(h1);
    links.removeCachedPackageHandle(h2);
    handles = links.getCachedPackageHandles(c1);
    EXPECT_EQ(handles.size(), 0);

    links.cachePackageHandle(c1, h1);
    links.cachePackageHandle(c2, h2);
    handles = links.getCachedPackageHandles(c1);
    EXPECT_EQ(handles.size(), 1);
    EXPECT_EQ(*handles.begin(), h1);
    handles = links.getCachedPackageHandles(c2);
    EXPECT_EQ(handles.size(), 1);
    EXPECT_EQ(*handles.begin(), h2);

    links.removeCachedPackageHandle(h1);
    handles = links.getCachedPackageHandles(c1);
    EXPECT_EQ(handles.size(), 0);

    handles = links.getCachedPackageHandles(c2);
    EXPECT_EQ(handles.size(), 1);

    links.removeCachedPackageHandle(h2);
    handles = links.getCachedPackageHandles(c2);
    EXPECT_EQ(handles.size(), 0);
}
