
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

#ifndef __MOCK_ENCODING_H__
#define __MOCK_ENCODING_H__

#include "IEncodingComponent.h"
#include "LogExpect.h"
#include "gmock/gmock.h"
#include "race_printers.h"

class MockEncoding : public IEncodingComponent {
public:
    MockEncoding(LogExpect &logger, IEncodingSdk &sdk) : logger(logger), sdk(sdk) {
        using ::testing::_;
        ON_CALL(*this, getEncodingProperties()).WillByDefault([this]() {
            LOG_EXPECT(this->logger, "getEncodingProperties");
            return EncodingProperties({0, "application/octet-stream"});
        });
        ON_CALL(*this, getEncodingPropertiesForParameters(_))
            .WillByDefault([this](const EncodingParameters &params) {
                LOG_EXPECT(this->logger, "getEncodingPropertiesForParameters", params);
                return SpecificEncodingProperties({1000});
            });
        ON_CALL(*this, encodeBytes(_, _, _))
            .WillByDefault([this](RaceHandle handle, const EncodingParameters &params,
                                  const std::vector<uint8_t> &bytes) {
                LOG_EXPECT(this->logger, "encodeBytes", handle, params, bytes.size());
                return ComponentStatus::COMPONENT_OK;
            });
        ON_CALL(*this, decodeBytes(_, _, _))
            .WillByDefault([this](RaceHandle handle, const EncodingParameters &params,
                                  const std::vector<uint8_t> &bytes) {
                LOG_EXPECT(this->logger, "decodeBytes", handle, params, bytes.size());
                return ComponentStatus::COMPONENT_OK;
            });
        ON_CALL(*this, onUserInputReceived(_, _, _))
            .WillByDefault([this](RaceHandle handle, bool answered, const std::string &response) {
                LOG_EXPECT(this->logger, "onUserInputReceived", handle, answered, response);
                return ComponentStatus::COMPONENT_OK;
            });
    }
    MOCK_METHOD(EncodingProperties, getEncodingProperties, (), (override));
    MOCK_METHOD(SpecificEncodingProperties, getEncodingPropertiesForParameters,
                (const EncodingParameters &params), (override));
    MOCK_METHOD(ComponentStatus, encodeBytes,
                (RaceHandle handle, const EncodingParameters &params,
                 const std::vector<uint8_t> &bytes),
                (override));
    MOCK_METHOD(ComponentStatus, decodeBytes,
                (RaceHandle handle, const EncodingParameters &params,
                 const std::vector<uint8_t> &bytes),
                (override));
    MOCK_METHOD(ComponentStatus, onUserInputReceived,
                (RaceHandle handle, bool answered, const std::string &response), (override));

public:
    LogExpect &logger;
    IEncodingSdk &sdk;
};

#endif