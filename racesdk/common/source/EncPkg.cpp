
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

#include "EncPkg.h"

#include <cstring>  // memcpy

const size_t TRACE_ID_LENGTH = sizeof(uint64_t);     // using 64 bit trace id
const size_t SPAN_ID_LENGTH = sizeof(uint64_t);      // using 64 bit span id
const size_t PACKAGE_TYPE_LENGTH = sizeof(uint8_t);  // using 8 bit package type

EncPkg::EncPkg(uint64_t _traceId, uint64_t _spanId, const RawData &_cipherText) :
    traceId(_traceId),
    spanId(_spanId),
    packageType(static_cast<uint8_t>(PKG_TYPE_UNDEF)),
    cipherText(_cipherText) {}

EncPkg::EncPkg(RawData rawData) :
    traceId(0), spanId(0), packageType(static_cast<uint8_t>(PKG_TYPE_UNDEF)) {
    // TODO: should we throw an exception if the incoming size is greater than zero but less than
    // the size TRACE_ID_LENGTH + SPAN_ID_LENGTH?
    if (rawData.size() >= (TRACE_ID_LENGTH + SPAN_ID_LENGTH + PACKAGE_TYPE_LENGTH)) {
        // TODO: endianness
        std::memcpy(&traceId, rawData.data(), TRACE_ID_LENGTH);
        std::memcpy(&spanId, rawData.data() + TRACE_ID_LENGTH, SPAN_ID_LENGTH);
        std::memcpy(&packageType, rawData.data() + (TRACE_ID_LENGTH + SPAN_ID_LENGTH),
                    PACKAGE_TYPE_LENGTH);
        cipherText =
            RawData(rawData.begin() + (TRACE_ID_LENGTH + SPAN_ID_LENGTH + PACKAGE_TYPE_LENGTH),
                    rawData.end());
    }
}

RawData EncPkg::getRawData() const {
    RawData rawData;
    // TODO: endianness
    rawData.insert(rawData.end(), reinterpret_cast<const uint8_t *>(&traceId),
                   reinterpret_cast<const uint8_t *>(&traceId) + sizeof(traceId));
    rawData.insert(rawData.end(), reinterpret_cast<const uint8_t *>(&spanId),
                   reinterpret_cast<const uint8_t *>(&spanId) + sizeof(spanId));
    rawData.insert(rawData.end(), reinterpret_cast<const uint8_t *>(&packageType),
                   reinterpret_cast<const uint8_t *>(&packageType) + sizeof(packageType));
    rawData.insert(rawData.end(), cipherText.begin(), cipherText.end());

    return rawData;
}

RawData EncPkg::getCipherText() const {
    return cipherText;
}

uint64_t EncPkg::getTraceId() const {
    return traceId;
}

uint64_t EncPkg::getSpanId() const {
    return spanId;
}

PackageType EncPkg::getPackageType() const {
    switch (packageType) {
        case PKG_TYPE_NM:
            return PKG_TYPE_NM;
        case PKG_TYPE_TEST_HARNESS:
            return PKG_TYPE_TEST_HARNESS;
        case PKG_TYPE_SDK:
            return PKG_TYPE_SDK;
        default:
            return PKG_TYPE_UNDEF;
    }
}

void EncPkg::setTraceId(uint64_t value) {
    spanId = value;
}

void EncPkg::setSpanId(uint64_t value) {
    spanId = value;
}

void EncPkg::setPackageType(PackageType value) {
    packageType = static_cast<uint8_t>(value);
}

size_t EncPkg::getSize() const {
    return cipherText.size() + TRACE_ID_LENGTH + SPAN_ID_LENGTH + PACKAGE_TYPE_LENGTH;
}
