
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

#include "../../source/decomposed-comms/ComponentManager.h"
#include "../../source/helper.h"
#include "LogExpect.h"
#include "MockComponentPlugin.h"
#include "MockComponentWrappers.h"
#include "MockRaceSdkComms.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockComponentManagerInternal : public ComponentManagerInternal {
public:
    // This is cursed. I'm passing references sdk / componentPlugins to the base class, but they
    // aren't initialized until after the base class constructor is complete. This is why having an
    // interface that both the mock and the actual class implement makes more sense. Oh well.
    MockComponentManagerInternal(LogExpect &logger) :
        ComponentManagerInternal(nullptr, mockSdkComms, {}, transportPlugin, usermodelPlugin, {}),
        logger(logger),
        mockSdkComms(logger),
        transportPlugin("transport", logger),
        usermodelPlugin("usermodel", logger),
        encoding(logger),
        transport(logger),
        usermodel(logger),
        mockConnection("mockConnectionId", "mockLinkId"),
        mockLink("mockLinkId"),
        mockLink2("mockLinkId2") {
        using ::testing::_;

        mockLink.producerId = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        mockLink2.producerId = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

        ON_CALL(*this, teardown()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "teardown");
        });
        ON_CALL(*this, setup()).WillByDefault([this]() { LOG_EXPECT(this->logger, "setup"); });

        ON_CALL(*this, getState()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getState");
            return CMTypes::State::INITIALIZING;
        });
        ON_CALL(*this, getCompositionId).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getCompositionId");
            return "mockCompositionId";
        });

        ON_CALL(*this, encodingComponentFromEncodingParams(_))
            .WillByDefault([this](const EncodingParameters &params) {
                LOG_EXPECT(this->logger, "encodingComponentFromEncodingParams", params);
                return &encoding;
            });
        ON_CALL(*this, getTransport()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getTransport");
            return &transport;
        });
        ON_CALL(*this, getUserModel()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getUserModel");
            return &usermodel;
        });

        ON_CALL(*this, getLink(_)).WillByDefault([this](const LinkID &linkId) {
            LOG_EXPECT(this->logger, "getLink", linkId);
            if (linkId == "mockLinkId") {
                return &mockLink;
            } else if (linkId == "mockLinkId2") {
                return &mockLink2;
            } else {
                throw std::out_of_range("Invalid link Id");
            }
        });

        ON_CALL(*this, getLinks()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getLinks");
            return std::vector<CMTypes::Link *>{&mockLink, &mockLink2};
        });
        ON_CALL(*this, getConnection(_)).WillByDefault([this](const ConnectionID &connId) {
            LOG_EXPECT(this->logger, "getConnection", connId);
            return &mockConnection;
        });

        ON_CALL(*this, updatedActions()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "updatedActions");
        });
        ON_CALL(*this, encodeForAction(_)).WillByDefault([this](CMTypes::ActionInfo *info) {
            LOG_EXPECT(this->logger, "encodeForAction", *info)
        });
        ON_CALL(*this, getPackageHandlesForAction(_))
            .WillByDefault([this](CMTypes::ActionInfo *info) {
                LOG_EXPECT(this->logger, "getPackageHandlesForAction", *info)
                return std::vector<CMTypes::PackageFragmentHandle>{};
            });

        ON_CALL(*this, actionDone(_)).WillByDefault([this](CMTypes::ActionInfo *info) {
            LOG_EXPECT(this->logger, "actionDone", *info)
        });
    }

    // Comms Plugin APIs
    MOCK_METHOD(CMTypes::CmInternalStatus, init,
                (CMTypes::ComponentWrapperHandle postId, const PluginConfig &pluginConfig),
                (override));

    MOCK_METHOD(PluginResponse, shutdown, (CMTypes::ComponentWrapperHandle postId), (override));

    MOCK_METHOD(PluginResponse, sendPackage,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::PackageSdkHandle handle,
                 const ConnectionID &connectionId, EncPkg &&pkg, double timeoutTimestamp,
                 uint64_t batchId),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, openConnection,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::ConnectionSdkHandle handle,
                 LinkType linkType, const LinkID &linkId, const std::string &linkHints,
                 int32_t sendTimeout),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, closeConnection,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::ConnectionSdkHandle handle,
                 const ConnectionID &connectionId),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, destroyLink,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::LinkSdkHandle handle,
                 const LinkID &linkId),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, createLink,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::LinkSdkHandle handle,
                 const std::string &channelGid),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, loadLinkAddress,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::LinkSdkHandle handle,
                 const std::string &channelGid, const std::string &linkAddress),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, loadLinkAddresses,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::LinkSdkHandle handle,
                 const std::string &channelGid, const std::vector<std::string> &linkAddresses),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, createLinkFromAddress,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::LinkSdkHandle handle,
                 const std::string &channelGid, const std::string &linkAddress),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, deactivateChannel,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::ChannelSdkHandle handle,
                 const std::string &channelGid),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, activateChannel,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::ChannelSdkHandle handle,
                 const std::string &channelGid, const std::string &roleName),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, onUserInputReceived,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::UserSdkHandle handle,
                 bool answered, const std::string &response),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, onUserAcknowledgementReceived,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::UserSdkHandle handle),
                (override));

    // Common Apis
    MOCK_METHOD(CMTypes::CmInternalStatus, requestPluginUserInput,
                (CMTypes::ComponentWrapperHandle postId, const std::string &componentId,
                 const std::string &key, const std::string &prompt, bool cache),
                (override));
    MOCK_METHOD(CMTypes::CmInternalStatus, requestCommonUserInput,
                (CMTypes::ComponentWrapperHandle postId, const std::string &componentId,
                 const std::string &key),
                (override));
    MOCK_METHOD(CMTypes::CmInternalStatus, updateState,
                (CMTypes::ComponentWrapperHandle postId, const std::string &componentId,
                 ComponentState state),
                (override));

    // IEncodingSdk APIs
    MOCK_METHOD(CMTypes::CmInternalStatus, onBytesEncoded,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::EncodingHandle handle,
                 std::vector<uint8_t> &&bytes, EncodingStatus status),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, onBytesDecoded,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::DecodingHandle handle,
                 std::vector<uint8_t> &&bytes, EncodingStatus status),
                (override));

    // ITransportSdk APIs
    MOCK_METHOD(CMTypes::CmInternalStatus, onLinkStatusChanged,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::LinkSdkHandle handle,
                 const LinkID &linkId, LinkStatus status, const LinkParameters &params),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, onPackageStatusChanged,
                (CMTypes::ComponentWrapperHandle postId, CMTypes::PackageFragmentHandle handle,
                 PackageStatus status),
                (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, onEvent,
                (CMTypes::ComponentWrapperHandle postId, const Event &event), (override));

    MOCK_METHOD(CMTypes::CmInternalStatus, onReceive,
                (CMTypes::ComponentWrapperHandle postId, const LinkID &linkId,
                 const EncodingParameters &params, std::vector<uint8_t> &&bytes),
                (override));

    // IUserModelSdk APIs
    MOCK_METHOD(CMTypes::CmInternalStatus, onTimelineUpdated,
                (CMTypes::ComponentWrapperHandle postId), (override));

    // methods for sub-managers
    MOCK_METHOD(void, teardown, (), (override));
    MOCK_METHOD(void, setup, (), (override));

    MOCK_METHOD(CMTypes::State, getState, (), (override));
    MOCK_METHOD(const std::string &, getCompositionId, (), (override));

    MOCK_METHOD(EncodingComponentWrapper *, encodingComponentFromEncodingParams,
                (const EncodingParameters &params), (override));
    MOCK_METHOD(TransportComponentWrapper *, getTransport, (), (override));
    MOCK_METHOD(UserModelComponentWrapper *, getUserModel, (), (override));

    MOCK_METHOD(CMTypes::Link *, getLink, (const LinkID &linkId), (override));
    MOCK_METHOD(std::vector<CMTypes::Link *>, getLinks, (), (override));
    MOCK_METHOD(CMTypes::Connection *, getConnection, (const ConnectionID &connId), (override));

    MOCK_METHOD(void, updatedActions, (), (override));
    MOCK_METHOD(void, encodeForAction, (CMTypes::ActionInfo * info), (override));
    MOCK_METHOD(std::vector<CMTypes::PackageFragmentHandle>, getPackageHandlesForAction,
                (CMTypes::ActionInfo * info), (override));

    MOCK_METHOD(void, actionDone, (CMTypes::ActionInfo * info), (override));

public:
    LogExpect &logger;
    MockRaceSdkComms mockSdkComms;
    MockComponentPlugin transportPlugin;
    MockComponentPlugin usermodelPlugin;
    MockEncodingComponentWrapper encoding;
    MockTransportComponentWrapper transport;
    MockUserModelComponentWrapper usermodel;
    CMTypes::Connection mockConnection;
    CMTypes::Link mockLink;
    CMTypes::Link mockLink2;
};
