
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

#include "ClrMsg.h"

ClrMsg::ClrMsg(const std::string &msg, const std::string &from, const std::string &to,
               std::int64_t msgTime, std::int32_t msgNonce, std::int8_t ampIndex,
               std::uint64_t msgTraceId, std::uint64_t msgSpanId) :
    plainMsg(msg),
    fromPersona(from),
    toPersona(to),
    createTime(msgTime),
    nonce(msgNonce),
    ampIndex(ampIndex),
    traceId(msgTraceId),
    spanId(msgSpanId) {}

std::string ClrMsg::getMsg() const {
    return plainMsg;
}

std::string ClrMsg::getFrom() const {
    return fromPersona;
}

std::string ClrMsg::getTo() const {
    return toPersona;
}

std::int64_t ClrMsg::getTime() const {
    return createTime;
}

std::int32_t ClrMsg::getNonce() const {
    return nonce;
}

std::int8_t ClrMsg::getAmpIndex() const {
    return ampIndex;
}

std::uint64_t ClrMsg::getTraceId() const {
    return traceId;
}

std::uint64_t ClrMsg::getSpanId() const {
    return spanId;
}

void ClrMsg::setTraceId(std::uint64_t value) {
    traceId = value;
}

void ClrMsg::setSpanId(std::uint64_t value) {
    spanId = value;
}
