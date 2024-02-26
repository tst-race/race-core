#
# Copyright 2023 Two Six Technologies
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from copy import deepcopy
from hashlib import sha256 as hashclass
from networkManagerPluginBindings import ClrMsg
from typing import List


class ExtClrMsg(ClrMsg):
    """
    ExtClrMsg Class. An extension of ClrMsg to support additional fields used by the
    TwoSix network manager stub servers. Requires support from RaceCrypto.py for
    formating / parsing
    """

    UNSET_UUID = -1
    UNSET_RING_TTL = -1

    MSG_UNDEF = 0  # Default / undefined type
    MSG_CLIENT = 1  # client-to-client message for humans
    MSG_LINKS = 2  # control-plane link message for network managers

    ###
    # Class Lifecycle Methods
    ###

    def __init__(
        self,
        msg: str,
        _from: str,
        to: str,
        msgTime: int,
        msgNonce: int,
        msgAmpIndex: int,
        uuid: int = UNSET_UUID,
        ringTtl: int = -1,
        ringIdx: int = 0,
        msgType: int = MSG_UNDEF,
        committeesVisited: List[str] = None,
        committeesSent: List[str] = None,
    ):
        """
        Purpose:
            Constructor for the ExtClrMsg Class.
        Args:
            msg (String): the message contents
            _from (String): UUID of the sender
            to (String): UUID of the recipient
            msgTime (Int): time the message was sent
            msgNonce (Int): nonce to differentiate identical messages
            uuid (Int): UUID of this message, used to detect duplicates
            ringTtl (Int): TTL for a ring message to detect when it has finished the ring
            ringIdx (Int): Index of which ring is being traversed
            msgType (Int): Type of the message
            committeesVisited (List<String>): List of committee names this messsage has
                already been to.
            committeesSent (List<String>): List of committee names this message is already
                being sent to.
        Returns:
            N/A
        """

        super().__init__(msg, _from, to, msgTime, msgNonce, msgAmpIndex)
        self.uuid = uuid
        self.ringTtl = ringTtl
        self.ringIdx = ringIdx
        self.msgType = msgType
        self.committeesVisited = [] if committeesVisited is None else committeesVisited
        self.committeesSent = [] if committeesSent is None else committeesSent

    def isUuidSet(self) -> bool:
        """
        Purpose:
            Checks if this message's UUID is set (not -1)
        Args:
            N/A
        Returns:
            truth (Bool): True if UUID != ExtClrMsg.UNSET_UUID (-1)
        """
        return self.uuid != ExtClrMsg.UNSET_UUID

    def unsetRingTtl(self):
        """
        Purpose:
             Sets the ringTtl to the unset value (ExtClrMsg.UNSET_RING_TTL). Used when
                forwarding to a new committee.
        Args:
            N/A
        Returns:
            N/A
        """
        self.ringTtl = ExtClrMsg.UNSET_RING_TTL

    def decRingTtl(self):
        """
        Purpose:
            Decrement the ringTtl value (but do not go below 0)
        Args:
            N/A
        Returns:
            N/A
        """
        if self.ringTtl > 0:
            self.ringTtl -= 1

    def isRingTtlSet(self) -> bool:
        """
        Purpose:
            Checks if the ringTtl is a set value (not ExtClrMsg.UNSET_RING_TTL)
        Args:
            N/A
        Returns:
            truth (Bool): True if ringTtl != ExrClrMsg.UNSET_RING_TTL (-1)
        """
        return self.ringTtl != ExtClrMsg.UNSET_RING_TTL

    def asClrMsg(self) -> ClrMsg:
        """
        Purpose:
            Return a copy of this message as a ClrMsg, removing all the extended message
                fields
        Args:
            N/A
        Returns:
            clrMsg (ClrMsg): the ClrMsg version of this ExtClrMsg
        """
        clrMsg = ClrMsg(
            self.getMsg(),
            self.getFrom(),
            self.getTo(),
            self.getTime(),
            self.getNonce(),
            self.getAmpIndex(),
        )
        clrMsg.setSpanId(self.getSpanId())
        clrMsg.setTraceId(self.getTraceId())
        return clrMsg

    def copy(self) -> "ExtClrMsg":
        """
        Purpose:
            Create a deep copy of this ExtClrMsg.
        Args:
            N/A
        Returns:
            N/A
        """
        cpy = ExtClrMsg(
            self.getMsg(),
            self.getFrom(),
            self.getTo(),
            self.getTime(),
            self.getNonce(),
            self.getAmpIndex(),
            self.uuid,
            self.ringTtl,
            self.ringIdx,
            self.msgType,
            deepcopy(self.committeesVisited),
            deepcopy(self.committeesSent),
        )
        cpy.setSpanId(self.getSpanId())
        cpy.setTraceId(self.getTraceId())
        return cpy

    @classmethod
    def fromClrMsg(cls, clrMsg: ClrMsg) -> "ExtClrMsg":
        """
        Purpose:
            Construct an ExtClrMsg from a ClrMsg.
        Args:
            clrMsg (ClrMsg): the ClrMsg to construct from
        Returns:
            N/A
        """
        h = hashclass()
        h.update(
            (
                f"{clrMsg.getMsg()}"
                f"{clrMsg.getFrom()}"
                f"{clrMsg.getTo()}"
                f"{clrMsg.getTime()}"
                f"{clrMsg.getNonce()}"
                f"{clrMsg.getAmpIndex()}"
            ).encode("utf-8")
        )
        uuid = int.from_bytes(h.digest()[:8], "big")
        while uuid == ExtClrMsg.UNSET_UUID:
            uuid = 1
        ext = ExtClrMsg(
            clrMsg.getMsg(),
            clrMsg.getFrom(),
            clrMsg.getTo(),
            clrMsg.getTime(),
            clrMsg.getNonce(),
            clrMsg.getAmpIndex(),
            uuid=uuid,
        )
        ext.setSpanId(clrMsg.getSpanId())
        ext.setTraceId(clrMsg.getTraceId())
        ext.msgType = cls.MSG_CLIENT
        return ext
