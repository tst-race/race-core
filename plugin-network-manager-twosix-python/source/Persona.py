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
        Persona Class. Holds the python implementation of
        the persona class.
"""

# Python Library Imports
from enum import Enum


###
# Enum of Type
###


class PersonaType(Enum):
    """
    Purpose:
        Enum for the different types of personas
    """

    P_CLIENT = 0  # client
    P_SERVER = 1  # server


###
# Class
###


class Persona:
    """
    Purpose:
        Persona Class. Holds the python implementation of
        the persona class.
    """

    def __init__(self):
        """
        Purpose:
            Initialize the Persona Class
        Args:
            N/A
        Returns:
            N/A
        """

        self.aesKey = b""
        self.displayName = ""
        self.raceUuid = ""
        self.publicKey = 0
        self.personaType = ""

    def __eq__(self, other_persona):
        """
        Purpose:
            Equality checker for Persona Objects. Uses value of attributes to compare.
        Args:
            other_persona (Persona Obj): Obj to check equiality against
        Returns:
            isEqual (Bool): If the personas are equal
        """

        if isinstance(other_persona, self.__class__):
            return self.__dict__ == other_persona.__dict__
        else:
            return False

    def getAesKey(self):
        """
        Purpose:
            Get Aes Key value for a Persona
        Args:
            N/A
        Returns:
            aesKey (Bytes): aes key for persona
        """
        return self.aesKey

    def getDisplayName(self):
        """
        Purpose:
            Get Display Name value for a Persona
        Args:
            N/A
        Returns:
            displayName (String): DisplayName of the Race node
        """
        return self.displayName

    def getRaceUuid(self):
        """
        Purpose:
            Get UUID value for a Persona
        Args:
            N/A
        Returns:
            raceUuid (String): UUID of Race node
        """
        return self.raceUuid

    def getPublicKey(self):
        """
        Purpose:
            Get Public Key value for a Persona
        Args:
            N/A
        Returns:
            publicKey (Int): public key for persona
        """
        return self.publicKey

    def getPersonaType(self):
        """
        Purpose:
            Get Persona Type value for a Persona
        Args:
            N/A
        Returns:
            personaType (PersonaType): client or seerver
        """
        return self.personaType

    def setAesKey(self, aesKey):
        """
        Purpose:
            Set Aes Key value for a Persona
        Args:
            aesKey (Bytes): value to set
        Returns:
            N/A
        """
        self.aesKey = aesKey

    def setDisplayName(self, displayName):
        """
        Purpose:
            Set Display Name value for a Persona
        Args:
            displayName (String): value to set
        Returns:
            N/A
        """
        self.displayName = displayName

    def setRaceUuid(self, raceUuid):
        """
        Purpose:
            Set UUID value for a Persona
        Args:
            raceUuid (String): value to set
        Returns:
            N/A
        """
        self.raceUuid = raceUuid

    def setPublicKey(self, publicKey):
        """
        Purpose:
            Set Public Key value for a Persona
        Args:
            publicKey (Int): value to set
        Returns:
            N/A
        """
        self.publicKey = publicKey

    def setPersonaType(self, personaType):
        """
        Purpose:
            Set Persona Type value for a Persona
        Args:
            personaType (enum): value to set
        Returns:
            N/A
        """
        self.personaType = personaType
