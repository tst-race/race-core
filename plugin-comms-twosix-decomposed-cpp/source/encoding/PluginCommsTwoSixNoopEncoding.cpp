
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

#include "PluginCommsTwoSixNoopEncoding.h"

#include "log.h"

const char *PluginCommsTwoSixNoopEncoding::name = "noop";

PluginCommsTwoSixNoopEncoding::PluginCommsTwoSixNoopEncoding(IEncodingSdk *_sdk) : sdk(_sdk) {
    if (this->sdk == nullptr) {
        throw std::runtime_error("PluginCommsTwoSixNoopEncoding: sdk parameter is NULL");
    }
    sdk->updateState(COMPONENT_STATE_STARTED);
}

ComponentStatus PluginCommsTwoSixNoopEncoding::onUserInputReceived(RaceHandle handle, bool answered,
                                                                   const std::string &response) {
    TRACE_METHOD(handle, answered, response);

    return COMPONENT_OK;
}

EncodingProperties PluginCommsTwoSixNoopEncoding::getEncodingProperties() {
    TRACE_METHOD();

    return {0, "application/octet-stream"};
}

SpecificEncodingProperties PluginCommsTwoSixNoopEncoding::getEncodingPropertiesForParameters(
    const EncodingParameters & /* params */) {
    TRACE_METHOD();

    return {1000000};
}

ComponentStatus PluginCommsTwoSixNoopEncoding::encodeBytes(RaceHandle handle,
                                                           const EncodingParameters &params,
                                                           const std::vector<uint8_t> &bytes) {
    TRACE_METHOD(handle, params.linkId, params.type, params.encodePackage, params.json);

    // TODO: do I need to thread this? Or should I just for exemplary sake?
    sdk->onBytesEncoded(handle, bytes, ENCODE_OK);

    return COMPONENT_OK;
}

ComponentStatus PluginCommsTwoSixNoopEncoding::decodeBytes(RaceHandle handle,
                                                           const EncodingParameters &params,
                                                           const std::vector<uint8_t> &bytes) {
    TRACE_METHOD(handle, params.linkId, params.type, params.encodePackage, params.json);

    // TODO: do I need to thread this? Or should I just for exemplary sake?
    sdk->onBytesDecoded(handle, bytes, ENCODE_OK);

    return COMPONENT_OK;
}
