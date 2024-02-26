
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

#include "ExtClrMsg.h"

#include <openssl/evp.h>

#include <cstring>

#include "Log.h"
#include "RaceLog.h"

ExtClrMsg::ExtClrMsg() :
    ClrMsg{"", "", "", 1, 0},
    uuid(UNSET_UUID),
    ringTtl(UNSET_RING_TTL),
    ringIdx(0),
    msgType(MSG_UNDEF) {}

ExtClrMsg::ExtClrMsg(const std::string &msg, const std::string &from, const std::string &to,
                     std::int64_t msgTime, std::int32_t msgNonce, std::int8_t ampIndex,
                     MsgUuid msgUuid, std::int32_t msgRingTtl, std::int32_t msgRingIdx,
                     MsgType msgType, const std::vector<std::string> &msgCommitteesVisited,
                     const std::vector<std::string> &msgCommitteesSent) :
    ClrMsg{msg, from, to, msgTime, msgNonce, ampIndex},
    uuid(msgUuid),
    ringTtl(msgRingTtl),
    ringIdx(msgRingIdx),
    msgType(msgType),
    committeesVisited(msgCommitteesVisited),
    committeesSent(msgCommitteesSent) {}

ExtClrMsg::ExtClrMsg(const ClrMsg &clrMsg) :
    ClrMsg{clrMsg.getMsg(),   clrMsg.getFrom(),     clrMsg.getTo(),      clrMsg.getTime(),
           clrMsg.getNonce(), clrMsg.getAmpIndex(), clrMsg.getTraceId(), clrMsg.getSpanId()} {
    uuid = makeUuid();
    ringTtl = UNSET_RING_TTL;
    ringIdx = 0;
    msgType = MSG_CLIENT;
}

/**
 * @brief Deterministically construct the UUID for this msg by taking the first 8 bytes of a SHA256
 * of the ClrMsg of this ExtClrMsg.
 *
 * @return The UUID of this ExtClrMsg
 */
MsgUuid ExtClrMsg::makeUuid() const {
    // MsgHash digest = crypto.getMessageHash(*this);
    MsgHash digest = crypto.getMessageHash(reinterpret_cast<const ClrMsg &>(*this));
    MsgUuid newUuid;
    logDebug("makeUuid about to memcpy");
    std::memcpy(static_cast<void *>(&newUuid), digest.data(), sizeof(MsgUuid));
    if (newUuid == UNSET_UUID) {
        newUuid = 1;
    }
    logDebug("makeUuid memcpy'd " + std::to_string(newUuid));
    return newUuid;
}

/**
 * @brief Set the UUID
 *
 * @param value The new UUID value
 */
void ExtClrMsg::setUuid(MsgUuid value) {
    uuid = value;
}

/**
 * @brief Get the UUID (should be first 8 bytes of a SHA256 of ClrMsg contents)
 *
 * @return The UUID of the ExtClrMsg
 */
MsgUuid ExtClrMsg::getUuid() const {
    return uuid;
}

/**
 * @brief Checks if the UUID is a set or unset value
 *
 * @return true if UUID != -1
 */
bool ExtClrMsg::isUuidSet() const {
    return uuid != UNSET_UUID;
}

/**
 * @brief Set the ringTtl to the value
 *
 * @param value The value to set ringTtl to
 */
void ExtClrMsg::setRingTtl(std::int32_t value) {
    ringTtl = value;
}

/**
 * @brief Get the ringTtl value
 *
 * @return The ringTtl value
 */
std::int32_t ExtClrMsg::getRingTtl() const {
    return ringTtl;
}

/**
 * @brief Set the msgType to the value
 *
 * @param value The value to set msgType to
 */
void ExtClrMsg::setMsgType(MsgType value) {
    msgType = value;
}

/**
 * @brief Get the msgType value
 *
 * @return The msgType value
 */
MsgType ExtClrMsg::getMsgType() const {
    return msgType;
}

/**
 * @brief Check if the ringTtl value is valid
 *
 * @return true if ringTtl != -1
 */
bool ExtClrMsg::isRingTtlSet() const {
    return ringTtl != UNSET_RING_TTL;
}

/**
 * @brief Sets the ringTtl value to the unset value
 */
void ExtClrMsg::unsetRingTtl() {
    ringTtl = UNSET_RING_TTL;
}

/**
 * @brief Decrement the ringTtl unless it is already 0
 */
void ExtClrMsg::decRingTtl() {
    if (ringTtl > 0) {
        --ringTtl;
    }
}

/**
 * @brief Set the ringIdx
 *
 * @param value The new ringIdx value
 */
void ExtClrMsg::setRingIdx(std::int32_t value) {
    ringIdx = value;
}

/**
 * @brief Get the ringIdx
 *
 * @return The ringIdx value
 */
std::int32_t ExtClrMsg::getRingIdx() const {
    return ringIdx;
}

/**
 * @brief Append a new committee name to committeesVisited
 *
 * @param value The name of the committee
 */
void ExtClrMsg::addCommitteeVisited(const std::string &value) {
    committeesVisited.push_back(value);
}

/**
 * @brief Get the committeesVisited vector
 *
 * @return The committeesVisited vector
 */
std::vector<std::string> ExtClrMsg::getCommitteesVisited() const {
    return committeesVisited;
}

/**
 * @brief Add the name of a committee a copy of this message was sent to (so that this recipient
 * will not send to the same)
 *
 * @param value The committee name to add
 */
void ExtClrMsg::addCommitteeSent(const std::string &value) {
    committeesSent.push_back(value);
}

/**
 * @brief Get the committeesSent vector
 *
 * @return The committeeSent vector
 */
std::vector<std::string> ExtClrMsg::getCommitteesSent() const {
    return committeesSent;
}

/**
 * @brief Erase all entries in the committeesSent vector
 */
void ExtClrMsg::clearCommitteesSent() {
    committeesSent.clear();
}

/**
 * @brief Remove the extra fields and return this message as ClrMsg. Used to obtain the message to
 * forward to a client
 *
 * @return This msg pruned to a simple ClrMsg
 */
ClrMsg ExtClrMsg::asClrMsg() const {
    ClrMsg result = ClrMsg(getMsg(), getFrom(), getTo(), getTime(), getNonce(), getAmpIndex());
    result.setSpanId(getSpanId());
    result.setTraceId(getTraceId());
    return result;
}

/**
 * @brief Create a deep-copy of this ExtClrMsg for altering data structures and sending to different
 * recipients.
 *
 * @return A new ExtClrMsg with equal values
 */
ExtClrMsg ExtClrMsg::copy() const {
    ExtClrMsg result = ExtClrMsg(getMsg(), getFrom(), getTo(), getTime(), getNonce(), getAmpIndex(),
                                 getUuid(), getRingTtl(), getRingIdx(), getMsgType(),
                                 getCommitteesVisited(), getCommitteesSent());
    result.setTraceId(getTraceId());
    result.setSpanId(getSpanId());
    return result;
}
