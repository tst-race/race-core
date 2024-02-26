
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

#ifndef __EXT_CLR_MSG_H__
#define __EXT_CLR_MSG_H__

#include <string>
#include <vector>

#include "ClrMsg.h"
#include "RaceCrypto.h"

const int UNSET_UUID{-1};
const int UNSET_RING_TTL{-1};

using MsgUuid = std::int64_t;

enum MsgType {
    MSG_UNDEF = 0,          // default / undefined type
    MSG_CLIENT = 1,         // client-to-client message for humans
    MSG_LINKS = 2,          // control-plane link message for network managers
    MSG_BOOTSTRAPPING = 3,  // bootstrapping new node message
};

/**
 * @brief the ExtClrMsg class extends the SDK-defined ClrMsg class to
 * enable additional record-keeping needed by the TwoSix network manager stub
 * server protocol without requiring other network manager performers to use
 * it. In addition to the member data and functions defined here,
 * there are additions to RaceCrypto.h/cpp to handle formatting and
 * parsing ExtClrMsg.
 */
class ExtClrMsg : public ClrMsg {
public:
    ExtClrMsg();
    ExtClrMsg(const std::string &msg, const std::string &from, const std::string &to,
              std::int64_t msgTime, std::int32_t msgNonce, std::int8_t ampIndex, MsgUuid uuid,
              std::int32_t ringTtl, std::int32_t ringIdx, MsgType msgType,
              const std::vector<std::string> &committeesVisited = std::vector<std::string>(),
              const std::vector<std::string> &committeesSent = std::vector<std::string>());
    explicit ExtClrMsg(const ClrMsg &clrMsg);

    /**
     * @brief Set the UUID
     *
     * @param value The new UUID value
     */
    void setUuid(MsgUuid value);

    /**
     * @brief Get the UUID (should be first 8 bytes of a SHA256 of ClrMsg contents)
     *
     * @return The UUID of the ExtClrMsg
     */
    MsgUuid getUuid() const;

    /**
     * @brief Checks if the UUID is a set or unset value
     *
     * @return true if UUID != -1
     */
    bool isUuidSet() const;

    /**
     * @brief Set the ringTtl to the value
     *
     * @param value The value to set ringTtl to
     */
    void setRingTtl(std::int32_t value);

    /**
     * @brief Get the ringTtl value
     *
     * @return The ringTtl value
     */
    std::int32_t getRingTtl() const;

    /**
     * @brief Set the msgType to the value
     *
     * @param value The value to set msgType to
     */
    void setMsgType(MsgType value);

    /**
     * @brief Get the msgType value
     *
     * @return The msgType value
     */
    MsgType getMsgType() const;

    /**
     * @brief Check if the ringTtl value is valid
     *
     * @return true if ringTtl != -1
     */
    bool isRingTtlSet() const;

    /**
     * @brief Sets the ringTtl value to the unset value
     */
    void unsetRingTtl();

    /**
     * @brief Decrement the ringTtl (will make UNSET if it was 0)
     */
    void decRingTtl();

    /**
     * @brief Set the ringIdx
     *
     * @param value The new ringIdx value
     */
    void setRingIdx(std::int32_t value);

    /**
     * @brief Get the ringIdx
     *
     * @return The ringIdx value
     */
    std::int32_t getRingIdx() const;

    /**
     * @brief Append a new committee name to committeesVisited
     *
     * @param value The name of the committee
     */
    void addCommitteeVisited(const std::string &value);

    /**
     * @brief Get the committeesVisited vector
     *
     * @return The committeesVisited vector
     */
    std::vector<std::string> getCommitteesVisited() const;

    /**
     * @brief Add the name of a committee a copy of this message was sent to (so that this recipient
     * will not send to the same)
     *
     * @param value The committee name to add
     */
    void addCommitteeSent(const std::string &value);

    /**
     * @brief Get the committeesSent vector
     *
     * @return The committeeSent vector
     */
    std::vector<std::string> getCommitteesSent() const;

    /**
     * @brief Erase all entries in the committeesSent vector
     */
    void clearCommitteesSent();

    /**
     * @brief Remove the extra fields and return this message as ClrMsg. Used to obtain the message
     * to forward to a client
     *
     * @return This msg pruned to a simple ClrMsg
     */
    ClrMsg asClrMsg() const;

    /**
     * @brief Create a deep-copy of this ExtClrMsg for altering data structures and sending to
     * different recipients.
     *
     * @return A new ExtClrMsg with equal values
     */
    ExtClrMsg copy() const;

    inline int32_t PACK(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3) const {
        return (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
    }

    /**
     * @brief Deterministically construct the UUID for this msg by taking the first 8 bytes of a
     * SHA256 of the ClrMsg of this ExtClrMsg.
     *
     * @return The UUID of this ExtClrMsg
     */
    MsgUuid makeUuid() const;

private:
    RaceCrypto crypto;
    MsgUuid uuid;
    std::int32_t ringTtl;
    std::int32_t ringIdx;
    MsgType msgType;
    std::vector<std::string> committeesVisited;
    std::vector<std::string> committeesSent;
};

#endif
