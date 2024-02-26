
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

#ifndef __MOCK_RACE_CHANNELS__
#define __MOCK_RACE_CHANNELS__

#include "../../include/RaceChannels.h"
#include "LogExpect.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockRaceChannels : public RaceChannels {
public:
    MockRaceChannels(LogExpect &logger) : logger(logger) {
        using ::testing::_;
        ON_CALL(*this, getSupportedChannels()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getSupportedChannels");
            return std::map<std::string, ChannelProperties>{};
        });
        ON_CALL(*this, update(_, _, _))
            .WillByDefault([this](const std::string &channelGid, ChannelStatus status,
                                  const ChannelProperties &properties) {
                LOG_EXPECT(this->logger, "update", channelGid, status, properties);
                return true;
            });
        ON_CALL(*this, getChannelProperties(_))
            .WillByDefault([this](const std::string &channelGid) {
                LOG_EXPECT(this->logger, "getChannelProperties", channelGid);
                return ChannelProperties{};
            });
        ON_CALL(*this, getPluginsForChannel(_))
            .WillByDefault([this](const std::string &channelGid) {
                LOG_EXPECT(this->logger, "getPluginsForChannel", channelGid);
                return std::vector<std::string>{};
            });
        ON_CALL(*this, getWrapperIdForChannel(_))
            .WillByDefault([this](const std::string &channelGid) {
                LOG_EXPECT(this->logger, "getWrapperIdForChannel", channelGid);
                return std::string{};
            });
        ON_CALL(*this, isAvailable(_)).WillByDefault([this](const std::string &channelGid) {
            LOG_EXPECT(this->logger, "isAvailable", channelGid);
            return true;
        });
        ON_CALL(*this, update(_)).WillByDefault([this](const ChannelProperties &properties) {
            LOG_EXPECT(this->logger, "update", properties);
            return true;
        });
        ON_CALL(*this, getLinksForChannel(_)).WillByDefault([this](const std::string &channelGid) {
            LOG_EXPECT(this->logger, "getLinksForChannel", channelGid);
            return std::vector<std::string>{};
        });
        ON_CALL(*this, setPluginsForChannel(_, _))
            .WillByDefault(
                [this](const std::string &channelGid, const std::vector<std::string> &plugins) {
                    json pluginsJson = plugins;
                    LOG_EXPECT(this->logger, "setPluginsForChannel", channelGid, pluginsJson);
                });
        ON_CALL(*this, setWrapperIdForChannel(_, _))
            .WillByDefault([this](const std::string &channelGid, const std::string &wrapperId) {
                LOG_EXPECT(this->logger, "setWrapperIdForChannel", channelGid, wrapperId);
            });
        ON_CALL(*this, setLinkId(_, _))
            .WillByDefault([this](const std::string &channelGid, const LinkID &linkId) {
                LOG_EXPECT(this->logger, "setLinkId", channelGid, linkId);
            });
        ON_CALL(*this, removeLinkId(_, _))
            .WillByDefault([this](const std::string &channelGid, const LinkID &linkId) {
                LOG_EXPECT(this->logger, "removeLinkId", channelGid, linkId);
            });
        ON_CALL(*this, setStatus(_, _))
            .WillByDefault([this](const std::string &channelGid, ChannelStatus status) {
                LOG_EXPECT(this->logger, "setStatus", channelGid, status);
            });
        ON_CALL(*this, checkMechanicalTags(_))
            .WillByDefault([this](const std::vector<std::string> &tags) {
                json tagsJson = tags;
                LOG_EXPECT(this->logger, "checkMechanicalTags", tagsJson);
                return false;
            });
        ON_CALL(*this, checkBehavioralTags(_))
            .WillByDefault([this](const std::vector<std::string> &tags) {
                json tagsJson = tags;
                LOG_EXPECT(this->logger, "checkBehavioralTags", tagsJson);
                return false;
            });
        ON_CALL(*this, activate(_, _))
            .WillByDefault([this](const std::string &channelGid, const std::string &roleName) {
                LOG_EXPECT(this->logger, "activate", channelGid, roleName);
                return false;
            });
        ON_CALL(*this, channelFailed(_)).WillByDefault([this](const std::string &channelGid) {
            LOG_EXPECT(this->logger, "channelFailed", channelGid);
        });
        ON_CALL(*this, setAllowedTags(_))
            .WillByDefault([this](const std::vector<std::string> &tags) {
                json tagsJson = tags;
                LOG_EXPECT(this->logger, "setAllowedTags", tagsJson);
            });
        ON_CALL(*this, setUserEnabledChannels(_))
            .WillByDefault([this](const std::vector<std::string> &channelGids) {
                json channels = channelGids;
                LOG_EXPECT(this->logger, "setUserEnabledChannels", channels);
            });
        ON_CALL(*this, setUserEnabled(_)).WillByDefault([this](const std::string &channelGid) {
            LOG_EXPECT(this->logger, "setUserEnabled", channelGid);
        });
        ON_CALL(*this, setUserDisabled(_)).WillByDefault([this](const std::string &channelGid) {
            LOG_EXPECT(this->logger, "setUserDisabled", channelGid);
        });
        ON_CALL(*this, isUserEnabled(_)).WillByDefault([this](const std::string &channelGid) {
            LOG_EXPECT(this->logger, "isUserEnabled", channelGid);
            return true;
        });
    }

    MOCK_METHOD((std::map<std::string, ChannelProperties>), getSupportedChannels, (), (override));
    MOCK_METHOD(bool, update,
                (const std::string &channelGid, ChannelStatus status,
                 const ChannelProperties &properties),
                (override));
    MOCK_METHOD(ChannelProperties, getChannelProperties, (const std::string &channelGid),
                (override));
    MOCK_METHOD(std::vector<std::string>, getPluginsForChannel, (const std::string &channelGid),
                (override));
    MOCK_METHOD(std::string, getWrapperIdForChannel, (const std::string &channelGid), (override));
    MOCK_METHOD(bool, isAvailable, (const std::string &channelGid), (override));
    MOCK_METHOD(bool, update, (const ChannelProperties &properties), (override));
    MOCK_METHOD(std::vector<LinkID>, getLinksForChannel, (const std::string &channelGid),
                (override));
    MOCK_METHOD(void, setPluginsForChannel,
                (const std::string &channelGid, const std::vector<std::string> &plugins),
                (override));
    MOCK_METHOD(void, setWrapperIdForChannel,
                (const std::string &channelGid, const std::string &wrapperId), (override));
    MOCK_METHOD(void, setLinkId, (const std::string &channelGid, const LinkID &linkId), (override));
    MOCK_METHOD(void, removeLinkId, (const std::string &channelGid, const LinkID &linkId),
                (override));
    MOCK_METHOD(void, setStatus, (const std::string &channelGid, ChannelStatus status), (override));
    MOCK_METHOD(bool, checkMechanicalTags, (const std::vector<std::string> &tags), (override));
    MOCK_METHOD(bool, checkBehavioralTags, (const std::vector<std::string> &tags), (override));
    MOCK_METHOD(bool, activate, (const std::string &channelGid, const std::string &roleName),
                (override));
    MOCK_METHOD(void, channelFailed, (const std::string &channelGid), (override));
    MOCK_METHOD(void, setAllowedTags, (const std::vector<std::string> &tags), (override));
    MOCK_METHOD(void, setUserEnabledChannels, (const std::vector<std::string> &), (override));
    MOCK_METHOD(void, setUserEnabled, (const std::string &), (override));
    MOCK_METHOD(void, setUserDisabled, (const std::string &), (override));
    MOCK_METHOD(bool, isUserEnabled, (const std::string &), (override));

    LogExpect &logger;
};

#endif