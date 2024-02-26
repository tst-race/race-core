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

"""
    Purpose:
        RaceCrypto Class. Is a python implementation of an encryption/
        decrpytion method for the stub implementation.
"""

# Python Library Imports
import json
from typing import Optional
from Crypto.Cipher import AES
from hashlib import sha256 as hashclass

# RACE Libraries
from networkManagerPluginBindings import ClrMsg

# Package modules
from .ExtClrMsg import ExtClrMsg
from .Log import logError, logDebug


class RaceCrypto(object):
    """
    RaceCrypto Class. Is a python implementation of an encryption/
    decrpytion method for the stub implementation.
    """

    ###
    # Attributes
    ###

    delimiter = None
    KEY_LENGTH = 32

    ###
    # Class Lifecycle Methods
    ###

    def __init__(self):
        """
        Purpose:
            Constructor for the RaceCrypto Class.
        Args:
            N/A
        Returns:
            N/A
        """

        self.delimiter = "~~~"

    def __del__(self):
        """
        Purpose:
            Destructor for the RaceCrypto Class.
        Args:
            N/A
        Returns:
            N/A
        """

        pass

    #####
    # Encryption/Decryption Methods
    #####

    def encryptClrMsg(self, input_msg, key) -> bytes:
        """
        Purpose:
            Encrypt a message (from a ClrMsg)
        Args:
            input_msg (str): String message to encrypt
            key (bytes) (len(key) = 16): key for the encryption
        Returns:
            encoded_cipher (bytes): encoded encrypted message
        """
        logDebug(f"RaceCrypto: encryptClrMsg called, input len = {len(input_msg)}")

        cipher = AES.new(key, AES.MODE_GCM)
        nonce = cipher.nonce
        ciphertext, mac = cipher.encrypt_and_digest(input_msg.encode("utf-8"))

        if len(nonce) != 16 or len(mac) != 16:
            logError(
                f"Unexpected length from pycryptodome: nonce={len(nonce)}, mac={len(mac)}"
            )
            raise ValueError()

        encmsg = nonce + mac + ciphertext

        logDebug(f"RaceCrypto: encryptClrMsg finished, output len = {len(encmsg)}")
        return encmsg

    def decryptEncPkg(self, encmsg, key) -> str:
        """
        Purpose:
            Decrypt a message (from a EncPkg)

            Will decrypt a message that was encrypted using the encryptClrMsg
            method.
        Args:
            encmsg (bytes): encoded encrypted message to decrypt
            key (bytes) (len(key) = 16): key for the decryption
        Returns:
            output_msg (str): String message that was decrypted
        """
        logDebug(f"RaceCrypto: decryptEncPkg called, input len = {len(encmsg)}")
        if len(encmsg) <= 32:
            raise ValueError()

        nonce = encmsg[:16]
        mac = encmsg[16:32]
        ciphertext = encmsg[32:]

        cipher = AES.new(key, AES.MODE_GCM, nonce=nonce)
        output_msg = cipher.decrypt_and_verify(ciphertext, mac).decode("utf-8")

        logDebug(f"RaceCrypto: decryptEncPkg finished, output len = {len(output_msg)}")
        return output_msg

    def getDelimiter(self):
        """
        Purpose:
            Get the delimiter splitting the message
        Args:
            N/A
        Returns:
            delimiter (String): delimiter splitting the message in a string
        """

        return self.delimiter

    def formatDelimitedMessage(self, msg_obj: ClrMsg) -> str:
        """
        Purpose:
            Format a message object as a string for sending
        Args:
            msg_obj (ExtClrMsg obj): Message object to encode
        Returns:
            delimited_message (String): encoded message
        """
        if isinstance(msg_obj, ExtClrMsg):
            return f"{self.delimiter}".join(
                [
                    f"extClrMsg",
                    f"{msg_obj.getMsg()}",
                    f"{msg_obj.getFrom()}",
                    f"{msg_obj.getTo()}",
                    f"{msg_obj.getTime()}",
                    f"{msg_obj.getNonce()}",
                    f"{msg_obj.getAmpIndex()}",
                    f"{msg_obj.uuid}",
                    f"{msg_obj.ringTtl}",
                    f"{msg_obj.ringIdx}",
                    f"{msg_obj.msgType}",
                    f"{json.dumps(msg_obj.committeesVisited)}",
                    f"{json.dumps(msg_obj.committeesSent)}",
                ]
            )
        else:
            return (
                f"clrMsg{self.delimiter}{msg_obj.getMsg()}{self.delimiter}"
                f"{msg_obj.getFrom()}{self.delimiter}{msg_obj.getTo()}"
                f"{self.delimiter}{msg_obj.getTime()}{self.delimiter}"
                f"{msg_obj.getNonce()}{self.delimiter}"
                f"{msg_obj.getAmpIndex()}"
            )

    def parseDelimitedMessage(self, msg_text: str) -> Optional[ClrMsg]:
        """
        Purpose:
            Parse a delimited messages into a ClrMsg.
        Args:
            msg_text (String): encoded message
        Returns:
            msg_obj (ClrMsg obj): Message object decoded from delimited_message
        """
        tokens = msg_text.split(self.delimiter)
        logDebug(f"message: {msg_text}")
        if len(tokens) == 7:
            msg = tokens[1]
            _from = tokens[2]
            to = tokens[3]
            msgTime = int(tokens[4])
            msgNonce = int(tokens[5])
            msgAmpIndex = int(tokens[6])
            if len(tokens) == 7:
                return ClrMsg(msg, _from, to, msgTime, msgNonce, msgAmpIndex)

        logError("Invalid message to parse")
        return None

    def parseDelimitedExtMessage(self, msg_text: str) -> Optional[ExtClrMsg]:
        """
        Purpose:
            Parse a delimited messages into a ExtClrMsg.
        Args:
            msg_text (String): encoded message
        Returns:
            msg_obj (ExtClrMsg obj): Message object decoded from delimited_message
        """
        tokens = msg_text.split(self.delimiter)
        logDebug(f"message: {msg_text}")
        if len(tokens) >= 7:
            msg = tokens[1]
            _from = tokens[2]
            to = tokens[3]
            msgTime = int(tokens[4])
            msgNonce = int(tokens[5])
            msgAmpIndex = int(tokens[6])

        if len(tokens) == 7:
            return ExtClrMsg.fromClrMsg(
                ClrMsg(msg, _from, to, msgTime, msgNonce, msgAmpIndex)
            )

        elif len(tokens) == 13:
            uuid = int(tokens[7])
            ringTtl = int(tokens[8])
            ringIdx = int(tokens[9])
            msgType = int(tokens[10])
            committeesVisited = json.loads(tokens[11])
            committeesSent = json.loads(tokens[12])
            return ExtClrMsg(
                msg,
                _from,
                to,
                msgTime,
                msgNonce,
                msgAmpIndex,
                uuid,
                ringTtl,
                ringIdx,
                msgType,
                committeesVisited,
                committeesSent,
            )

        logError("Invalid message to parse")
        return None

    def getMessageHash(self, msg: ClrMsg) -> bytes:
        """
        Purpose:
            Get the SHA256 hahs of a ClrMsg
        Args:
            msg (ClrMsg): ClrMsg to hash
        Returns:
            hash (bytes): the 32 bytes of the SHA256 hash
        """
        h = hashclass()
        h.update((self.formatDelimitedMessage(msg)).encode("utf-8"))
        return h.digest()
