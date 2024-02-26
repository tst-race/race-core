
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

#ifndef __MOCK_RACE_LINKS__
#define __MOCK_RACE_LINKS__

#include "../../include/RaceLinks.h"
#include "LogExpect.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockRaceLinks : public RaceLinks {
public:
    MockRaceLinks(LogExpect &logger) : logger(logger) {
        using ::testing::_;
        ON_CALL(*this, addLink(_, _))
            .WillByDefault([this](const std::string &linkId, const personas::PersonaSet &personas) {
                json personasJson = personas;
                LOG_EXPECT(this->logger, "addLink", linkId, personasJson);
            });
        ON_CALL(*this, removeLink(_)).WillByDefault([this](const std::string &linkId) {
            LOG_EXPECT(this->logger, "removeLink", linkId);
        });
        ON_CALL(*this, completeNewLinkRequest(_, _))
            .WillByDefault([this](RaceHandle handle, const std::string &linkId) {
                LOG_EXPECT(this->logger, "completeNewLinkRequest", handle, linkId);
                return "";
            });
        ON_CALL(*this, addNewLinkRequest(_, _, _))
            .WillByDefault([this](RaceHandle handle, const personas::PersonaSet &personas,
                                  const std::string & /*linkAddress*/) {
                json personasJson = personas;
                LOG_EXPECT(this->logger, "addNewLinkRequest", handle, personasJson);
            });
        ON_CALL(*this, removeNewLinkRequest(_, _))
            .WillByDefault([this](RaceHandle handle, const LinkID &linkId) {
                LOG_EXPECT(this->logger, "removeNewLinkRequest", handle, linkId);
            });
        ON_CALL(*this, removeConnectionRequest(_)).WillByDefault([this](RaceHandle handle) {
            LOG_EXPECT(this->logger, "removeConnectionRequest", handle);
        });
        ON_CALL(*this, removeConnection(_)).WillByDefault([this](const ConnectionID &connId) {
            LOG_EXPECT(this->logger, "removeConnection", connId);
        });
        ON_CALL(*this, addConnectionRequest(_, _))
            .WillByDefault([this](RaceHandle handle, const LinkID &linkId) {
                LOG_EXPECT(this->logger, "addConnectionRequest", handle, linkId);
            });
        ON_CALL(*this, addConnection(_, _))
            .WillByDefault([this](RaceHandle handle, const ConnectionID &connId) {
                LOG_EXPECT(this->logger, "addConnection", handle, connId);
            });
        ON_CALL(*this, doesConnectionExist(_)).WillByDefault([this](const ConnectionID &connId) {
            LOG_EXPECT(this->logger, "doesConnectionExist", connId);
            return true;
        });
        ON_CALL(*this, doConnectionsExist(_))
            .WillByDefault([this](const std::unordered_set<ConnectionID> &connectionIds) {
                json connectionIdsJson = connectionIds;
                LOG_EXPECT(this->logger, "doConnectionsExist", connectionIdsJson);
                return personas::PersonaSet{};
            });
        ON_CALL(*this, updateLinkProperties(_, _))
            .WillByDefault([this](const LinkID &linkId, const LinkProperties &properties) {
                LOG_EXPECT(this->logger, "updateLinkProperties", linkId, properties);
            });
        ON_CALL(*this, getLinkProperties(_)).WillByDefault([this](const LinkID &linkId) {
            LOG_EXPECT(this->logger, "getLinkProperties", linkId);
            return LinkProperties{};
        });
        ON_CALL(*this, getAllPersonaSet()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getAllPersonaSet");
            return personas::PersonaSet{};
        });
        ON_CALL(*this, doesLinkIncludeGivenPersonas(_, _))
            .WillByDefault([this](const personas::PersonaSet &connectionProfilePersonas,
                                  const personas::PersonaSet &givenPersonas) {
                json connectionProfilePersonasJson = connectionProfilePersonas;
                json givenPersonasJson = givenPersonas;
                LOG_EXPECT(this->logger, "doesLinkIncludeGivenPersonas",
                           connectionProfilePersonasJson, givenPersonasJson);
                return true;
            });
        ON_CALL(*this, getAllLinksForPersonas(_, _))
            .WillByDefault([this](const personas::PersonaSet &personas, const LinkType linkType) {
                json personasJson = personas;
                LOG_EXPECT(this->logger, "getAllLinksForPersonas", personasJson, linkType);
                return std::vector<std::string>{};
            });
        ON_CALL(*this, setPersonasForLink(_, _))
            .WillByDefault([this](const std::string &linkId, const personas::PersonaSet &personas) {
                json personasJson = personas;
                LOG_EXPECT(this->logger, "setPersonasForLink", linkId, personasJson);
                return true;
            });
        ON_CALL(*this, getAllPersonasForLink(_)).WillByDefault([this](const LinkID &linkId) {
            LOG_EXPECT(this->logger, "getAllPersonasForLink", linkId);
            return personas::PersonaSet{};
        });
        ON_CALL(*this, getLinkForConnection(_)).WillByDefault([this](const ConnectionID &connId) {
            LOG_EXPECT(this->logger, "getLinkForConnection", connId);
            return LinkID{};
        });
        ON_CALL(*this, addTraceCtxForLink(_, _, _))
            .WillByDefault(
                [this](const LinkID &linkId, std::uint64_t traceId, std::uint64_t spanId) {
                    LOG_EXPECT(this->logger, "addTraceCtxForLink", linkId, traceId, spanId);
                });
        ON_CALL(*this, getTraceCtxForLink(_)).WillByDefault([this](const LinkID &linkId) {
            LOG_EXPECT(this->logger, "getTraceCtxForLink", linkId);
            return std::pair<uint64_t, uint64_t>{};
        });
        ON_CALL(*this, addTraceCtxForConnection(_, _, _))
            .WillByDefault(
                [this](const ConnectionID &connId, std::uint64_t traceId, std::uint64_t spanId) {
                    LOG_EXPECT(this->logger, "addTraceCtxForConnection", connId, traceId, spanId);
                });
        ON_CALL(*this, getTraceCtxForConnection(_))
            .WillByDefault([this](const ConnectionID &connId) {
                LOG_EXPECT(this->logger, "getTraceCtxForConnection", connId);
                return std::pair<uint64_t, uint64_t>{};
            });
    }

    MOCK_METHOD(void, addLink, (const std::string &linkId, const personas::PersonaSet &personas),
                (override));
    MOCK_METHOD(void, removeLink, (const std::string &linkId), (override));
    MOCK_METHOD(const std::string, completeNewLinkRequest,
                (RaceHandle handle, const std::string &linkId), (override));
    MOCK_METHOD(void, addNewLinkRequest,
                (RaceHandle handle, const personas::PersonaSet &personas,
                 const std::string &linkAddress),
                (override));
    MOCK_METHOD(void, removeNewLinkRequest, (RaceHandle handle, const LinkID &linkId), (override));
    MOCK_METHOD(void, removeConnectionRequest, (RaceHandle handle), (override));
    MOCK_METHOD(void, removeConnection, (const ConnectionID &connId), (override));
    MOCK_METHOD(void, addConnectionRequest, (RaceHandle handle, const LinkID &linkId), (override));
    MOCK_METHOD(void, addConnection, (RaceHandle handle, const ConnectionID &connId), (override));
    MOCK_METHOD(bool, doesConnectionExist, (const ConnectionID &connId), (override));
    MOCK_METHOD(std::unordered_set<ConnectionID>, doConnectionsExist,
                (const std::unordered_set<ConnectionID> &connectionIds), (override));
    MOCK_METHOD(void, updateLinkProperties,
                (const LinkID &linkId, const LinkProperties &properties), (override));
    MOCK_METHOD(LinkProperties, getLinkProperties, (const LinkID &linkId), (override));
    MOCK_METHOD(personas::PersonaSet, getAllPersonaSet, (), (override));
    MOCK_METHOD(bool, doesLinkIncludeGivenPersonas,
                (const personas::PersonaSet &connectionProfilePersonas,
                 const personas::PersonaSet &givenPersonas),
                (override));
    MOCK_METHOD(std::vector<LinkID>, getAllLinksForPersonas,
                (const personas::PersonaSet &personas, const LinkType linkType), (override));
    MOCK_METHOD(bool, setPersonasForLink,
                (const std::string &linkId, const personas::PersonaSet &personas), (override));
    MOCK_METHOD(personas::PersonaSet, getAllPersonasForLink, (const LinkID &linkId), (override));
    MOCK_METHOD(LinkID, getLinkForConnection, (const ConnectionID &connId), (override));
    MOCK_METHOD(void, addTraceCtxForLink,
                (const LinkID &linkId, std::uint64_t traceId, std::uint64_t spanId), (override));
    MOCK_METHOD((std::pair<std::uint64_t, std::uint64_t>), getTraceCtxForLink,
                (const LinkID &linkId), (override));
    MOCK_METHOD(void, addTraceCtxForConnection,
                (const ConnectionID &connId, std::uint64_t traceId, std::uint64_t spanId),
                (override));
    MOCK_METHOD((std::pair<std::uint64_t, std::uint64_t>), getTraceCtxForConnection,
                (const ConnectionID &connId), (override));

    LogExpect &logger;
};

#endif
