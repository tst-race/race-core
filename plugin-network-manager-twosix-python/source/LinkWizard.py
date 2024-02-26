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
    Python implementation of the network manager dynamic link wizard.
"""

# System Python Library Imports
import json
from typing import Dict, List, Tuple

# RACE Python Library Imports
from networkManagerPluginBindings import ChannelProperties, IRaceSdkNM, LinkProperties
from networkManagerPluginBindings import CT_DIRECT, CT_INDIRECT, CT_UNDEF  # ConnectionType
from networkManagerPluginBindings import (
    LINK_CREATED,
    LINK_DESTROYED,
    LINK_LOADED,
)  # LinkStatus
from networkManagerPluginBindings import (
    LD_BIDI,
    LD_CREATOR_TO_LOADER,
    LD_LOADER_TO_CREATOR,
)  # LinkDirection
from networkManagerPluginBindings import LT_RECV, LT_SEND  # LinkType
from networkManagerPluginBindings import LS_BOTH, LS_CREATOR, LS_LOADER, LS_UNDEF  # LinkSide
from networkManagerPluginBindings import SDK_OK  # SdkStatus

# Local Python Library Imports
from .ExtClrMsg import ExtClrMsg
from .Log import logDebug, logError, logInfo
from .Persona import Persona, PersonaType
from .RaceCrypto import RaceCrypto


# mypy type aliases
ChannelStatus = int
LinkSide = int
LinkStatus = int
LinkType = int
RaceHandle = int


# Human-friendly versions of enum values
CONNECTION_TYPE = ["CT_UNDEF", "CT_DIRECT", "CT_INDIRECT", "CT_MIXED", "CT_LOCAL"]
LINK_DIRECTION = ["LD_UNDEF", "LD_CREATOR_TO_LOADER", "LD_LOADER_TO_CREATOR", "LD_BIDI"]
LINK_SIDE = ["LS_UNDEF", "LS_CREATOR", "LS_LOADER", "LS_BOTH"]
LINK_STATUS = ["LINK_UNDEF", "LINK_CREATED", "LINK_LOADED", "LINK_DESTROYED"]
LINK_TYPE = ["LT_UNDEF", "LT_SEND", "LT_RECV", "LT_BIDI"]


class LinkWizard:
    """
    network manager dynamic link wizard
    """

    def __init__(
        self, race_uuid: str, persona_type: PersonaType, messenger, sdk: IRaceSdkNM
    ) -> None:
        """
        Purpose:
            Initialize the link wizard
        Args:
            race_uuid: Current node RACE UUID
            persona_type: Current node persona type
            messenger: RACE message sender
            sdk: RACE Network Manager SDK
        Returns:
            N/A
        """
        self._race_uuid = race_uuid
        self._persona_type = persona_type
        self._messenger = messenger
        self._sdk = sdk

        self._encryptor = RaceCrypto()

        self._uuid_to_supported_channels_map: Dict[str, Dict[str, LinkSide]] = {}
        self._request_map: Dict[str, List[LinkType]] = {}
        self._pending_create: Dict[RaceHandle, List[str]] = {}
        self._pending_load: Dict[RaceHandle, List[str]] = {}

    def process_link_msg(self, sender: Persona, msg: str) -> bool:
        """
        Purpose:
            Handle a LinkWizard protocol message, potentially triggering creation/loading
            of new links and/or sending messages to other personas
        Args:
            sender: The persona that sent the message to us
            msg: The string of the message itself (expected to be JSON)
        Returns:
            Whether the parsing and attendant actions of the message happened successfully.
            No remedy action by teh caller is expected.
        """
        logDebug(
            f"  ━☆ LinkWizard::process_link_msg: called from uuid {sender.getRaceUuid()} "
            f"msg: {msg}"
        )
        try:
            uuid = sender.getRaceUuid()
            msg_json = json.loads(msg)
            if msg_json.get("getSupportedChannels"):
                self._respond_supported_channels(uuid)

            if msg_json.get("supportedChannels"):
                # Update map of UUIDs to channels
                self._uuid_to_supported_channels_map[uuid] = msg_json.get(
                    "supportedChannels"
                )
                logDebug(
                    "  ━☆ LinkWizard::process_link_msg: updated supported channels for "
                    f"{uuid} to: {self._uuid_to_supported_channels_map[uuid]}"
                )

                # If we were previously waiting to get this info, execute the delayed
                # request
                if self._request_map.get(uuid):
                    for link_type in self._request_map.get(uuid, []):
                        # Just replicate the call we originally delayed
                        self.obtain_unicast_link(sender, link_type)
                    del self._request_map[uuid]

            if msg_json.get("requestCreateLink"):
                self._handle_create_link_request(
                    uuid, msg_json.get("requestCreateLink")
                )

            if msg_json.get("requestLoadLinkAddress"):
                request = msg_json.get("requestLoadLinkAddress")
                self._handle_load_link_address_request(
                    uuid,
                    request.get("channelGid", ""),
                    request.get("address", ""),
                    request.get("personas", []),
                )

        except json.JSONDecodeError as exc:
            logError(
                f"  ━☆ LinkWizard::process_link_msg: Error parsing JSON: {msg} failed "
                f"with error: {exc.msg}"
            )

        logDebug("  ━☆ LinkWizard::process_link_msg: returned")
        return True

    def obtain_unicast_link(self, persona: Persona, link_type: LinkType) -> bool:
        """
        Purpose:
            Attempt to construct a new unicast link with the persona of the type specified.
            If link_type is LT_BIDI this may cause creation of two unidirectional links.
            This returns false if this node cannot send to persona, in which case it should
            be retried when that is possible. If the persona _can_ be reached and false is
            returned, this indicates there are no compatible channels to establish a link
            on.
        Args:
            persona: The Persona to establish a unicast link with
            link_type: The type of link to obtain (e.g. LT_RECV; LT_SEND; LT_BIDI)
        Returns:
            Success of the operation.
        """
        logDebug(
            "  ━☆ LinkWizard::obtain_unicast_link: called for uuid "
            f"{persona.getRaceUuid()} for LinkType: {LINK_TYPE[link_type]}"
        )
        uuid = persona.getRaceUuid()

        # Do we know what channels this persona supports?
        if not self._uuid_to_supported_channels_map.get(uuid):
            # Ask the persona what channels they support
            if self._query_supported_channels(uuid):
                # Queue this request so we can re-process after we get a response
                self._request_map.setdefault(uuid, [])
                self._request_map.get(uuid).append(link_type)
                return True
            else:
                # Else no connections are open yet, not able to query
                return False
        else:
            return self._internal_obtain_unicast_link(persona, link_type)

    def _internal_obtain_unicast_link(self, persona: Persona, link_type: LinkType):
        """
        Purpose:
            Internal helper function for obtaining a unicast link. Specifically, performs
            link selection and either calls createLink() and queues an action to send the
            LinkAddress when the link is created; or sends a LinkWizard message to persona
            to ask them to create a link and share the LinkAddress.
        Args:
            persona: The Persona to establish a unicast link with
            link_type: The type of link to obtain (e.g. LT_RECV; LT_SEND; LT_BIDI)
        Returns:
            Success of the operation. This could indicate either that the persona is
            unreachable or that there are no compatible channels
        """
        uuid = persona.getRaceUuid()
        logDebug(
            f"  ━☆ LinkWizard::_internal_obtain_unicast_link: called for uuid {uuid}"
        )

        their_channel_gids = self._uuid_to_supported_channels_map.get(uuid, {})
        potential_channel_props: Dict[
            str, ChannelProperties
        ] = self._sdk.getSupportedChannels()

        # Special case: we want bidirectional.
        if link_type == LD_BIDI:
            bidi_channel_props = {}
            for (channel_gid, channel_props) in potential_channel_props.items():
                if channel_props.linkDirection == LD_BIDI:
                    bidi_channel_props[channel_gid] = channel_props
            potential_channel_props = bidi_channel_props

            # If no channels were bidirectional, recurse to try satisfying with
            # separate LT_RECV and LT_SEND links
            if not potential_channel_props:
                # TODO handle cleanup if one of these fails
                recv_success = self._internal_obtain_unicast_link(persona, LT_RECV)
                send_success = self._internal_obtain_unicast_link(persona, LT_SEND)
                logDebug("  ━☆ LinkWizard::_internal_obtain_unicast_link: returned")
                return recv_success and send_success

        supported_potential_channels = {}
        for (channel_gid, channel_props) in potential_channel_props.items():
            logDebug(
                "  ━☆ LinkWizard::_internal_obtain_unicast_link: potential channel "
                f"{channel_gid} LinkDirection: "
                f"{LINK_DIRECTION[channel_props.linkDirection]}"
            )
            if channel_gid in their_channel_gids:
                link_direction = channel_props.linkDirection
                our_link_side = channel_props.currentRole.linkSide
                their_link_side = their_channel_gids[channel_gid]

                # In order to create a link:
                # 1. we have to support creating while they have to support loading, and
                # 2. we have to be either:
                #   a. using a bidirectional link, or
                #   b. sending over a creator-to-loader link, or
                #   c. receiving over a loader-to-creator link
                if (
                    our_link_side in [LS_CREATOR, LS_BOTH]
                    and their_link_side in [LS_LOADER, LS_BOTH]
                    and (
                        link_direction == LD_BIDI
                        or (
                            link_type == LT_SEND
                            and link_direction == LD_CREATOR_TO_LOADER
                        )
                        or (
                            link_type == LT_RECV
                            and link_direction == LD_LOADER_TO_CREATOR
                        )
                    )
                ):
                    supported_potential_channels[channel_gid] = (
                        channel_props,
                        LS_CREATOR,
                    )
                # In order to load a link:
                # 1. we have to support loading while they have to support creating, and
                # 2. we have to be either:
                #   a. using a bidirectional link, or
                #   b. sending over a loader-to-creator link, or
                #   c. receiving over a creator-to-loader link
                elif (
                    our_link_side in [LS_LOADER, LS_BOTH]
                    and their_link_side in [LS_CREATOR, LS_BOTH]
                    and (
                        link_direction == LD_BIDI
                        or (
                            link_type == LT_SEND
                            and link_direction == LD_LOADER_TO_CREATOR
                        )
                        or (
                            link_type == LT_RECV
                            and link_direction == LD_CREATOR_TO_LOADER
                        )
                    )
                ):
                    supported_potential_channels[channel_gid] = (
                        channel_props,
                        LS_LOADER,
                    )

        if self._persona_type == PersonaType.P_CLIENT:
            best_channel = self._get_preferred_channel_gid_for_sending_to_persona(
                supported_potential_channels, PersonaType.P_CLIENT
            )
        else:
            best_channel = self._get_preferred_channel_gid_for_sending_to_persona(
                supported_potential_channels, persona.getPersonaType()
            )

        (best_channel_id, best_channel_props, best_channel_link_side) = best_channel

        if not best_channel_id:
            logInfo(
                "  ━☆ LinkWizard: Could not find a channel candidate to connect to "
                f"{uuid}"
            )
            logDebug("  ━☆ LinkWizard::_internal_obtain_unicast_link: returned")
            return False

        if (
            len(self._sdk.getLinksForChannel(best_channel_id))
            >= self._sdk.getChannelProperties(best_channel_id).maxLinks
        ):
            logError(
                "  ━☆ LinkWizard::_internal_obtain_unicast_link: error creating links for channel "
                + best_channel_id
                + " because the number of links on the channel is at or exceeds the max number of links for the channel"
            )
            return False

        logDebug(
            f"  ━☆ LinkWizard: bestChannel: {best_channel_id} LinkSide: "
            f"{LINK_SIDE[best_channel_link_side]} LinkDirection: "
            f"{LINK_DIRECTION[best_channel_props.linkDirection]} required LinkType: "
            f"{LINK_TYPE[link_type]}"
        )
        if best_channel_link_side == LS_CREATOR:
            logDebug("  ━☆ LinkWizard: creating link")
            response = self._sdk.createLink(best_channel_id, [uuid], 0)
            if response.status != SDK_OK:
                logError(
                    "  ━☆ LinkWizard: Error creating link for channel GID: "
                    f"{best_channel_id} failed with sdk response status: "
                    f"{response.status}"
                )
                logDebug("  ━☆ LinkWizard::_internal_obtain_unicast_link: returned")
                return False
            self._pending_create[response.handle] = [uuid]

        elif best_channel_link_side == LS_LOADER:
            logDebug(f"  ━☆ LinkWizard: requesting link creation")
            # we want to be the loader, so prompt the other node to create
            self._request_create_link(uuid, best_channel_id)

        else:
            logError(f"  ━☆ LinkWizard: invalid link side: {best_channel_link_side}")

        logDebug("  ━☆ LinkWizard::_internal_obtain_unicast_link: returned")
        return True

    def _request_create_link(self, uuid: str, channel_gid: str) -> bool:
        """
        Purpose:
            Send a LinkWizard message requesting the uuid to create a link of channel_gid
            and share the LinkAddress back.
        Args:
            uuid: The string uuid of the persona to request to
            channel_gid: The name of the channel to request be created
        Returns:
            Success of the operation. Failure indicates the message could not be sent
        """
        logDebug(
            f"  ━☆ LinkWizard::_request_create_link: called for uuid {uuid} on channel "
            f"{channel_gid}"
        )
        msg_json = json.dumps(
            {
                "requestCreateLink": channel_gid,
            }
        )
        msg = ExtClrMsg(
            msg=msg_json,
            _from=self._race_uuid,
            to=uuid,
            msgTime=1,  # TODO fix timestamp
            msgNonce=0,  # TODO fix nonce
            msgAmpIndex=0,
            uuid=0,
            ringTtl=0,
            ringIdx=0,
            msgType=ExtClrMsg.MSG_LINKS,
        )
        msg_str = self._encryptor.formatDelimitedMessage(msg)
        success = self._messenger.sendFormattedMsg(
            uuid, msg_str, msg.getTraceId(), msg.getSpanId()
        )
        logDebug("  ━☆ LinkWizard::_request_create_link: returned")
        return success

    def _handle_create_link_request(self, uuid: str, channel_gid: str) -> bool:
        """
        Purpose:
            Handle a createLink request message. Create the link if we can and then queue
            an action to send the LinkAddress back to uuid when the link is created.
        Args:
            uuid: The string uuid of the persona requesting the create
            channel_gid: The name of the channel to create
        Returns:
            Success of the operation. Failure indicates the channel is not supported or
            could not be created
        """
        logDebug(
            f"  ━☆ LinkWizard::_handle_create_link_request: called for uuid {uuid} on "
            f"channel {channel_gid}"
        )
        if channel_gid not in self._sdk.getSupportedChannels():
            logError(f"  ━☆ LinkWizard::Requested link is not supported {channel_gid}")
            return False

        if (
            len(self._sdk.getLinksForChannel(channel_gid))
            >= self._sdk.getChannelProperties(channel_gid).maxLinks
        ):
            logError(
                "  ━☆ LinkWizard::_handle_create_link_request: error creating links for channel "
                + channel_gid
                + " because the number of links on the channel is at or exceeds the max number of links for the channel"
            )
            return False

        response = self._sdk.createLink(channel_gid, [uuid], 0)
        if response.status != SDK_OK:
            logError(f"  ━☆ LinkWizard::Error creating link {channel_gid}")
            return False

        self._pending_create[response.handle] = [uuid]

        logDebug("  ━☆ LinkWizard::_handle_create_link_request: returned")
        return True

    def _request_load_link_address(
        self, uuid: str, channel_gid: str, link_address: str, personas: List[str]
    ) -> bool:
        """
        Purpose:
            Send a LinkWizard message requesting the uuid to load a LinkAddress.
        Args:
            uuid: The string uuid of the persona to request
            channel_gid: The name of the channel to call loadLinkAddress on
            link_address: The LinkAddress to load
            personas: The list of persona uuid's that should be associated with this
                loaded link (for multicast groups)
        Returns:
            Success of the operation. Failure indicates the message could not be sent.
        """
        logDebug(
            f"  ━☆ LinkWizard::_request_load_link_address: called for uuid {uuid} on "
            f"channel {channel_gid}"
        )
        msg_json = json.dumps(
            {
                "requestLoadLinkAddress": {
                    "channelGid": channel_gid,
                    "address": link_address,
                    "personas": personas,
                },
            }
        )
        msg = ExtClrMsg(
            msg=msg_json,
            _from=self._race_uuid,
            to=uuid,
            msgTime=1,  # TODO fix timestamp
            msgNonce=0,  # TODO fix nonce
            msgAmpIndex=0,
            uuid=0,
            ringTtl=0,
            ringIdx=0,
            msgType=ExtClrMsg.MSG_LINKS,
        )
        msg_str = self._encryptor.formatDelimitedMessage(msg)
        success = self._messenger.sendFormattedMsg(
            uuid, msg_str, msg.getTraceId(), msg.getSpanId()
        )
        logDebug("  ━☆ LinkWizard::_request_load_link_address: returned")
        return success

    def _handle_load_link_address_request(
        self, uuid: str, channel_gid: str, link_address: str, personas: List[str]
    ) -> bool:
        """
        Purpose:
            Handle a loadLinkAddress request. Call loadLinkAddress with the passed
            channel_gid and link_address and the personas with our own persona removed
            from it (if it was present).
        Args:
            uuid: The string uuid of the persona requesting the load
            channel_gid: The name of the channel to load
            link_address: The LinkAddress to load
            personas: The list of persona uuid's that are involved in this link. This may
                contain our own uuid, which should be removed before calling
                loadLinkAddress.
        Returns:
            Success of the operation. Failure indicates the channel is not supported or
            failed to load.
        """
        logDebug(
            "  ━☆ LinkWizard::_handle_load_link_address_request: called for uuid "
            f"{uuid} on channel {channel_gid}"
        )
        if channel_gid not in self._sdk.getSupportedChannels():
            logError(f"  ━☆ LinkWizard::Requested link is not supported {channel_gid}")
            return False

        if (
            len(self._sdk.getLinksForChannel(channel_gid))
            >= self._sdk.getChannelProperties(channel_gid).maxLinks
        ):
            logError(
                "  ━☆ LinkWizard::_handle_load_link_address_request: error loading links for channel "
                + channel_gid
                + " because the number of links on the channel is at or exceeds the max number of links for the channel"
            )
            return False

        # erase our own UUID from the list of personas reached by this link
        if self._race_uuid in personas:
            personas.remove(self._race_uuid)
        response = self._sdk.loadLinkAddress(channel_gid, link_address, personas, 0)
        if response.status != SDK_OK:
            logError(
                f"  ━☆ LinkWizard: Error loading link address for channel {channel_gid} "
                f"with address {link_address} failed with sdk response status: "
                f"{response.status}"
            )
            return False

        self._pending_load[response.handle] = [uuid]

        logDebug("  ━☆ LinkWizard::_handle_load_link_address_request: returned")
        return True

    def _get_preferred_channel_gid_for_sending_to_persona(
        self,
        potential_channel_props: Dict[str, Tuple[ChannelProperties, LinkSide]],
        recipient_persona_type: PersonaType,
    ) -> Tuple[str, ChannelProperties, LinkSide]:
        """
        Purpose:
            Get the preferred channel for sending to a type of persona, i.e. client or
            server.
        Args:
            potential_channel_props: The potential channels for connecting to the persona.
            recipient_persona_type: The type of persona being connected to.
        Returns:
            The ID of the preferred channel, or empty string on error, and its associated
            properties.
        """
        logDebug(
            "  ━☆ LinkWizard::_get_preferred_channel_gid_for_sending_to_persona: "
            f"{len(potential_channel_props)} to choose from"
        )

        best_channel_id = ""
        best_channel_props = ChannelProperties()
        best_channel_link_side = LS_UNDEF
        for (
            channel_id,
            (channel_props, channel_link_side),
        ) in potential_channel_props.items():
            send_bandwidth = (
                channel_props.loaderExpected.send.bandwidth_bps
                if channel_link_side == LS_LOADER
                else channel_props.creatorExpected.send.bandwidth_bps
            )
            logDebug(
                f"  ━☆ LinkWizard: channel: {channel_id} connection type: "
                f"{CONNECTION_TYPE[channel_props.connectionType]} send bandwidth: {send_bandwidth}"
            )
            if self._rank_channel_properties(
                (channel_props, channel_link_side),
                (best_channel_props, best_channel_link_side),
                recipient_persona_type,
            ):
                best_channel_id = channel_id
                best_channel_props = channel_props
                best_channel_link_side = channel_link_side
                logDebug(f"  ━☆ LinkWizard::New Best ID: {best_channel_id}")

        if (
            best_channel_props.connectionType == CT_DIRECT
            and recipient_persona_type == PersonaType.P_CLIENT
        ):
            logError("  ━☆ LinkWizard::Client link could not find CT_INDIRECT channel ")
            return ("", ChannelProperties(), LS_UNDEF)
        return (best_channel_id, best_channel_props, best_channel_link_side)

    def _rank_channel_properties(
        self,
        lhs: Tuple[ChannelProperties, LinkSide],
        rhs: Tuple[ChannelProperties, LinkSide],
        recipient_persona_type: PersonaType,
    ) -> bool:
        """
        Purpose:
            Comparator for two ChannelProperties.
            For clients it prefers CT_INDIRECT, then not CT_UNDEF, then expected send
            bandwidth.
            For servers, it prefers not CT_UNDEF, then expected send bandwidth.
        Args:
            lhs: ChannelProperties of first channel
            rhs: ChannelProperties of second channel
            recipient_persona_type: PersonaType of the destination of the channel
        Returns:
            True if lhs is higher-priority than rhs, false otherwise
        """
        if recipient_persona_type == PersonaType.P_CLIENT:
            # prefer indirect for clients
            if (
                lhs[0].connectionType == CT_INDIRECT
                and rhs[0].connectionType != CT_INDIRECT
            ):
                return True
            if (
                rhs[0].connectionType == CT_INDIRECT
                and lhs[0].connectionType != CT_INDIRECT
            ):
                return False
        lhs_bandwidth = (
            lhs[0].loaderExpected.send.bandwidth_bps
            if lhs[1] == LS_LOADER
            else lhs[0].creatorExpected.send.bandwidth_bps
        )
        rhs_bandwidth = (
            rhs[0].loaderExpected.send.bandwidth_bps
            if rhs[1] == LS_LOADER
            else rhs[0].creatorExpected.send.bandwidth_bps
        )
        return lhs_bandwidth > rhs_bandwidth

    def handle_channel_status_update(
        self, handle: RaceHandle, channel_gid: str, status: ChannelStatus
    ) -> bool:
        """
        Purpose:
            Handle a change to channel status. Currently has no behavior.
        Args:
            handle: RACE handle associated with the channel status change
            channel_gid: GID of the channel whose status has changed
            status: New channel status
        Returns:
            Success of the operation.
        """
        logDebug("  ━☆ LinkWizard::handle_channel_status_update: called")
        logDebug("  ━☆ LinkWizard::handle_channel_status_update: returned")
        return True

    def handle_link_status_update(
        self,
        handle: RaceHandle,
        link_id: str,
        status: LinkStatus,
        properties: LinkProperties,
    ) -> bool:
        """
        Purpose:
            Handle a change to link status. This watches for link statuses associated with
            calls the LinkWizard previously made and triggers behavior for them. Primarily
            for createLink results this generates transmission of the LinkAddress to other
            nodes.
        Args:
            handle: The RaceHandle of the previous link-related call
            link_id: Currently unused
            status: The new status of the link
            properties: The current LinkProperties of the link
        Returns:
            Success of the operation. False if the handle did not correspond to a previous
            LinkWizard call.
        """
        logDebug(
            f"  ━☆ LinkWizard::handle_link_status_update: called for link ID: {link_id} "
            f" with status {LINK_STATUS[status]}"
        )
        if status == LINK_CREATED:
            if handle not in self._pending_create:
                # This link was not created by the LinkWizard, ignore.
                logDebug(
                    "  ━☆ LinkWizard::LINK_CREATED status update but no pendingCreate entry, "
                    "ignoring."
                )
                return False
            # list of personas we will want to load this link's address
            uuid_list = self._pending_create.get(handle, [])
            # Include ourselves in the list of personas associated with the link
            uuid_list.append(self._race_uuid)

            for uuid in uuid_list:
                if uuid != self._race_uuid:  # Don't send to ourselves
                    # TODO pass error status back if this fails?
                    self._request_load_link_address(
                        uuid, properties.channelGid, properties.linkAddress, uuid_list
                    )
            del self._pending_create[handle]

        elif status == LINK_LOADED:
            if handle not in self._pending_load:
                # This link was not loaded by the LinkWizard, ignore.
                logDebug(
                    "  ━☆ LinkWizard::LINK_LOADED status update but no pendingLoad entry,"
                    " ignoring."
                )
                return False
            del self._pending_load[handle]

        elif status == LINK_DESTROYED:
            if handle not in self._pending_load:
                # This link was not loaded by the LinkWizard, ignore.
                logDebug(
                    "  ━☆ LinkWizard::LINK_DESTROYED status update but no pendingLoad "
                    " entry, ignoring."
                )
                return False
            del self._pending_load[handle]

        logDebug("  ━☆ LinkWizard::handle_link_status_update: returned")
        return True

    def _respond_supported_channels(self, uuid: str) -> bool:
        """
        Purpose:
            Respond to a LinkWizard querySupportedChannels message with the list of
            channels this node supports.
        Args:
            uuid The string uuid of the persona to respond to.
        Returns:
            Success of the operation. Failure if the uuid could not be reached.
        """
        logDebug("  ━☆ LinkWizard::_respond_supported_channels: called")

        msg_json = json.dumps(
            {
                "supportedChannels": {
                    channel_gid: channel_props.currentRole.linkSide
                    for channel_gid, channel_props in self._sdk.getSupportedChannels().items()
                }
            }
        )
        msg = ExtClrMsg(
            msg=msg_json,
            _from=self._race_uuid,
            to=uuid,
            msgTime=1,  # TODO fix timestamp
            msgNonce=0,  # TODO fix nonce
            msgAmpIndex=0,
            uuid=0,
            ringTtl=0,
            ringIdx=0,
            msgType=ExtClrMsg.MSG_LINKS,
        )
        logDebug(f"  ━☆ LinkWizard::responding: {msg_json}")
        msg_str = self._encryptor.formatDelimitedMessage(msg)
        success = self._messenger.sendFormattedMsg(
            uuid, msg_str, msg.getTraceId(), msg.getSpanId()
        )
        logDebug("  ━☆ LinkWizard::_respond_supported_channels: returned")
        return success

    def _query_supported_channels(self, uuid: str) -> bool:
        """
        Purpose:
            Send a LinkWizard message querying the channels supported by uuid.
        Args:
            uuid: The uuid of the persona to query.
        Returns:
            Success of the operation. Failure if the uuid could not be reached.
        """
        logDebug(f"  ━☆ LinkWizard::_query_supported_channels: called for {uuid}")
        msg_json = json.dumps(
            {
                "getSupportedChannels": True,
            }
        )
        msg = ExtClrMsg(
            msg=msg_json,
            _from=self._race_uuid,
            to=uuid,
            msgTime=1,  # TODO fix timestamp
            msgNonce=0,  # TODO fix nonce
            msgAmpIndex=0,
            uuid=0,
            ringTtl=0,
            ringIdx=0,
            msgType=ExtClrMsg.MSG_LINKS,
        )
        msg_str = self._encryptor.formatDelimitedMessage(msg)
        success = self._messenger.sendFormattedMsg(
            uuid, msg_str, msg.getTraceId(), msg.getSpanId()
        )
        logDebug("  ━☆ LinkWizard::_query_supported_channels: returned")
        return success
