
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

#include "PluginCommsTwoSixBase64Encoding.h"

#include "base64.h"
#include "log.h"

const char *PluginCommsTwoSixBase64Encoding::name = "base64";

PluginCommsTwoSixBase64Encoding::PluginCommsTwoSixBase64Encoding(IEncodingSdk *_sdk) : sdk(_sdk) {
    if (this->sdk == nullptr) {
        throw std::runtime_error("PluginCommsTwoSixBase64Encoding: sdk parameter is NULL");
    }
    sdk->updateState(COMPONENT_STATE_STARTED);
}

ComponentStatus PluginCommsTwoSixBase64Encoding::onUserInputReceived(RaceHandle handle,
                                                                     bool answered,
                                                                     const std::string &response) {
    TRACE_METHOD(handle, answered, response);

    return COMPONENT_OK;
}

EncodingProperties PluginCommsTwoSixBase64Encoding::getEncodingProperties() {
    TRACE_METHOD();

    return {0, "text/plain"};
}

SpecificEncodingProperties PluginCommsTwoSixBase64Encoding::getEncodingPropertiesForParameters(
    const EncodingParameters & /* params */) {
    TRACE_METHOD();

    return {1000000};
}

ComponentStatus PluginCommsTwoSixBase64Encoding::encodeBytes(RaceHandle handle,
                                                             const EncodingParameters &params,
                                                             const std::vector<uint8_t> &bytes) {
    TRACE_METHOD(handle, params.linkId, params.type, params.encodePackage, params.json);

    // TODO: do I need to thread this? Or should I just for exemplary sake?

    std::string base64EncodedBytes = base64::encode(bytes);
    std::vector<uint8_t> encodedBytes(base64EncodedBytes.begin(), base64EncodedBytes.end());
    sdk->onBytesEncoded(handle, encodedBytes, ENCODE_OK);

    return COMPONENT_OK;
}

ComponentStatus PluginCommsTwoSixBase64Encoding::decodeBytes(RaceHandle handle,
                                                             const EncodingParameters &params,
                                                             const std::vector<uint8_t> &bytes) {
    TRACE_METHOD(handle, params.linkId, params.type, params.encodePackage, params.json);

    // TODO: do I need to thread this? Or should I just for exemplary sake?

    std::vector<uint8_t> decodedBytes;
    try {
        std::string base64Bytes(bytes.begin(), bytes.end());
        decodedBytes = base64::decode(base64Bytes);
    } catch (const std::invalid_argument &e) {
        logError(logPrefix + "failed to decode: " + std::string(e.what()));

        // TODO: should this call be made to indicate error?
        sdk->onBytesDecoded(handle, {}, ENCODE_FAILED);

        return COMPONENT_ERROR;
    }
    sdk->onBytesDecoded(handle, decodedBytes, ENCODE_OK);

    return COMPONENT_OK;
}
