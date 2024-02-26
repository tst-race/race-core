
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

#pragma once

#include "../../source/decomposed-comms/ComponentWrappers.h"
#include "../../source/helper.h"
#include "LogExpect.h"
#include "MockComponentPlugin.h"
#include "MockRaceSdkComms.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockTransportComponentWrapper : public TransportComponentWrapper {
public:
    MockTransportComponentWrapper(LogExpect &logger) :
        TransportComponentWrapper("", "", nullptr, nullptr), logger(logger) {
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
            .WillByDefault([this](CMTypes::LinkSdkHandle handle, const LinkID &linkId) {
                LOG_EXPECT(this->logger, "createLink", handle, linkId);
            });
        ON_CALL(*this, loadLinkAddress(_, _, _))
            .WillByDefault([this](CMTypes::LinkSdkHandle handle, const LinkID &linkId,
                                  const std::string &linkAddress) {
                LOG_EXPECT(this->logger, "loadLinkAddress", handle, linkId, linkAddress);
            });
        ON_CALL(*this, loadLinkAddresses(_, _, _))
            .WillByDefault([this](CMTypes::LinkSdkHandle handle, const LinkID &linkId,
                                  const std::vector<std::string> &linkAddress) {
                nlohmann::json linkAddressesJson = linkAddress;
                LOG_EXPECT(this->logger, "loadLinkAddresses", handle, linkId, linkAddressesJson);
            });
        ON_CALL(*this, createLinkFromAddress(_, _, _))
            .WillByDefault([this](CMTypes::LinkSdkHandle handle, const LinkID &linkId,
                                  const std::string &linkAddress) {
                LOG_EXPECT(this->logger, "createLinkFromAddress", handle, linkId, linkAddress);
            });
        ON_CALL(*this, destroyLink(_, _))
            .WillByDefault([this](CMTypes::LinkSdkHandle handle, const LinkID &linkId) {
                LOG_EXPECT(this->logger, "destroyLink", handle, linkId);
            });
        ON_CALL(*this, getActionParams(_)).WillByDefault([this](const Action &action) {
            LOG_EXPECT(this->logger, "getActionParams", action);
            return std::vector<EncodingParameters>{};
        });
        ON_CALL(*this, enqueueContent(_, _, _))
            .WillByDefault([this](const EncodingParameters &params, const Action &action,
                                  const std::vector<uint8_t> &content) {
                LOG_EXPECT(this->logger, "enqueueContent", params, action, content.size());
            });
        ON_CALL(*this, dequeueContent(_)).WillByDefault([this](const Action &action) {
            LOG_EXPECT(this->logger, "dequeueContent", action);
        });
        ON_CALL(*this, doAction(_, _))
            .WillByDefault([this](const std::vector<CMTypes::PackageFragmentHandle> &handles,
                                  const Action &action) {
                std::vector<RaceHandle> raceHandles(handles.size());
                std::transform(handles.begin(), handles.end(), raceHandles.begin(),
                               [](const auto &handle) { return handle.handle; });
                nlohmann::json handlesJson = raceHandles;
                LOG_EXPECT(this->logger, "doAction", handlesJson, action);
            });
    }

    MOCK_METHOD(TransportProperties, getTransportProperties, (), (override));

    MOCK_METHOD(LinkProperties, getLinkProperties, (const LinkID &linkId), (override));

    MOCK_METHOD(void, createLink, (CMTypes::LinkSdkHandle handle, const LinkID &linkId),
                (override));
    MOCK_METHOD(void, loadLinkAddress,
                (CMTypes::LinkSdkHandle handle, const LinkID &linkId,
                 const std::string &linkAddress),
                (override));
    MOCK_METHOD(void, loadLinkAddresses,
                (CMTypes::LinkSdkHandle handle, const LinkID &linkId,
                 const std::vector<std::string> &linkAddress),
                (override));
    MOCK_METHOD(void, createLinkFromAddress,
                (CMTypes::LinkSdkHandle handle, const LinkID &linkId,
                 const std::string &linkAddress),
                (override));
    MOCK_METHOD(void, destroyLink, (CMTypes::LinkSdkHandle handle, const LinkID &linkId),
                (override));

    MOCK_METHOD(std::vector<EncodingParameters>, getActionParams, (const Action &action),
                (override));

    MOCK_METHOD(void, enqueueContent,
                (const EncodingParameters &params, const Action &action,
                 const std::vector<uint8_t> &content),
                (override));

    MOCK_METHOD(void, dequeueContent, (const Action &action), (override));

    MOCK_METHOD(void, doAction,
                (const std::vector<CMTypes::PackageFragmentHandle> &handles, const Action &action),
                (override));

public:
    LogExpect &logger;
};

class MockUserModelComponentWrapper : public UserModelComponentWrapper {
public:
    MockUserModelComponentWrapper(LogExpect &logger) :
        UserModelComponentWrapper("", "", nullptr, nullptr), logger(logger) {
        using ::testing::_;
        ON_CALL(*this, getUserModelProperties()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getUserModelProperties");
            return UserModelProperties{};
        });
        ON_CALL(*this, addLink(_, _))
            .WillByDefault([this](const LinkID &link, const LinkParameters &params) {
                LOG_EXPECT(this->logger, "addLink", link, params);
            });
        ON_CALL(*this, removeLink(_)).WillByDefault([this](const LinkID &link) {
            LOG_EXPECT(this->logger, "removeLink", link);
        });
        ON_CALL(*this, getTimeline(_, _)).WillByDefault([this](Timestamp start, Timestamp end) {
            LOG_EXPECT(this->logger, "getTimeline", start, end);
            return ActionTimeline{};
        });
        ON_CALL(*this, onTransportEvent(_)).WillByDefault([this](const Event &event) {
            LOG_EXPECT(this->logger, "onTransportEvent", event);
        });
        ON_CALL(*this, onSendPackage(_, _)).WillByDefault([this](const LinkID &linkId, int bytes) {
            LOG_EXPECT(this->logger, "onSendPackage", linkId, bytes);
            return ActionTimeline{};
        });
    }

    MOCK_METHOD(UserModelProperties, getUserModelProperties, (), (override));

    MOCK_METHOD(void, addLink, (const LinkID &link, const LinkParameters &params), (override));

    MOCK_METHOD(void, removeLink, (const LinkID &link), (override));

    MOCK_METHOD(ActionTimeline, getTimeline, (Timestamp start, Timestamp end), (override));

    MOCK_METHOD(void, onTransportEvent, (const Event &event), (override));

    MOCK_METHOD(ActionTimeline, onSendPackage, (const LinkID &linkId, int bytes), (override));

public:
    LogExpect &logger;
};

class MockEncodingComponentWrapper : public EncodingComponentWrapper {
public:
    MockEncodingComponentWrapper(LogExpect &logger) :
        EncodingComponentWrapper("", "", nullptr, nullptr), logger(logger) {
        using ::testing::_;
        ON_CALL(*this, getEncodingProperties()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getEncodingProperties");
            return EncodingProperties{0, "application/octet-stream"};
        });
        ON_CALL(*this, getEncodingPropertiesForParameters(_))
            .WillByDefault([this](const EncodingParameters &params) {
                LOG_EXPECT(this->logger, "getEncodingPropertiesForParameters", params);
                return SpecificEncodingProperties({1000});
            });
        ON_CALL(*this, encodeBytes(_, _, _))
            .WillByDefault([this](CMTypes::EncodingHandle handle, const EncodingParameters &params,
                                  const std::vector<uint8_t> &bytes) {
                LOG_EXPECT(this->logger, "encodeBytes", handle, params, bytes.size());
            });
        ON_CALL(*this, decodeBytes(_, _, _))
            .WillByDefault([this](CMTypes::DecodingHandle handle, const EncodingParameters &params,
                                  const std::vector<uint8_t> &bytes) {
                LOG_EXPECT(this->logger, "decodeBytes", handle, params, bytes.size());
            });
    }

    MOCK_METHOD(EncodingProperties, getEncodingProperties, (), (override));

    MOCK_METHOD(SpecificEncodingProperties, getEncodingPropertiesForParameters,
                (const EncodingParameters &params), (override));

    MOCK_METHOD(void, encodeBytes,
                (CMTypes::EncodingHandle handle, const EncodingParameters &params,
                 const std::vector<uint8_t> &bytes),
                (override));

    MOCK_METHOD(void, decodeBytes,
                (CMTypes::DecodingHandle handle, const EncodingParameters &params,
                 const std::vector<uint8_t> &bytes),
                (override));

public:
    LogExpect &logger;
};