#!/usr/bin/env python3.6

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
        Mock classes so that unittests don't ned to link
        cross language for testing. Basically a hardcoded
        version of the SDK
"""

# Python Library Imports
import inspect
import os
import sys
import pytest
from enum import Enum
from unittest import mock


###
# Mock Classes
###


class MockLinkType(Enum):
    LT_UNDEF = 0  # undefined
    LT_SEND = 1  # send
    LT_RECV = 2  # receive
    LT_BIDI = 3  # bidirectional


class MockRaceSdk(object):
    """
    Purpose:
        Mock the RaceSdk Object
    """

    def __init__(self):
        """"""
        self.methodCallCount = {}
        self.increment_call_counter(inspect.currentframe().f_code.co_name)

        self.linkProfiles = [
            {
                "linkId": 1,
                "utilizedBy": [
                    "existing-race-persona-1",
                ],
                "connectedTo": [
                    "existing-race-persona-2",
                ],
                "profile": "{}",
                "properties": {
                    "type": MockLinkType.LT_SEND,
                },
            },
            {
                "linkId": 2,
                "utilizedBy": [
                    "existing-race-persona-1",
                ],
                "connectedTo": [
                    "existing-race-persona-2",
                ],
                "profile": "{}",
                "properties": {
                    "type": MockLinkType.LT_RECV,
                },
            },
        ]
        self.connIds = []
        self.activePersona = "existing-race-persona-1"

        pass

    def getActivePersona(self):
        """
        Purpose:
            Mock the getActivePersona Method
        Args:
            N/A
        Returns:
            personaUuid (String): UUID of the persona
        """
        self.increment_call_counter(inspect.currentframe().f_code.co_name)

        return self.activePersona

    def getLinks(self, personaUuid, linkType):
        """
        Purpose:
            Mock the getLinks Method
        Args:
            personaUuid (String): UUID of the persona to send to
            linkType (linkType enum): type of link to get
        Returns:
            personaUuid (String): UUID of the persona
        """
        self.increment_call_counter(inspect.currentframe().f_code.co_name)

        if not isinstance(linkType, MockLinkType):
            raise Exception(f"{linkType} Is not a link type")

        possibleLinks = []
        for linkProfile in self.linkProfiles:
            self.activePersona
            if (
                linkType == linkProfile["properties"]["type"]
                and self.activePersona in linkProfile["utilizedBy"]
                and personaUuid in linkProfile["connectedTo"]
            ):
                possibleLinks.append(linkProfile)

        return possibleLinks

    def sendEncryptedPackage(self, ePkg, connId, batchId, timeout):
        """
        Purpose:
            Mock the sendEncryptedPackage Method
        Args:
            N/A
        Returns:
            N/A
        """
        self.increment_call_counter(inspect.currentframe().f_code.co_name)

        pass

    def openConnection(self, linkType, chosenLinkID, config, linkHints, timeout):
        """
        Purpose:
            Mock the openConnection Method
        Args:
            N/A
        Returns:
            connId (String): ID of the new connection
        """
        self.increment_call_counter(inspect.currentframe().f_code.co_name)

        connId = f"{chosenLinkID}_{linkType.name}_conn"

        return connId

    def closeConnection(self, connId):
        """
        Purpose:
            Mock the closeConnection Method
        Args:
            N/A
        Returns:
            N/A
        """
        self.increment_call_counter(inspect.currentframe().f_code.co_name)

        return

    def presentCleartextMessage(self, clrMsg):
        """
        Purpose:
            Mock the presentCleartextMessage Method
        Args:
            N/A
        Returns:
            N/A
        """
        self.increment_call_counter(inspect.currentframe().f_code.co_name)

        print(f"common_clssses.py | SDK: {clrMsg}")

        return

    def increment_call_counter(self, method_name):
        """
        Purpose:
            Incremenet the call counter for the method
        Args:
            method_name (String): Increment the call counter
        Returns:
            N/A
        """

        self.methodCallCount.setdefault(method_name, 0)
        self.methodCallCount[method_name] += 1


if __name__ == "__main__":

    x = MockRaceSdk()
    myPersona = x.getActivePersona()
    theirPersona = "existing-race-persona-2"
    y = x.getLinks(theirPersona, MockLinkType.LT_SEND)
    z = x.getLinks(theirPersona, MockLinkType.LT_RECV)
    import pdb

    pdb.set_trace()
