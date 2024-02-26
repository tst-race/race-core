
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

#ifndef __CLR_MSG_H_
#define __CLR_MSG_H_

#include <cstdint>
#include <string>

const int8_t NON_AMP_MESSAGE = 0;

class ClrMsg {
public:
    ClrMsg(const std::string &msg, const std::string &from, const std::string &to,
           std::int64_t msgTime, std::int32_t msgNonce, std::int8_t ampIndex = NON_AMP_MESSAGE,
           std::uint64_t msgTraceId = 0, std::uint64_t msgSpanId = 0);
    std::string getMsg() const;
    std::string getFrom() const;
    std::string getTo() const;
    std::int64_t getTime() const;
    std::int32_t getNonce() const;
    std::int8_t getAmpIndex() const;

    std::uint64_t getTraceId() const;
    std::uint64_t getSpanId() const;

    void setTraceId(std::uint64_t value);
    void setSpanId(std::uint64_t value);

private:
    std::string plainMsg;
    std::string fromPersona;
    std::string toPersona;
    std::int64_t createTime;
    std::int32_t nonce;

    // Used for determining if the message is for a the client app or an amp plugin (and which one)
    // The value will be NON_AMP_MESSAGE if it's for the client
    std::int8_t ampIndex;

    std::uint64_t traceId;
    std::uint64_t spanId;
};

inline bool operator==(const ClrMsg &lhs, const ClrMsg &rhs) {
    return lhs.getTime() == rhs.getTime() && lhs.getNonce() == rhs.getNonce() &&
           lhs.getFrom() == rhs.getFrom() && lhs.getTo() == rhs.getTo() &&
           lhs.getMsg() == rhs.getMsg() && lhs.getAmpIndex() == rhs.getAmpIndex();

    // don't compare opentracing ids when checking for equality
}

inline bool operator!=(const ClrMsg &lhs, const ClrMsg &rhs) {
    return !operator==(lhs, rhs);
}

#endif
