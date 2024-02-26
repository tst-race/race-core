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
        Base Python implementation for network manager plugins
"""

# Python Libraries
from functools import cmp_to_key
from typing import Dict, List, Set, Tuple
import traceback

# RACE Libraries
from .LinkWizard import LinkWizard
from .Log import logDebug, logError, logInfo, logWarning
from .PluginHelpers import read_json_file_into_memory
from .Persona import Persona, PersonaType
from .RaceCrypto import RaceCrypto

# Swig Libraries
from networkManagerPluginBindings import (
    ByteVector,
    EncPkg,
    IRacePluginNM,
    IRaceSdkNM,
    LinkProperties,
    ChannelProperties,
    CHANNEL_AVAILABLE,
)
from networkManagerPluginBindings import CONNECTION_CLOSED, CONNECTION_OPEN  # ConnectionStatus
from networkManagerPluginBindings import (
    LINK_CREATED,
    LINK_DESTROYED,
    LINK_LOADED,
)  # LinkStatus
from networkManagerPluginBindings import LT_RECV, LT_SEND, LT_UNDEF  # LinkTypes
from networkManagerPluginBindings import PLUGIN_ERROR, PLUGIN_FATAL, PLUGIN_OK  # PluginResponse
from networkManagerPluginBindings import SDK_OK  # SdkStatus
from networkManagerPluginBindings import CT_UNDEF  # ConnectionTypes
from networkManagerPluginBindings import RACE_UNLIMITED  # timeout constant
from networkManagerPluginBindings import RACE_BLOCKING
from networkManagerPluginBindings import PLUGIN_READY  # PluginStatus
from networkManagerPluginBindings import NULL_RACE_HANDLE

from networkManagerPluginBindings import UD_TOAST  # UserDisplayTypes


LinkType = int  # mypy type alias
PluginResponse = int  # mypy type alias


class PluginNMTwoSix(IRacePluginNM):
    """
    Base class for Network Manager Python plugins
    """

    def __init__(self, sdk: IRaceSdkNM, persona_type: PersonaType):
        """
        Purpose:
            Constructor for the class.
        Args:
            sdk (IRaceSdkNM): RACE Network Manager SDK
        Returns:
            N/A
        """
        super().__init__()

        if sdk is None:
            raise Exception("sdk was not passed to plugin")
        self._race_sdk = sdk

        # Encryptor and Decryptor
        self.encryptor = RaceCrypto()

        self._race_persona = Persona()
        self._uuid_to_persona_map: Dict[str, Persona] = {}

        self._opening_connections: Dict[int, Tuple[Persona, LinkType]] = {}
        self._recv_connections: Set[str] = set()
        self._uuid_to_send_connections_map: Dict[
            str, List[Tuple[str, LinkProperties]]
        ] = {}
        self._send_connection_to_uuid_map: Dict[str, str] = {}
        self._expected_links = {}
        self._channel_roles = {}
        self._link_wizard = LinkWizard(
            self._race_sdk.getActivePersona(), persona_type, self, self._race_sdk
        )
        self._obtain_unicast_link_to_retry: Dict[str, LinkType] = {}
        self.staticLinkProfiles = {}
        self.link_wizard_initialized = False
        self.channels_to_use: Set[str] = set()
        self.genesis_link_requests: Set[str] = set()

    def loadPersonas(self, sdk: IRaceSdkNM) -> None:
        """
        Purpose:
            Load all personas from the config into a dict with key being
            the UUID and value being the persona object. Also set the persona
            object for the plugin
        Args:
            plugin_config_file_path (str): Path to the plugin configuration file directory
        Returns:
            N/A
        """
        logInfo("loadPersonas: called")

        active_uuid = self._race_sdk.getActivePersona()
        if not active_uuid:
            logError("SDK didn't return a persona UUID")
            return

        persona_config = read_json_file_into_memory(sdk, "personas/race-personas.json")
        for persona_info in persona_config:
            persona = Persona()
            persona.setDisplayName(persona_info["displayName"])
            persona.setRaceUuid(persona_info["raceUuid"])
            persona.setPublicKey(int(persona_info["publicKey"]))

            key_path = "personas/" + persona_info["aesKeyFile"]
            persona.setAesKey(bytes(sdk.readFile(key_path)))

            if persona_info["personaType"].lower() == "client":
                persona.setPersonaType(PersonaType.P_CLIENT)
            elif persona_info["personaType"].lower() == "server":
                persona.setPersonaType(PersonaType.P_SERVER)
            else:
                logError(
                    f"No valid personaType specified: {persona_info['personaType']}"
                )
                return

            self._uuid_to_persona_map[persona_info["raceUuid"]] = persona

        try:
            self._race_persona = self._uuid_to_persona_map[active_uuid]
        except KeyError:
            logError(f"SDK didn't return a real persona: {active_uuid}")
            return

        logDebug(
            f"I am {self._race_persona.getDisplayName()}: "
            f"UUID = {self._race_persona.getRaceUuid()}"
        )

    def loadStaticLinks(self, sdk: IRaceSdkNM) -> bool:
        """
        Purpose:
            Load static links for this node from a config file
        Args:
            configFilePath (str): Path to the config file
        Returns:
            N/A
        """
        logInfo("loadStaticLinks: called")
        link_profiles_path = "link-profiles.json"
        logDebug(f"Reading JSON File Into Memory: {link_profiles_path}")
        self.staticLinkProfiles = read_json_file_into_memory(sdk, link_profiles_path)

        logInfo("loadStaticLinks: returned")

    def initStaticLinks(self, channel_id: str) -> bool:
        """
        Purpose:
            Initialize (create or load) static links for the specified channel
        Args:
            channel_id (str): Channel GID for which to initialize static links
        Returns:
            True if all static links were initialized
        """
        logInfo(f"initStaticLinks: called for {channel_id}")

        channelLinkProfiles = self.staticLinkProfiles.get(channel_id)
        if not channelLinkProfiles:
            logWarning(f"initStaticLinks: no links found for channel {channel_id}")
            # not a failure
            return True

        if (
            len(self._race_sdk.getLinksForChannel(channel_id))
            >= self._race_sdk.getChannelProperties(channel_id).maxLinks
        ):
            logWarning(
                "initStaticLinks: the number of links on channel: "
                + channel_id
                + " is at or exceeds the max number of links for the channel, please update config gen scripts to not fulfill more than the maximum number of links supported."
            )

        for linkProfile in channelLinkProfiles:
            if linkProfile.get("role") == "creator":
                logDebug(
                    f"initStaticLinks: creating link: {linkProfile.get('description')}"
                )
                response = self._race_sdk.createLinkFromAddress(
                    channel_id,
                    linkProfile.get("address"),
                    linkProfile.get("personas"),
                    0,
                )
                logDebug("initStaticLinks: creating link returned")
                if response.status != SDK_OK:
                    logError(
                        f"initStaticLinks: error creating link \
                            {linkProfile.get('description')} for channel {channel_id} with address \
                            {linkProfile.get('address')} \
                            , failed with sdk response status: {response.status}"
                    )
                    return False
                self.genesis_link_requests.add(response.handle)

            elif linkProfile.get("role") == "loader":
                logDebug(
                    f"initStaticLinks: loading link: {linkProfile.get('description')}"
                )

                response = self._race_sdk.loadLinkAddress(
                    channel_id,
                    linkProfile.get("address"),
                    linkProfile.get("personas"),
                    0,
                )
                if response.status != SDK_OK:
                    logError(
                        f"initStaticLinks: error loading link address for link \
                            {linkProfile.get('description')} for channel {channel_id} with address \
                            {linkProfile.get('address')} \
                            , failed with sdk response status: {response.status}"
                    )
                    return False
                self.genesis_link_requests.add(response.handle)
            else:
                logError(
                    f"initStaticLinks: unrecognized role {linkProfile.get('role')} for link \
                    {linkProfile.get('description')} for channel {channel_id}"
                )

        logInfo("initStaticLinks: returned")
        return True

    def openRecvConns(self, personas: List[Persona]) -> bool:
        """
        Purpose:
            Open receiving connections for each given persona.
        Args:
            personas (List[Persona]): Personas for which to open receiving connections
        Returns:
            False if an error was received from the SDK on openConnection call, otherwise True
        """
        logInfo("openRecvConns: called")
        link_type = LT_RECV
        links_to_open = set()

        for persona in personas:
            if persona == self._race_persona:
                continue

            uuid = persona.getRaceUuid()
            logDebug(f"openRecvConns: determining links to: {uuid}")
            links = self._race_sdk.getLinksForPersonas([uuid], link_type)
            if not links:
                logWarning(f"No links to receive from {uuid}")
                continue
            links_to_open.update(links)

        for link_id in links_to_open:
            hints = "{}"
            props = self._race_sdk.getLinkProperties(link_id)
            # NOTE: due to SWIG, you MUST assign the base return object from
            # getLinkProperties() and then access its fields, you CANNOT do:
            # hints = self._race_sdk.getLinkProperties(potentialLink).supported_hints
            if "polling_interval_ms" in props.supported_hints:
                hints = '{"polling_interval_ms": 500}'

            logDebug(f"openRecvConns: opening connection {link_id}")
            resp = self._race_sdk.openConnection(
                link_type, link_id, hints, 0, RACE_UNLIMITED, RACE_BLOCKING
            )
            if resp.status != SDK_OK:
                logError(f"openRecvConns: failed to open LinkID: {link_id}")
                return False

            self._opening_connections[resp.handle] = (Persona(), LT_RECV)

        logInfo("openRecvConns: returned")
        return True

    def closeRecvConns(self):
        """
        Purpose:
            Close receiving connections.
        Args:
            N/A
        Returns:
            N/A
        """
        logInfo("closeRecvConns: called")
        for conn_id in self._recv_connections:
            logDebug(f"Closing connection: {conn_id}")
            self._race_sdk.closeConnection(conn_id, 0)
        self._recv_connections = {}
        logInfo("closeRecvConns: returned")

    def invokeLinkWizard(self, personas: List[Persona]) -> bool:
        """
        Purpose:
            Invoke the LinkWizard if requested number of links are not already created
        Args:
            personas (List[Persona]): Personas for which to open receiving connections
        Returns:
            False if an error was received from the SDK on openConnection call, otherwise True
        """
        logInfo("invokeLinkWizard: called")
        link_type = LT_SEND

        for persona in personas:
            if self._race_persona == persona:
                continue

            existing_channels = set()
            existing_links = self._get_sorted_links(persona)
            for link_id in existing_links:
                link_props = self._race_sdk.getLinkProperties(link_id)
                existing_channels.add(link_props.channelGid)
            expected_links = self._expected_links.get(persona.getRaceUuid(), [])
            for _ in range(len(existing_channels), len(expected_links)):
                if not self._link_wizard.obtain_unicast_link(persona, LT_SEND):
                    logInfo("Invoking the LinkWizard")
                    self._obtain_unicast_link_to_retry[persona.getRaceUuid()] = LT_SEND
                else:
                    logWarning(
                        "LinkWizard not enabled, not creating prefered number of links"
                    )
                    break

        logInfo("invokeLinkWizard: returned")
        return True

    def compareLinkProperties(
        self, props1: LinkProperties, props2: LinkProperties, persona_type: PersonaType
    ) -> int:
        """
        Purpose:
            Compare two sets of LinkProperties, in the context of using them to send to a particular PersonaType
        Args:
            pair1 (LinkProperties): LinkProperties of first link
            pair2 (LinkProperties): LinkProperties of second link
            persona_type (PersonaType): PersonaType to use the link to send to
        Returns:
            Int: -1 if pair1 is superior, 1 if pair2 is superior
        """
        raise NotImplementedError("Subclasses must implement this")

    def sendFormattedMsg(
        self, dest_uuid: str, msg: str, trace_id: int, span_id: int, link_rank: int = 0
    ) -> bool:
        """
        Purpose:
            Sends a stringified message to the specified destination persona.
        Args:
            dest_uuid: The uuid of the persona to send to
            msg: The stringified message to encrypt and send
            trace_id: The OpenTracing traceId of the original received EncPkg to continue on
            span_id: The OpenTracing spanId of the original received EncPkg to continue on
        Returns:
            True for success, false for failure
        """
        dest_persona = self._uuid_to_persona_map.get(dest_uuid, None)
        if not dest_persona:
            logError(
                f"Failed to find destination UUID {dest_uuid} in self._uuid_to_persona_map"
            )
            return False

        enc_pkg = EncPkg(
            trace_id,
            span_id,
            self.encryptor.encryptClrMsg(msg, dest_persona.getAesKey()),
        )

        return self.sendTo(dest_persona, enc_pkg)

    def sendTo(self, persona: Persona, pkg: EncPkg, conn_idx=0) -> bool:
        """
        Purpose:
            Send encrypted package to the specified persona
        Args:
            persona (Persona): Persona to which to send the package
            pkg (EncPkg): Encrypted package to be sent
            conn_idx (Int): idx into the sorted list of connections to use, defaults to 0, otherwise % by length of list to remain valid
        Returns:
            The handle returned from the SDK on the API call to sendEncryptedPackage, or NULL_RACE_HANDLE on error.
        """
        uuid = persona.getRaceUuid()
        conn_id = None
        if uuid in self._uuid_to_send_connections_map:
            ranked_conn_ids = self._uuid_to_send_connections_map[uuid]
            conn_id = ranked_conn_ids[conn_idx % len(ranked_conn_ids)][0]
        else:
            logError(f"No connections to {uuid} found, not sending message")
            return NULL_RACE_HANDLE

        logDebug(f"Sending package on {conn_id} to {uuid}")
        resp = self._race_sdk.sendEncryptedPackage(pkg, conn_id, 0, 0)
        if resp.status != SDK_OK:
            logError("Failed to send package")
            return NULL_RACE_HANDLE

        return resp.handle

    def _handleConnectionOpened(
        self, handle: int, conn_id: str, properties: LinkProperties
    ) -> PluginResponse:
        """
        Purpose:
            Notify network manager about an opened connection.
        Args:
            handle (int): The RaceHandle of the original openConnection or closeConnection
                call, if that is what caused the change. Otherwise, 0.
            conn_id (str): The ConnectionID of the connection
            properties (LinkProperties): Properties of the link
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        persona, link_type = self._opening_connections.pop(
            handle, (Persona(), LT_UNDEF)
        )
        uuid = persona.getRaceUuid()
        logDebug(f"connection opened for persona {uuid} of type {link_type}")

        if link_type == LT_RECV:
            self._recv_connections.add(conn_id)
            logDebug(f"receive connection opened: {conn_id}")
        else:
            if properties.linkType == LT_SEND and persona == Persona():
                logError(
                    "Opened LT_SEND connection but no persona was associated with it"
                )
                return PLUGIN_ERROR
            elif persona != Persona():
                self._uuid_to_send_connections_map[uuid] = sorted(
                    self._uuid_to_send_connections_map.get(uuid, [])
                    + [(conn_id, properties)],
                    key=cmp_to_key(
                        lambda pair1, pair2: self.compareLinkProperties(
                            pair1[1], pair2[1], persona.getPersonaType()
                        )
                    ),
                )
                self._send_connection_to_uuid_map[conn_id] = uuid
                logDebug(f"unicast send opened: {conn_id} to {uuid}")

                # Maybe this new connection allows a previously failed LinkWizard function
                if self._obtain_unicast_link_to_retry.get(uuid):
                    self._link_wizard.obtain_unicast_link(
                        persona, self._obtain_unicast_link_to_retry[uuid]
                    )
                    del self._obtain_unicast_link_to_retry[uuid]

        return PLUGIN_OK

    def _handleConnectionClosed(
        self, handle: int, conn_id: str, link_id: str, properties: LinkProperties
    ) -> PluginResponse:
        """
        Purpose:
            Notify network manager about a closed connection.
        Args:
            handle (int): The RaceHandle of the original openConnection or closeConnection
                call, if that is what caused the change. Otherwise, 0.
            conn_id (str): The ConnectionID of the connection
            link_id (str): The LinkID of the link this connection is on
            properties (LinkProperties): Properties of the link
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        persona, link_type = self._opening_connections.pop(
            handle, (Persona(), LT_UNDEF)
        )
        if conn_id in self._recv_connections:
            self._recv_connections.discard(conn_id)
            logDebug(f"receive closed {conn_id}")
        else:
            # If it was not a recv connection, it must be a send
            uuid = None
            if conn_id not in self._send_connection_to_uuid_map:
                logError(f"Could not find UUID for closed connection {conn_id}")
                return PLUGIN_ERROR

            uuid = self._send_connection_to_uuid_map[conn_id]
            del self._send_connection_to_uuid_map[conn_id]

            # remove this connection-properties pair from the sorted list
            self._uuid_to_send_connections_map[uuid] = [
                (cid, props)
                for cid, props in self._uuid_to_send_connections_map[uuid]
                if cid != conn_id
            ]
            if uuid not in self._uuid_to_persona_map:
                logError(
                    f"Could not find persona for UUID {uuid} on closed connection, not attempting to open new connection to replace it"
                )
                return PLUGIN_ERROR

            persona = self._uuid_to_persona_map[uuid]
            potential_links = self._get_sorted_links(persona)
            # remove links we are already using to connect to this persona
            for openId, _ in self._uuid_to_send_connections_map[uuid]:
                link = self._race_sdk.getLinkForConnection(openId)
                try:
                    potential_links.remove(link)
                except ValueError:
                    pass

            if len(potential_links) == 0:
                logWarning(
                    "Could not find a link to reach persona " + persona.getRaceUuid()
                )
                return PLUGIN_OK

            chosen_link_id = potential_links[0]
            hints = "{}"
            props = self._race_sdk.getLinkProperties(chosen_link_id)
            # NOTE: due to SWIG, you MUST assign the base return object from
            # getLinkProperties() and then access its fields, you CANNOT do:
            # hints = self._race_sdk.getLinkProperties(potentialLink).supported_hints
            if "batch" in props.supported_hints:
                hints = '{"batch": true}'

            resp = self._race_sdk.openConnection(
                LT_SEND, chosen_link_id, hints, 0, RACE_UNLIMITED, RACE_BLOCKING
            )
            if resp.status != SDK_OK:
                logError(f"invokeLinkWizard: failed to open LinkID: {chosen_link_id}")
                return False

            self._opening_connections[resp.handle] = (persona, LT_SEND)

        return PLUGIN_OK

    def _get_sorted_links(self, persona: Persona) -> List[str]:
        """
        Purpose:
            Get a sorted list of send links for the persona
        Args:
            persona (Persona): Persona to get links for
        Returns:
            List of send links sorted based on the compareLinkProperties function
        """
        potential_link_prop_pairs = [
            (link, self._race_sdk.getLinkProperties(link))
            for link in self._race_sdk.getLinksForPersonas(
                [persona.getRaceUuid()], LT_SEND
            )
        ]
        # sort the links in order of goodness based on compareLinkProperties
        # with the context of what type of node (client vs. server) they are
        # connecting to
        potential_link_prop_pairs = sorted(
            potential_link_prop_pairs,
            key=cmp_to_key(
                lambda pair1, pair2: self.compareLinkProperties(
                    pair1[1], pair2[1], persona.getPersonaType()
                )
            ),
        )
        potential_links = [
            link_id
            for link_id, props in potential_link_prop_pairs
            if props.connectionType != CT_UNDEF
        ]
        return potential_links

    def prepareToBootstrap(
        self, handle: int, link_id: str, config_path: str, device_info: str
    ):
        return PLUGIN_OK

    def onBootstrapKeyReceived(self, persona: str, key: ByteVector):
        return PLUGIN_OK

    def onConnectionStatusChanged(
        self,
        handle: int,
        conn_id: str,
        status: int,
        link_id: str,
        properties: LinkProperties,
    ) -> PluginResponse:
        """
        Purpose:
            Notify network manager about a change in the status of a connection. The handle will
            correspond to a handle returned inside an SdkResponse to a previous
            openConnection or closeConnection call.
        Args:
            handle (int): The RaceHandle of the original openConnection or closeConnection
                call, if that is what caused the change. Otherwise, 0.
            conn_id (str): The ConnectionID of the connection
            status (int): The ConnectionStatus of the connection updated
            link_id (str): The LinkID of the link this connection is on
            properties (LinkProperties): Properties of the link
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        try:
            logInfo("onConnectionStatusChanged: called")
            logDebug(
                f"onConnectionStatusChanged: handle={handle} conn_id={conn_id} status={status}"
            )
            result = PLUGIN_OK
            if status == CONNECTION_OPEN:
                result = self._handleConnectionOpened(handle, conn_id, properties)
            elif status == CONNECTION_CLOSED:
                result = self._handleConnectionClosed(
                    handle, conn_id, link_id, properties
                )

            # All openConnection calls have been resolved - it is possible they were resolved by
            # CONNECTON_CLOSED responses in which case we might not actually be ready. CPP has a
            # more thorough connectivity check.
            if len(self._opening_connections) == 0:
                self._race_sdk.onPluginStatusChanged(PLUGIN_READY)
                self._race_sdk.displayInfoToUser("network manager is ready", UD_TOAST)

            return result
        except Exception as e:
            logError("".join(traceback.TracebackException.from_exception(e).format()))
            return PLUGIN_FATAL

    def onLinkPropertiesChanged(
        self, link_id: str, link_properties: LinkProperties
    ) -> PluginResponse:
        """
        Purpose:
            Notify network manager about a change to the LinkProperties of a Link.
        Args:
            link_id (str): The LinkID of the link that has been updated
            link_properties (LinkProperties): The LinkProperties that were updated
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        try:
            logInfo("onLinkPropertiesChanged: called")
            logDebug(
                f"onLinkPropertiesChanged: link_id={link_id} link_properties={link_properties}"
            )
            logInfo("onLinkPropertiesChanged: returned")
            return PLUGIN_OK
        except Exception as e:
            logError("".join(traceback.TracebackException.from_exception(e).format()))
            return PLUGIN_FATAL

    def onPersonaLinksChanged(
        self, recipient_persona: str, linkType: int, links: List[str]
    ) -> PluginResponse:
        """
        Purpose:
            Notify network manager about a change to the Links associated with a Persona
        Args:
            recipient_persona (str): The Persona that has changed link associations
            linkType (int): The LinkType of the links (send, recv, bidi)
            links (List[str]): The list of links that are now associated with this persona
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        try:
            logInfo("onPersonaLinksChanged: called")
            logDebug(
                f"onPersonaLinksChanged: recipient_persona={recipient_persona} linkType={linkType} links={links}"
            )
            logInfo("onPersonaLinksChanged: returned")
            return PLUGIN_OK
        except Exception as e:
            logError("".join(traceback.TracebackException.from_exception(e).format()))
            return PLUGIN_FATAL

    def onChannelStatusChanged(
        self, handle: int, channelGid: str, status: int, properties: ChannelProperties
    ):
        try:
            if status == CHANNEL_AVAILABLE:
                self.initStaticLinks(channelGid)
                self.channels_to_use.discard(channelGid)
                if not self.channels_to_use:
                    logInfo("All expected channels are now available")
            self._link_wizard.handle_channel_status_update(handle, channelGid, status)
            return PLUGIN_OK
        except Exception as e:
            logError("".join(traceback.TracebackException.from_exception(e).format()))
            return PLUGIN_FATAL

    def onLinkStatusChanged(
        self, handle: int, linkId: str, status: int, properties: LinkProperties
    ):
        try:
            logDebug(f"onLinkStatusChanged: called for link: {linkId} status: {status}")
            self._link_wizard.handle_link_status_update(
                handle, linkId, status, properties
            )
            if status == LINK_CREATED:
                uuid_list = self._race_sdk.getPersonasForLink(linkId)
                for uuid in uuid_list:
                    logInfo(f"Opening LinkID: {linkId} for {uuid}")

                    hints = "{}"
                    if "batch" in properties.supported_hints:
                        hints = '{"batch": true}'

                    response = self._race_sdk.openConnection(
                        properties.linkType, linkId, hints, 0, RACE_UNLIMITED, 0
                    )
                    if response.status != SDK_OK:
                        logError(
                            f"onLinkStatusChanged LINK_CREATED failed to open LinkID: {linkId}"
                        )
                        return PLUGIN_OK

                    persona = self._uuid_to_persona_map.get(uuid, Persona())
                    self._opening_connections[response.handle] = (
                        persona,
                        properties.linkType,
                    )

            elif status == LINK_LOADED:
                uuid_list = self._race_sdk.getPersonasForLink(linkId)
                for uuid in uuid_list:
                    logInfo(f"Opening LinkID: {linkId} for {uuid}")

                    hints = "{}"
                    if "batch" in properties.supported_hints:
                        hints = '{"batch": true}'

                    response = self._race_sdk.openConnection(
                        properties.linkType, linkId, hints, 0, RACE_UNLIMITED, 0
                    )
                    if response.status != SDK_OK:
                        logError(
                            f"onLinkStatusChanged LINK_LOADED failed to open LinkID: {linkId}"
                        )
                        return PLUGIN_OK

                    persona = self._uuid_to_persona_map.get(uuid, Persona())
                    self._opening_connections[response.handle] = (
                        persona,
                        properties.linkType,
                    )

            elif status == LINK_DESTROYED:
                # TODO
                pass
            else:
                logWarning("onLinkStatusChanged: received invalid LinkStatus")

            self.genesis_link_requests.discard(handle)
            if (
                (not self.link_wizard_initialized)
                and (not self.genesis_link_requests)
                and (not self.channels_to_use)
            ):
                self.link_wizard_initialized = True
                self.invokeLinkWizard(self._uuid_to_persona_map.values())

            logDebug("onLinkStatusChanged: returned")
            return PLUGIN_OK
        except Exception as e:
            logError("".join(traceback.TracebackException.from_exception(e).format()))
            return PLUGIN_FATAL

    def onUserInputReceived(
        self,
        handle: int,
        answered: bool,
        response: str,
    ) -> PluginResponse:
        """
        Purpose:
            Notify network manager about received user input response
        Args:
            handle: The handle for this callback
            answered: True if the response contains an actual answer to the input prompt, otherwise
                the response is an empty string and not valid
            response: The user response answer to the input prompt
        Returns:
            PluginResponse: The status of the Plugin in response to this call
        """
        try:
            logInfo("onUserInputReceived: called")
            return PLUGIN_OK
        except Exception as e:
            logError("".join(traceback.TracebackException.from_exception(e).format()))
            return PLUGIN_FATAL

    def notifyEpoch(self, data: str) -> PluginResponse:
        """
        Purpose:
            Notify network manager to perform epoch changeover processing
        Args:
            data: Data associated with epoch (network manager implementation specific)
        Returns:
            PluginResponse: The status of the Plugin in response to this call
        """
        logDebug(f"notifyEpoch: called with data={data}")
        return PLUGIN_OK

    def onUserAcknowledgementReceived(
        self,
        handle: int,
    ) -> int:
        """
        Purpose:
            Notify the plugin that the user acknowledged the displayed information

        Args:
            handle: The handle for asynchronously returning the status of this call.

        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        logDebug(f"onUserAcknowledgementReceived: called")
        return PLUGIN_OK
