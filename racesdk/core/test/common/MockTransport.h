
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

#ifndef __MOCK_TRANSPORT_H__
#define __MOCK_TRANSPORT_H__

#include "ITransportComponent.h"
#include "LogExpect.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockTransport : public ITransportComponent {
public:
    MockTransport(LogExpect &logger, ITransportSdk &sdk) : logger(logger), sdk(sdk) {
        using ::testing::_;
        ON_CALL(*this, getTransportProperties()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getTransportProperties");
            return TransportProperties{};
        });
        ON_CALL(*this, getLinkProperties(_)).WillByDefault([this](const LinkID &linkId) {
            LOG_EXPECT(this->logger, "getLinkProperties", linkId);
            return LinkProperties{};
        });
        ON_CALL(*this, createLink(_, _))
            .WillByDefault([this](RaceHandle handle, const LinkID &linkId) {
                LOG_EXPECT(this->logger, "createLink", handle, linkId);
                return ComponentStatus::COMPONENT_OK;
            });
        ON_CALL(*this, loadLinkAddress(_, _, _))
            .WillByDefault(
                [this](RaceHandle handle, const LinkID &linkId, const std::string &linkAddress) {
                    LOG_EXPECT(this->logger, "loadLinkAddress", handle, linkId, linkAddress);
                    return ComponentStatus::COMPONENT_OK;
                });
        ON_CALL(*this, loadLinkAddresses(_, _, _))
            .WillByDefault([this](RaceHandle handle, const LinkID &linkId,
                                  const std::vector<std::string> & /* linkAddress */) {
                LOG_EXPECT(this->logger, "loadLinkAddresses", handle, linkId);
                return ComponentStatus::COMPONENT_OK;
            });
        ON_CALL(*this, createLinkFromAddress(_, _, _))
            .WillByDefault(
                [this](RaceHandle handle, const LinkID &linkId, const std::string &linkAddress) {
                    LOG_EXPECT(this->logger, "createLinkFromAddress", handle, linkId, linkAddress);
                    return ComponentStatus::COMPONENT_OK;
                });
        ON_CALL(*this, destroyLink(_, _))
            .WillByDefault([this](RaceHandle handle, const LinkID &linkId) {
                LOG_EXPECT(this->logger, "destroyLink", handle, linkId);
                return ComponentStatus::COMPONENT_OK;
            });
        ON_CALL(*this, getActionParams(_)).WillByDefault([this](const Action &action) {
            LOG_EXPECT(this->logger, "getActionParams", action);
            return std::vector<EncodingParameters>{};
        });
        ON_CALL(*this, enqueueContent(_, _, _))
            .WillByDefault([this](const EncodingParameters &params, const Action &action,
                                  const std::vector<uint8_t> & /* content */) {
                LOG_EXPECT(this->logger, "enqueueContent", params, action);
                return ComponentStatus::COMPONENT_OK;
            });
        ON_CALL(*this, dequeueContent(_)).WillByDefault([this](const Action &action) {
            LOG_EXPECT(this->logger, "dequeueContent", action);
            return ComponentStatus::COMPONENT_OK;
        });
        ON_CALL(*this, doAction(_, _))
            .WillByDefault([this](const std::vector<RaceHandle> &handles, const Action &action) {
                nlohmann::json handlesJson = handles;
                LOG_EXPECT(this->logger, "doAction", handlesJson, action);
                return ComponentStatus::COMPONENT_OK;
            });
        ON_CALL(*this, onUserInputReceived(_, _, _))
            .WillByDefault([this](RaceHandle handle, bool answered, const std::string &response) {
                LOG_EXPECT(this->logger, "onUserInputReceived", handle, answered, response);
                return ComponentStatus::COMPONENT_OK;
            });
    }

    MOCK_METHOD(TransportProperties, getTransportProperties, (), (override));
    MOCK_METHOD(LinkProperties, getLinkProperties, (const LinkID &linkId), (override));
    MOCK_METHOD(ComponentStatus, createLink, (RaceHandle handle, const LinkID &linkId), (override));
    MOCK_METHOD(ComponentStatus, loadLinkAddress,
                (RaceHandle handle, const LinkID &linkId, const std::string &linkAddress),
                (override));
    MOCK_METHOD(ComponentStatus, loadLinkAddresses,
                (RaceHandle handle, const LinkID &linkId,
                 const std::vector<std::string> &linkAddress),
                (override));
    MOCK_METHOD(ComponentStatus, createLinkFromAddress,
                (RaceHandle handle, const LinkID &linkId, const std::string &linkAddress),
                (override));
    MOCK_METHOD(ComponentStatus, destroyLink, (RaceHandle handle, const LinkID &linkId),
                (override));
    MOCK_METHOD(std::vector<EncodingParameters>, getActionParams, (const Action &action),
                (override));
    MOCK_METHOD(ComponentStatus, enqueueContent,
                (const EncodingParameters &params, const Action &action,
                 const std::vector<uint8_t> &content),
                (override));
    MOCK_METHOD(ComponentStatus, dequeueContent, (const Action &action), (override));
    MOCK_METHOD(ComponentStatus, doAction,
                (const std::vector<RaceHandle> &handle, const Action &action), (override));
    MOCK_METHOD(ComponentStatus, onUserInputReceived,
                (RaceHandle handle, bool answered, const std::string &response), (override));

public:
    LogExpect &logger;
    ITransportSdk &sdk;
};

#endif
